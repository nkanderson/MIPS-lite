#include "utilities.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

//ChatGPT non MIPS sign extend dummy testing
void runSignExtendTests() {
    // Positive values (sign bit 0)
    assert(signExtend(0b000101, 6) == 5);     // 6-bit value for +5
    assert(signExtend(0b011111, 6) == 31);    // 6-bit value for +31

    // Negative values (sign bit 1)
    assert(signExtend(0b111011, 6) == -5);    // 6-bit value for -5
    assert(signExtend(0b100000, 6) == -32);   // 6-bit value for -32

    // 12-bit positive immediate
    assert(signExtend(0x07F, 12) == 127);     // 127
    assert(signExtend(0x7FF, 12) == 2047);    // 2047

    // 12-bit negative immediate
    assert(signExtend(0x800, 12) == -2048);   // -2048
    assert(signExtend(0xFFF, 12) == -1);       // -1

    // Edge case: zero
    assert(signExtend(0x0, 12) == 0);
    assert(signExtend(0x0, 6) == 0);

    std::cout << " All ChatGPT dummy signExtend tests passed!" << std::endl;

}

//Testing sample lines pulled from memory_parser_tests.cpp
uint32_t hexStringToUint(const std::string& hexStr) {
    uint32_t value;
    std::stringstream ss;
    ss << std::hex << hexStr;
    ss >> value;
    return value;
}

void runSignExtendHexTests() {
    // Sample instruction hex lines
    std::vector<std::string> sample_lines = {
        "040103E8", "040204B0", "00003800", "00004000",
        "00005000", "040B0032", "040C0020", "00000000",
        "00000000", "00000000", "00000000", "00000000",
        "040103E8", "040204B0", "00003800", "00004000"
    };

    // Example: for each line, extract the immediate (lower 16 bits) and sign-extend it
    for (const auto& line : sample_lines) {
        uint32_t full_instruction = hexStringToUint(line);

        uint16_t immediate = full_instruction & 0xFFFF; // Lower 16 bits

        int32_t extended_immediate = signExtend(immediate, 16);

        // Determine if sign-extended value is negative
        std::string signInfo = (extended_immediate < 0) ? "Negative" : "Positive";

        // For testing: print original and extended
        std::cout << "Hex: " << line 
                  << " | Immediate: 0x" << std::hex << immediate 
                  << " | Sign-Extended: " << std::dec << extended_immediate
                  << " (" << signInfo << ")" 
                  << std::endl;

        // Optional: assert certain things depending on known expectations
        // e.g., assert(extended_immediate == expected_value);
    }

    std::cout << "All hex-based signExtend tests completed!" << std::endl;
}

int main() {
    runSignExtendTests();
    runSignExtendHexTests();
    return 0;
}
