#include <random>
#include <bitset>

void encode_lsb(std::vector<unsigned char>& iData, std::vector<unsigned char>& fileData, std::vector<int>& positions) {
	int b = 0;
    int c = 1;
    int shift = 6;
    bool err = false;

    std::vector<unsigned char>copyofData = fileData;
    std::vector<unsigned char>checkbits;
    unsigned char currentByte = 0x00;

    std::cout << "encoding file ..." << std::endl;

	for (auto position : positions)
	{
		unsigned char val = iData[position];

        if(b>=fileData.size()) {
            std::cerr << "Error:    past eof error" << std::endl;
            break;
        }

		if (c % 5 == 0) {
			for (auto checkbit : checkbits) {
				unsigned char tmp1 = checkbit;
				currentByte |= ((tmp1 & 0x03) << shift);
				shift -= 2;
			}

            std::bitset<16> x(currentByte);
            std::bitset<16> y(copyofData[b]);

            if(currentByte != copyofData[b]) {
                std::cout << "Error:    encoding expected:  " << x << "     got: " << y << std::endl;
                err = true;
            }

            ++b;
            c = 1;
            shift = 6;
            checkbits.clear();
            currentByte = 0x00;
		}

		val &= 0xFC;
		auto tmp = fileData[b];
		val |= ((tmp & 0xc0) >> 6);

        unsigned char dataToWrite = static_cast<unsigned char>((tmp & 0xc0) >> 6);

		checkbits.push_back(val);

		fileData[b] <<= 2;

		++c;
            
		iData[position] = val;
 	}
}

std::vector<unsigned char> decode_file(std::vector<unsigned char>& iFile, std::vector<int>& positions) {

    std::cout << "decoding file ..." << std::endl;

    std::vector<unsigned char> data;
    unsigned char currentByte = 0x00;
    int shift = 6; int b = 0; int n = 0; int corrupt = 0;

    for (auto position : positions) {

        unsigned char val = iFile[position];
            
        unsigned char tmp = val;

        currentByte |= ((tmp & 0x03) << shift);

        shift -= 2;
        ++n;

        if (shift < 0) {
            data.push_back(currentByte);
            currentByte = 0x00;
            shift = 6;
        }
    }

    return data;
}

std::vector<int> entropyChannel(std::uint64_t seed) {
    if (seed == 0) {
        std::cerr << "Error: bad seed" << std::endl;
        exit(1);
    }

    std::cout << "Generating entropy ..." << std::endl;

    int numPos = 0;
    int pos_len = seed % 10;
    seed /= 10;
    for (int i = 0; i < pos_len; ++i) {
        numPos += (seed % 10) * static_cast<int>(pow(10, i));
        seed /= 10;
    }

    if (numPos <= 0) {
        std::cerr << "Error: bad entropy" << std::endl;
        exit(1);
    }

    std::vector<int> pos(numPos);
    std::iota(pos.begin(), pos.end(), 0);
    std::seed_seq seedSeq{ static_cast<unsigned int>(seed) };
    std::mt19937_64 gen(seedSeq);
    std::shuffle(pos.begin(), pos.end(), gen);

    return pos;
}
