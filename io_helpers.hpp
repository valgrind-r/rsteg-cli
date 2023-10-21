#include <iostream>
#include <fstream>
#include <cstdlib>
#include <sstream>
#include <vector>
#include <cstdint>
#include <array>

extern "C" {
    #include <png.h>
}

struct VideoInfo {
    int width;
    int height;
    int numChannels;
    double framerate;
    std::string codec;
    std::vector<unsigned char> rawData;
};

struct AudioInfo {
    std::string codec;
    int sampleRate;
    int channels;
    std::vector<unsigned char> rawData;
};

// update for linux IO
bool readBinaryFile(const char* filename, std::vector<unsigned char>& data) {
    std::ifstream inputFile(filename, std::ios::binary);
    if (!inputFile.is_open()) {
        std::cerr << "Error:    unable to open the file" << std::endl;
        return false;
    }

    inputFile.seekg(0, std::ios::end);
    std::streamsize fileSize = inputFile.tellg();
    inputFile.seekg(0, std::ios::beg);

    if (fileSize <= 0) {
        std::cerr << "Error:    no data to read" << std::endl;
        return false;
    }

    data.resize(static_cast<size_t>(fileSize));
    inputFile.read(reinterpret_cast<char*>(data.data()), fileSize);

    inputFile.close();

    return true;
}

std::pair<std::vector<int>, std::vector<unsigned char>> readImage(const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Error:     unable to read PNG file\n");
        exit(1);
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        fclose(fp);
        fprintf(stderr, "png_create_read_struct failed.\n");
        exit(1);
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        fclose(fp);
        png_destroy_read_struct(&png, NULL, NULL);
        fprintf(stderr, "png_create_info_struct failed.\n");
        exit(1);
    }

    if (setjmp(png_jmpbuf(png))) {
        fclose(fp);
        png_destroy_read_struct(&png, &info, NULL);
        fprintf(stderr, "Error during png_init_io or png_read_info.\n");
        exit(1);
    }

    png_init_io(png, fp);
    png_read_info(png, info);

    int width = png_get_image_width(png, info);
    int height = png_get_image_height(png, info);
    png_byte color_type = png_get_color_type(png, info);
    png_byte bit_depth = png_get_bit_depth(png, info);

    int num_channels = (color_type == PNG_COLOR_TYPE_RGB) ? 3 : 4;

    std::vector<unsigned char> imageData;
    png_bytep row = (png_bytep)malloc(num_channels * width);

    for (int y = 0; y < height; y++) {
        png_read_row(png, row, NULL);
        for (int x = 0; x < width; x++) {
            for (int c = 0; c < num_channels; c++) {
                imageData.push_back(row[x * num_channels + c]);
            }
        }
    }

    free(row);
    fclose(fp);
    png_destroy_read_struct(&png, &info, NULL);

    return std::make_pair(std::vector<int>{width, height, num_channels}, imageData);
}

bool writeImage(const char* filename, const std::vector<unsigned char>& imageData, int width, int height, int numChannels) {
    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "Error:     failed to create output PNG\n");
        return false;
    }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        fclose(fp);
        fprintf(stderr, "png_create_write_struct failed.\n");
        fclose(fp);
        return false;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        fclose(fp);
        png_destroy_write_struct(&png, (png_infopp)NULL);
        fprintf(stderr, "png_create_info_struct failed.\n");
        return false;
    }

    if (setjmp(png_jmpbuf(png))) {
        fclose(fp);
        png_destroy_write_struct(&png, &info);
        fprintf(stderr, "Error during png_init_io or png_write_info.\n");
        return false;
    }

    png_init_io(png, fp);

    png_byte color_type;
    if (numChannels == 3) {
        color_type = PNG_COLOR_TYPE_RGB;
    } else if (numChannels == 4) {
        color_type = PNG_COLOR_TYPE_RGBA;
    } else {
        fclose(fp);
        png_destroy_write_struct(&png, &info);
        fprintf(stderr, "Error:     unsupported number of channels.\n");
        return false;
    }

    png_set_IHDR(png, info, width, height, 8, color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);

    png_set_compression_level(png, 0);
    png_set_compression_strategy(png, 0);
    png_set_filter(png, 0, PNG_FILTER_NONE);

    std::vector<png_byte> row(numChannels * width);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < numChannels * width; x++) {
            row[x] = imageData[y * numChannels * width + x];
        }
        png_write_row(png, row.data());
    }

    png_write_end(png, NULL);

    fclose(fp);
    png_destroy_write_struct(&png, &info);

    return true;
}

