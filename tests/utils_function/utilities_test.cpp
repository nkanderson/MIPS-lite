#include "utilities.h"

#include <gtest/gtest.h>
#include <sys/types.h>

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// Convert a hexadecimal string to a uint32_t
uint32_t hexStringToUint(const std::string& hexStr) {
    uint32_t value;
    std::stringstream ss;
    ss << std::hex << hexStr;
    ss >> value;
    return value;
}

// Run tests for sign extension with hexadecimal inputs
// Keeping as helper function just in case. Can delete if needed.
void runSignExtendHexTests() {
    std::vector<std::string> sample_lines = {"040103E8", "040204B0", "00003800", "00004000",
                                             "00005000", "040B0032", "040C0020", "00000000",
                                             "00000000", "00000000", "00000000", "00000000",
                                             "040103E8", "040204B0", "00003800", "00004000"};

    for (const auto& line : sample_lines) {
        uint32_t full_instruction = hexStringToUint(line);
        int16_t immediate = static_cast<int16_t>(full_instruction & 0xFFFF);  // signed cast

        int32_t extended_immediate = signExtend(immediate);

        // Determine if sign-extended value is negative
        std::string signInfo = (extended_immediate < 0) ? "Negative" : "Positive";

        // Print original and extended values
        std::cout << "Hex: " << line << " | Immediate: 0x" << std::hex
                  << (full_instruction & 0xFFFF) << " | Sign-Extended: " << std::dec
                  << extended_immediate << " (" << signInfo << ")" << std::endl;
    }

    std::cout << "All hex-based signExtend tests completed!" << std::endl;
}

// Main function
/* int main() {
    // runSignExtendHexTests();


    return 0;

}*/

TEST(UtilitiesTest, HexConversionCheck) {
    std::vector<std::string> sample_lines = {"040103E8", "040204B0", "00003800", "00004000",
                                             "00005000", "040B0032", "040C0020", "00000000",
                                             "00000000", "00000000", "00000000", "00000000",
                                             "040103E8", "040204B0", "00003800", "00004000"};

    for (const auto& line : sample_lines) {
        uint32_t full_instruction = hexStringToUint(line);
        int16_t immediate = static_cast<int16_t>(full_instruction & 0xFFFF);

        int32_t extended = signExtend(immediate);

        // Basic validation — immediate is properly sign-extended
        if (immediate < 0) {
            EXPECT_LT(extended, 0);
        } else {
            EXPECT_GE(extended, 0);
        }
    }
}
