#include <iomanip>
#include <algorithm>
#include <chrono>
#include "io_helpers.hpp"
#include "lsb_rand.hpp"
#include "aes_helpers.hpp"

const std::uint64_t MIN = std::numeric_limits<std::uint16_t>::max();
const std::uint64_t MAX = std::numeric_limits<std::uint32_t>::max();

std::uint64_t generateSeed(int numPos) {
    auto now = std::chrono::high_resolution_clock::now();
    auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    std::mt19937_64 rng(static_cast<std::uint64_t>(nanoseconds));
    std::uniform_int_distribution<std::uint64_t> distribution(MIN, MAX);
    std::uint64_t seedVal = distribution(rng);
    int size = static_cast<uint32_t>(log10(numPos) + 1);

    std::stringstream Stream;
    Stream << seedVal << numPos << size;
    std::uint64_t f_seed;
    Stream >> f_seed;
    std::cout << "using seed:   " << f_seed << std::endl;

    return f_seed;
}

// Parse args
bool parseArgs(int& argc, char** argv, std::vector<int>& index) {
    std::vector<std::string> args(argv, argv + argc);

    if (argc < 2) {
        std::cerr << "rsteg --help for usage instructions." << std::endl;
        return false;
    }

    if (argc == 2 && (strcmp(argv[1], "--help") == 0)) {
        std::cout << "Rsteg version 1.0\n";
        std::cout << "GNU GENERAL PUBLIC LICENSE\nVersion 3, 29 June 2007\n";
        std::cout << "Author: valgrind-r https://github.com/valgrind-r\n";
        std::cout << "Available modes:\n";
        std::cout << "+-------+-------------------------------------------------------------------+\n";
        std::cout << "| Mode  | Description                                                       |\n";
        std::cout << "+-------+-------------------------------------------------------------------+\n";
        std::cout << "| enc   | encrypt file and embed in container                               |\n";
        std::cout << "| dec   | extract from container and decrypt files                          |\n";
        std::cout << "+------------------+--------------------------------------------------------+\n";
        std::cout << "| Key-derivation   | Description                                            |\n";
        std::cout << "+------------------+--------------------------------------------------------+\n";
        std::cout << "| ECDH -> AES-256  | Elliptic Curve Diffie-Hellman (NIST/c25519)            |\n";
        std::cout << "|         (SHA-2)  | with Advanced Encryption Standard (256-bit)            |\n";
        std::cout << "+------------------+--------------------------------------------------------+\n\n";
        std::cout << " Options: rsteg [enc|dec] --help for detailed instructions\n";
        std::cout << "+---------+-----------------------------------------------------------------+\n";
        std::cout << "| Option  | Description                                                     |\n";
        std::cout << "+---------+-----------------------------------------------------------------+\n";
        std::cout << "|  -i     | container file path                                             |\n";
        std::cout << "|         |     - input container path [ mode : enc ]                       |\n";
        std::cout << "|         |     - stego container path [ mode : dec ]                       |\n";
        std::cout << "|         |                                                                 |\n";
        std::cout << "|         | supported containers                                            |\n";
        std::cout << "|         | [ .png  .avi .mov .mkv .mp4 .webm .m2ts .wav .flac .alac ]      |\n";
        std::cout << "|         |                                                                 |\n";
        std::cout << "|         |                                                                 |\n";
        std::cout << "|  -o     | output path [ optional ]                                        |\n";
        std::cout << "|         |     - default [ mode : enc ]  out.[ container extension ]       |\n";
        std::cout << "|         |     - default [ mode : dec ]  file.[ embed file extension ]     |\n";
        std::cout << "|         |                                                                 |\n";
        std::cout << "|  -m     | path to file [ .txt / most archival formats supported ]         |\n";
        std::cout << "|  -rk    | path to openssl generated EC public key                         |\n";
        std::cout << "|  -pk    | path to openssl generated EC private key                        |\n";
        std::cout << "+---------+-----------------------------------------------------------------+\n";

        return false;
    }

    if (strcmp(argv[1], "enc") == 0) {
        if (argc < 10) {
            std::cerr << "usage: rsteg enc\n" << std::endl;
            std::cerr << "          -i      [ container ]" << std::endl;
            std::cerr << "          -m      [ embed file ]" << std::endl;
            std::cerr << "          -rk     [ recipient's public key ]" << std::endl;
            std::cerr << "          -pk     [ sender's private key ]" << std::endl;
            std::cerr << "OPTIONAL: -o      [ output file ]\n" << std::endl;
            std::cerr << "rsteg --help for more information" << std::endl;

            return false;
        }

        auto findArgIndex = [&](const std::string& option) {
            auto it = std::find(args.begin(), args.end(), option);
            return it != args.end() ? std::distance(args.begin(), it) : -1;
        };

        index.push_back(findArgIndex("-i"));
        index.push_back(findArgIndex("-m"));
        index.push_back(findArgIndex("-rk"));
        index.push_back(findArgIndex("-pk"));
        int oIndex = findArgIndex("-o");
        if (oIndex != -1) {
            index.push_back(oIndex);
        }
    }
    else if (strcmp(argv[1], "dec") == 0) {
        if (argc < 8) {
            std::cerr << "usage: rsteg dec\n" << std::endl;
            std::cerr << "          -i      [ container ]" << std::endl;
            std::cerr << "          -rk     [ sender's public key ]" << std::endl;
            std::cerr << "          -pk     [ recipient's private key ]" << std::endl;
            std::cerr << "OPTIONAL: -o      [ output file ]\n" << std::endl;
            std::cerr << "rsteg --help for more information" << std::endl;

            return false;
        }

        auto findArgIndex = [&](const std::string& option) {
            auto it = std::find(args.begin(), args.end(), option);
            return it != args.end() ? std::distance(args.begin(), it) : -1;
        };

        index.push_back(findArgIndex("-i"));
        index.push_back(findArgIndex("-rk"));
        index.push_back(findArgIndex("-pk"));
        int oIndex = findArgIndex("-o");
        if (oIndex != -1) {
            index.push_back(oIndex);
        }
    }

    // Check for invalid or duplicate arguments
    for (size_t i = 0; i < index.size(); ++i) {
        if (index[i] == -1 || std::count(index.begin(), index.end(), index[i]) > 1) {
            std::cerr << "Invalid or duplicate arguments... " << std::endl << "rsteg --help for more details." << std::endl;
            return false;
        }
    }

    return true;
}

