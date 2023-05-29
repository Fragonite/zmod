#include <windows.h>
#include <map>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include "zmod_common.cpp"

int main(int argc, char* argv[]) {
    // This program reads the file passed in and loads the contents into a buffer
    // It then prints the offset of every instance of the pattern FILD VSYNC

    if(argc < 2) {
        std::cout << "Usage: " << argv[0] << " <file>" << std::endl;
        return 1;
    }

    auto file = std::ifstream(argv[1], std::ios::binary | std::ios::ate);
    if(!file.is_open()) {
        std::cout << "Failed to open file: " << argv[1] << std::endl;
        return 1;
    }

    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    auto buffer = std::vector<uint8_t>(size);

    if(!file.read((char*)buffer.data(), size)) {
        std::cout << "Failed to read file: " << argv[1] << std::endl;
        return 1;
    }

    file.close();

    auto base = buffer.data();
    auto address = zmod::find_pattern(base, size, zmod::parse_hex("79 E4 DB 05"), "xxxx") + 2;
    if(!address) {
        std::cout << "Failed to find pattern" << std::endl;
        return 1;
    }

    auto pattern = std::vector<uint8_t>(address, address + 6);
    address = base;
    while(true)
    {
        auto offset = address - base;
        address = zmod::find_pattern(address, size - offset, pattern, "xxxxxx");
        if(!address) {
            break;
        }
        // Print address with leading 0x and 8 digits padded with 0 and uppercase
        std::cout << "0x" << std::uppercase << std::setfill('0') << std::setw(8) << std::hex << address - base + 0xC00 << "," << std::endl;
        ++address;
    }

    // Press any key to continue
    std::cin.get();
    return 0;
}