// extract seed
std::vector<unsigned char> decodeSeedBytes(const std::string& filePath) {
    std::vector<unsigned char> decodedSeedBytes;
    
    std::ifstream inputFile(filePath, std::ios::binary);
    
    if (!inputFile.is_open()) {
        std::cerr << "Error: unable to read seed" << std::endl;
        exit(1);
    }
    
    inputFile.seekg(-1, std::ios::end);
    int seedLength = 0;
    inputFile.read(reinterpret_cast<char*>(&seedLength), 1);

    if (seedLength < 0) {
        std::cerr << "Error: invalid seed length" << std::endl;
        exit(1);
    }
    
    decodedSeedBytes.resize(seedLength);

    inputFile.seekg(-seedLength-1, std::ios::cur);
    inputFile.read(reinterpret_cast<char*>(&decodedSeedBytes[0]), seedLength);
    
    inputFile.close();
    
    return decodedSeedBytes;
}

// ffmpeg/ffprobe subroutines
VideoInfo readVideo(const char* videoFileName) {
    VideoInfo videoInfo;
    std::string streamCheckCmd = "ffprobe -v error -select_streams v:0 -show_entries stream=codec_name -of default=noprint_wrappers=1:nokey=1 ";
    streamCheckCmd += videoFileName;
    std::array<char, 128> buffer;
    std::string result;
    FILE* pipe = popen(streamCheckCmd.c_str(), "r");
    if (!pipe) {
        std::cerr << "Error: Could not open pipe to ffprobe." << std::endl;
        exit(1);
    }
    
    std::string videoCodec;
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        videoCodec += buffer.data();
    }
    pclose(pipe);

    if (videoCodec.empty()) {
        std::cerr << "Error: No video stream found in the input file." << std::endl;
        exit(1);
    }

    std::string audioCheckCmd = "ffprobe -v error -select_streams a:0 -show_entries stream=codec_name -of default=noprint_wrappers=1:nokey=1 ";
    audioCheckCmd += videoFileName;
    pipe = popen(audioCheckCmd.c_str(), "r");
    if (!pipe) {
        std::cerr << "Error: Could not open pipe to ffprobe." << std::endl;
        exit(1);
    }
    
    std::string audioCodec;
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        audioCodec += buffer.data();
    }
    pclose(pipe);

    if (audioCodec.empty()) {
        std::cerr << "Error: Invalid video file." << std::endl;
        exit(1);
    }

    std::string metadataCmd = "ffprobe -v error -select_streams v:0 -show_entries stream=codec_name,width,height,r_frame_rate -of default=noprint_wrappers=1:nokey=1 ";
    metadataCmd += videoFileName;
    pipe = popen(metadataCmd.c_str(), "r");
    if (!pipe) {
        std::cerr << "Error: Could not open pipe to ffprobe." << std::endl;
        exit(1);
    }

    result.clear();
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    pclose(pipe);

    std::istringstream iss(result);
    std::string value;
    if (std::getline(iss, value)) {
        videoInfo.codec = value;
    }
    if (std::getline(iss, value)) {
        videoInfo.height = std::stoi(value);
    }
    if (std::getline(iss, value)) {
        videoInfo.width = std::stoi(value);
    }
    if (std::getline(iss, value)) {
        std::istringstream framerateStream(value);
        int numerator, denominator;
        char slash;
        if ((framerateStream >> numerator >> slash >> denominator) && denominator != 0) {
            videoInfo.framerate = static_cast<double>(numerator) / denominator;
        } else {
            std::cerr << "Error: failed fetching video metadata." << std::endl;
            exit(1);
        }
    }

    std::string rawDataCmd = "ffmpeg -i " + std::string(videoFileName) + " -f rawvideo -";
    pipe = popen(rawDataCmd.c_str(), "rb");
    if (!pipe) {
        std::cerr << "Error: Could not open pipe to FFmpeg." << std::endl;
        exit(1);
    }

    videoInfo.rawData.clear();
    unsigned char bufferArray[4096];
    size_t bytesRead;
    while ((bytesRead = fread(bufferArray, 1, sizeof(bufferArray), pipe)) > 0) {
        videoInfo.rawData.insert(videoInfo.rawData.end(), bufferArray, bufferArray + bytesRead);
    }
    pclose(pipe);

    videoInfo.numChannels = 3; // Expectation
    std::cout << "Video Codec: " << videoInfo.codec << std::endl;
    std::cout << "Width: " << videoInfo.width << "\tHeight: " << videoInfo.height << std::endl;
    std::cout << "Framerate: " << videoInfo.framerate << std::endl;

    return videoInfo;
}