std::string tar_helper(const std::vector<unsigned char>& data) {
    std::string filename = ".";
    for (size_t i = 0; data[i] != 0; ++i) {
        filename += static_cast<char>(data[i]);
    }
    return filename;
}

std::string getFileExtension(const std::vector<unsigned char>& data) {
    if (data.size() >= 4 && data[0] == 0x50 && data[1] == 0x4B && data[2] == 0x03 && data[3] == 0x04) {
        return ".zip";
    }
    else if (data.size() >= 257 && data[257] == 0x75 && data[258] == 0x73
            && data[259] == 0x74 && data[260] == 0x61 && data[261] == 0x72) {
        return tar_helper(data) + ".tar";
    }
    else if (data.size() >= 4 && data[0] == 0x1F && data[1] == 0x8B && data[2] == 0x08 && data[3] == 0x00) {
        return ".tar.gz";
    }
    else if (data.size() >= 5 && data[0] == 0xFD && data[1] == 0x37 && data[2] == 0x7A
            && data[3] == 0x58 && data[4] == 0x5A && data[5] == 0) {
        return ".tar.xz";
    }
    else if (data.size() >= 3 && data[0] == 0x42 && data[1] == 0x5A && data[2] == 0x68) {
        return ".tar.bz2";
    }
    else if (data.size() >= 4 && data[0] == 0x28 && data[1] == 0xB5 && data[2] == 0x2F && data[3] == 0xFD) {
        return ".tar.zst";
    }
    else if (data.size() >= 2 && data[0] == 0x37 && data[1] == 0x7A) {
        return ".7z";
    }
    else if (data.size() >= 4 && data[0] == 0x6B && data[1] == 0x6F && data[2] == 0x6C && data[3] == 0x79) {
        return ".dmg";
    }
    else if (data.size() >= 4 && data[0] == 0xAA && data[1] == 0x01) {
        return ".aar";
    }
    else if (data.size() >= 4 && data[0] == 0x2A && data[1] == 0x64 && data[2] == 0x61 && data[3] == 0x72) {
        return ".dar";
    }
    else if (data.size() >= 4 && data[0] == 0x43 && data[1] == 0x46 && data[2] == 0x53 && data[3] == 0x00) {
        return ".cfs";
    }
    else if (data.size() >= 7 && data[0] == 0x52 && data[1] == 0x61 && data[2] == 0x72 && 
        data[3] == 0x21 && data[4] == 0x1A && data[5] == 0x07 && data[6] == 0x00) {
        return ".rar";
    }
    else if (data.size() >= 3 && data[0] == 0xFF && data[1] == 0xD8 && data[2] == 0xFF) {
        return ".jpg";
    }
    else if (data.size() >= 4 && data[0] == 0x25 && data[1] == 0x50 && data[2] == 0x44 && data[3] == 0x46) {
        return ".pdf";
    }
    else {
        return ".txt"; // handling every case will bloat these branches use good practices and add your files to an archive before encoding.
    }
}

bool isVideoFile(const char* inputPath) {
    std::string path(inputPath);
    size_t dotPos = path.find_last_of('.');

    if (dotPos != std::string::npos && dotPos + 1 < path.length()) {
        std::string fileExtension = path.substr(dotPos);
        if (fileExtension == ".avi" ||fileExtension == ".mkv" ||
            fileExtension == ".mp4" || fileExtension == ".webm" ||
            fileExtension == ".mov" || fileExtension == ".m2t") {
            return true;
        }
    }
    
    return false;
}

bool isAudioFile(const char* inputPath) {
    std::string path(inputPath);
    size_t dotPos = path.find_last_of('.');

    if (dotPos != std::string::npos && dotPos + 1 < path.length()) {
        std::string fileExtension = path.substr(dotPos);
        if (fileExtension == ".mp3" || fileExtension == ".wav" ||
            fileExtension == ".flac" || fileExtension == ".aac" ||
            fileExtension == ".ogg" || fileExtension == ".m4a" ||
            fileExtension == ".opus" || fileExtension == ".wma") {
            return true;
        }
    }
    
    return false;
}

int main(int argc, char** argv) {
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();

    std::vector<int> index;
    if (!parseArgs(argc, argv, index)){
        return 1;
    }

    if (strcmp(argv[1], "enc") == 0)
    {
        std::string inputPath = argv[++index[0]];
        std::string inputFile = argv[++index[1]];
        std::string publicKey = argv[++index[2]];
        std::string privateKey = argv[++index[3]];
        std::string outputPath = (argc == 12) ?
            (argv[++index[4]] + (std::string(argv[index[4]]).find_last_of('.') == std::string::npos ?
            inputPath.substr(inputPath.find_last_of('.')) : "")) :
            ("./out" + (inputPath.find_last_of('.') != std::string::npos ? 
            inputPath.substr(inputPath.find_last_of('.')) : ""));

        std::pair<std::vector<int>, std::vector<unsigned char>> image;
        VideoInfo video; short vflag = 0;
        AudioInfo audio; short aflag = 0;
        
        if (isVideoFile(inputPath.c_str())) {
            video = readVideo(inputPath.c_str()); vflag = 1;
        }
        else if (isAudioFile(inputPath.c_str())) {
            audio = readAudio(inputPath.c_str()); aflag = 1;
        } else {
            std::cout << inputPath;
            image = readImage(inputPath.c_str());
        }

        std::vector<unsigned char> fileContents;
        if(!readBinaryFile(inputFile.c_str(), fileContents)){
            std::cerr << "Error:    unable to read embed file" << std::endl;
            return 1;
        }

        unsigned char messageKey[32];
        unsigned char iv[16]; 
        std::vector<unsigned char> sec = computeSharedSecret(privateKey, publicKey);
        deriveAesKeyAndIv(sec, messageKey, iv);
        std::vector<unsigned char> encryptedBytes(fileContents.size());
        int encryptedByteslength = encrypt(fileContents, static_cast<int>(fileContents.size()), messageKey, iv, encryptedBytes);

        /*  //  debug block
        std::cout << "AES-256 encrypted bytes: " << std::endl;
        bool first = true;
        for (auto ch : encryptedBytes) {
            if (!first) {
                std::cout << ' ';
            }
            std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(static_cast<unsigned char>(ch));
            first = false;
        }
        std::cout << std::dec << std::endl;  */

        // Calculate size for encoding
        int numPos = (static_cast<long>(encryptedBytes.size()) * 8) / (4 * 2) * 4; // this works but how ??

        std::cout << std::fixed << std::setprecision(1) << "minimum required container size:   " << static_cast<double>(numPos)/1024.0 << " KB" << std::endl; 

        if (numPos > image.second.size() && numPos > video.rawData.size() && numPos > audio.rawData.size()) {
            std::cerr << "Error:    insufficient container size" << std::endl;
            return 1;
        }

        std::cout << "file size:    " << std::fixed << std::setprecision(1) << static_cast<double>(encryptedBytes.size())/1024.0 << " KB" << std::endl;
        if (vflag == 1) {
            std::cout << "container size:   " << std::fixed << std::setprecision(1) << static_cast<double>(video.rawData.size())/1024.0 << " KB" << std::endl;
        } else if  (aflag == 1) {
            std::cout << "container size:   " << std::fixed << std::setprecision(1) << static_cast<double>(audio.rawData.size())/1024.0 << " KB" << std::endl;
        } else {
            std::cout << "container size:   " << std::fixed << std::setprecision(1) << static_cast<double>(image.second.size())/1024.0 << " KB" << std::endl;
        }

        std::uint64_t Seed = generateSeed(numPos);      
        if (Seed != 0) {
            unsigned char seedBytes[sizeof(Seed)];
            for (long long unsigned int i = 0; i < sizeof(Seed); ++i) {
                seedBytes[i] = (Seed >> (8 * i)) & 0xFF;
            }

            unsigned char encryptedSeed[AES_BLOCK_SIZE];
            int encryptedSeedLength = encrypt_seed(seedBytes, sizeof(seedBytes), messageKey, iv, encryptedSeed);

            std::cout << "AES-256 encrypted seed bytes:     ";
            for (int i = 0; i < encryptedSeedLength; ++i) {
                if (i != 0) {
                    std::cout << ' ';
                }
                std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(encryptedSeed[i]);
            }
            std::cout << std::dec << std::endl;

            auto start = std::chrono::high_resolution_clock::now();
            std::vector<int> pos = entropyChannel(Seed);
            auto stop = std::chrono::high_resolution_clock::now();

            auto duration = std::chrono::duration_cast<std::chrono::seconds>(stop - start);
            std::cout << "generated encoding sequence in " << std::setprecision(2) << static_cast<double>(duration.count()) << " s" << std::endl; 

            if (vflag == 1) {
                encode_lsb(video.rawData, encryptedBytes, pos);
            } else if (aflag == 1) {
                encode_lsb(audio.rawData, encryptedBytes, pos);
            } else {
                encode_lsb(image.second, encryptedBytes, pos);
            }

            std::vector<unsigned char> encodedSeedBytes;
            for (int i = 0; i < encryptedSeedLength; ++i) {
                encodedSeedBytes.push_back(encryptedSeed[i]);
            }
            encodedSeedBytes.push_back(encryptedSeedLength);

            if (vflag == 1) {
                if(!writeVideo(inputPath.c_str(), outputPath.c_str(), video.rawData, video.height, video.width, video.framerate, video.codec)) {
                    std::cerr << "Error: failed to write to container" << std::endl;
                    return 1;
                }
            }
            else if (aflag == 1) {
                if(!writeAudio(inputPath.c_str(), outputPath.c_str(), audio.rawData, audio.sampleRate, audio.channels, audio.codec)) {
                    std::cerr << "Error: failed to write to container" << std::endl;
                    return 1;
                }
            }
             else {
                if(!writeImage(outputPath.c_str(), image.second, image.first[0], image.first[1], image.first[2])) {
                    std::cerr << "Error: failed to write to container" << std::endl;
                    return 1;
                }
            }

            // write the seed
            std::ofstream outputFile(outputPath.c_str(), std::ios::out | std::ios::app | std::ios::binary);
            if (outputFile.is_open()) {
                outputFile.write(reinterpret_cast<char*>(encodedSeedBytes.data()), encodedSeedBytes.size());
                outputFile.close();
                std::cout << "seed written to container." << std::endl;
            } else {
                std::cerr << "Error: failed to embed seed bytes." << std::endl;
            }

            std::cout << "successfully created embedded container:\t" << outputPath << std::endl;

        } else {
            std::cerr << "Error: unhandled exception" << std::endl;
            return 1;
        }

    } else if (strcmp(argv[1], "dec") == 0) {
        const char* inputPath = argv[++index[0]];
        const char* publicKey = argv[++index[1]];
        const char* privateKey = argv[++index[2]];
        std::string outputPath = argc == 10 ? argv[++index[3]] : "./file";
        std::vector<unsigned char> encryptedSeed = decodeSeedBytes(inputPath);

        unsigned char messageKey[32];
        unsigned char iv[16]; 
        std::vector<unsigned char> sec = computeSharedSecret(privateKey, publicKey);
        deriveAesKeyAndIv(sec, messageKey, iv);

        std::cout << "extracted seed:   ";
        for (size_t i = 0; i < encryptedSeed.size(); ++i) {
            if (i != 0) {
                std::cout << ' ';
            }
            std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(encryptedSeed[i]);
        }
        std::cout << std::dec << std::endl;

        std::uint64_t decryptedSeed = 0;
        if(decrypt_seed(encryptedSeed.data(), static_cast<int>(encryptedSeed.size()), messageKey, iv, reinterpret_cast<unsigned char*>(&decryptedSeed)) < 0) {
            std::cerr << "Error:    failed to decrypt seed" << std::endl;
            return 1;
        }

        std::pair<std::vector<int>, std::vector<unsigned char>> stegoImage;
        std::vector<int> pos;
        int length = strlen(inputPath); VideoInfo video; int vflag = 0;
        AudioInfo audio; int aflag = 0;
        if (isVideoFile(inputPath)) {
            video = readVideo(inputPath); vflag = 1;
        }
        else if (isAudioFile(inputPath)) {
            audio = readAudio(inputPath); aflag = 1;
        } else {
            stegoImage = readImage(inputPath);
        }

        std::cout << "decrypted seed:   " << decryptedSeed << std::endl;

        auto start = std::chrono::high_resolution_clock::now();
        pos = entropyChannel(decryptedSeed);
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(stop - start);
        std::cout << "generated encoding sequence in " << std::setprecision(2) << static_cast<double>(duration.count()) << " s" << std::endl; 
        std::vector<unsigned char> extractedBytes;
        if (vflag == 1) {
            extractedBytes = decode_file(video.rawData, pos);
        }
        else if (aflag == 1) {
            extractedBytes = decode_file(audio.rawData, pos);
        } else {
            extractedBytes = decode_file(stegoImage.second, pos);
        }      

        std::vector<unsigned char> finalMessageBytes(extractedBytes.size());
        if(decrypt(extractedBytes, static_cast<int>(extractedBytes.size()), messageKey, iv, finalMessageBytes) < 0) {
            std::cerr << "Error:    unable to decrypt extracted file" << std::endl;
            return 1;
        }

        std::string outFile = outputPath + getFileExtension(finalMessageBytes);
        std::ofstream outputFile(outFile, std::ios::binary);
 
        if (outputFile.is_open()) {
            outputFile.write(reinterpret_cast<const char*>(finalMessageBytes.data()), finalMessageBytes.size());
            outputFile.close();
            std::cout << "reconstructed the file:   " << outFile << std::endl;
        } else {
            std::cerr << "Error: cannot reconstruct file" << std::endl;
            return false;
        }

    } else {
        std::cerr << "rsteg --help for more information" << std::endl;
        return 1;
    }

    return 0;
}