bool writeVideo(const char* inputVideoFileName, const char* outputVideoFileName, const std::vector<unsigned char>& bytes, int width, int height, double framerate, std::string& vCodec) {
    std::string codec;
    if (vCodec == "hevc")
        codec = " libx265 -x265-params lossless=1 ";
    else if (vCodec == "h264" || vCodec == "mpeg4")
        codec = " libx264 -crf 0 -g 24 ";
    else if (vCodec == "vp9" || vCodec == "vp8")
        codec = " libvpx-vp9 -lossless 1 -g 24 ";
    else if (vCodec == "av1")
        codec = " libsvtav1 -g 24 -svtav1-params lossless=1 ";
    else
        codec = " ffv1 ";
    
    std::string cmd = "ffmpeg -y -f rawvideo -s ";
    cmd += std::to_string(width) + "x" + std::to_string(height);
    cmd += " -r " + std::to_string(framerate) + " -i - ";
    cmd += "-i " + std::string(inputVideoFileName);
    cmd += " -map 0:v -map 1:a ";
    cmd += " -c:v " + codec;
    cmd += "-c:a copy -copyts ";
    cmd += " -map_metadata 1 ";
    cmd += " -shortest ";
    cmd += outputVideoFileName;
    std::cout << cmd << std::endl;
    FILE* pipe = popen(cmd.c_str(), "wb");
    if (!pipe) {
        std::cerr << "Error: Could not open pipe to FFmpeg." << std::endl;
        return false;
    }
    try {
        fwrite(bytes.data(), 1, bytes.size(), pipe);
    } catch (...) {
        std::cerr << "Error: Failed to initialize ffmpeg." << std::endl;
        return false;
    }

    int status = _pclose(pipe);
    if (status == -1) {
        std::cerr << "Error: FFmpeg process failed to terminate." << std::endl;
        exit(EXIT_FAILURE);
    }

    return true;
}

AudioInfo readAudio(const char* audioFileName) {
    AudioInfo audioInfo;
    std::string cmd = "ffprobe -v error -select_streams a:0 -show_entries stream=codec_name,sample_rate,channels -of default=noprint_wrappers=1:nokey=1 ";
    cmd += audioFileName;
    std::array<char, 128> buffer;
    std::string result;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        std::cerr << "Error: Could not open pipe to ffprobe." << std::endl;
        exit(1);
    }

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    pclose(pipe);

    std::istringstream iss(result);
    std::string value;
    if (std::getline(iss, value)) {
        audioInfo.codec = value;
    }
    if (std::getline(iss, value)) {
        audioInfo.sampleRate = std::stoi(value);
    }
    if (std::getline(iss, value)) {
        audioInfo.channels = std::stoi(value);
    }

    std::string rawDataCmd = "ffmpeg -i " + std::string(audioFileName) + " -f s16le -acodec pcm_s16le -";
    pipe = popen(rawDataCmd.c_str(), "rb");
    if (!pipe) {
        std::cerr << "Error: Could not open pipe to FFmpeg." << std::endl;
        exit(1);
    }

    audioInfo.rawData.clear();
    unsigned char bufferArray[4096];
    size_t bytesRead;
    while ((bytesRead = fread(bufferArray, 1, sizeof(bufferArray), pipe)) > 0) {
        audioInfo.rawData.insert(audioInfo.rawData.end(), bufferArray, bufferArray + bytesRead);
    }
    pclose(pipe);

    std::cout << "Audio Codec: " << audioInfo.codec << std::endl;
    std::cout << "Sample Rate: " << audioInfo.sampleRate << "\tChannels: " << audioInfo.channels << std::endl;

    return audioInfo;
}

bool writeAudio(const char* inputFile, const char* outputAudioFileName, const std::vector<unsigned char>& bytes, int sampleRate, int channels, std::string& codec) {
    std::string codecOption; std::string fileName = std::string(outputAudioFileName);
    size_t dotPos = fileName.find_last_of('.');
    fileName = fileName.substr(0, dotPos);
    if (codec == "aac" || codec == "alac") {
        codecOption = " alac ";
        fileName += ".m4a";
    } else if (codec == "pcm_s16le") {
        codecOption = " pcm_s16le ";
        fileName += ".wav";
    }
     else {
        fileName += ".flac";
        codecOption = " flac ";
    }
    
    std::string cmd = "ffmpeg -y -f s16le -ar " + std::to_string(sampleRate) + " -ac " + std::to_string(channels);
    cmd += " -i - -i " + std::string(inputFile);
    cmd += " -map 0:a -c:a " + codecOption + " -map_metadata 1 ";
    cmd += fileName;
    std::cout << cmd << std::endl;

    FILE* pipe = popen(cmd.c_str(), "wb");
    if (!pipe) {
        std::cerr << "Error: Could not open pipe to FFmpeg." << std::endl;
        return false;
    }
    try {
        fwrite(bytes.data(), 1, bytes.size(), pipe);
    } catch (...) {
        std::cerr << "Error: Failed to initialize ffmpeg." << std::endl;
        return false;
    }

    int status = pclose(pipe);
    if (status == -1) {
        std::cerr << "Error: FFmpeg process failed to terminate." << std::endl;
        return false;
    }

    return true;
}
