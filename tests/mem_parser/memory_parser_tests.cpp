/**
 * @file memory_parser_tests.cpp
 * @brief Unit tests for MemoryParser class
 *
 * To run these you can config and build with Cmake. Then run the tests with:
 *  > cd build/Debug
 *  > ctest -N # List tests
 *  > ctest -R <test_name> # Run a specific test
 *  > ./bin/memory_parser_test --gtest_filter=MemoryParserTest.* # Run all tests
 */

#include <gtest/gtest.h>
#include <sys/types.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "mips_mem_parser.h"

class MemoryParserTest : public ::testing::Test {
   protected:
    // Path for our temporary test file
    std::string test_filename = "test_memory.txt";

    // Sample instructions to use in our tests
    // Includes a few 0x00000000 instructions to validate the read instruction will recurse over
    // these
    std::vector<std::string> sample_lines = {"040103E8", "040204B0", "00003800", "00004000",
                                             "00005000", "040B0032", "040C0020", "00000000",
                                             "00000000", "00000000", "00000000", "00000000",
                                             "040103E8", "040204B0", "00003800", "00004000"};

    void SetUp() override {
        // Create a test memory file
        std::ofstream file(test_filename);
        ASSERT_TRUE(file.is_open()) << "Failed to create test file";

        for (const auto& instr : sample_lines) {
            file << instr << std::endl;
        }
        file.close();
    }

    void TearDown() override {
        // Clean up test file
        std::filesystem::remove(test_filename);
    }
};

// Test constructor
TEST_F(MemoryParserTest, Constructor) {
    EXPECT_NO_THROW({ MemoryParser parser(test_filename); });

    // Test with non-existent file
    EXPECT_THROW({ MemoryParser parser("non_existent_file.txt"); }, std::runtime_error);
}

// Test correct file size
TEST_F(MemoryParserTest, FileSize) {
    MemoryParser parser(test_filename);
    EXPECT_EQ(parser.getFileLineCount(), sample_lines.size()) << "File size mismatch";
}

// Test reading instructions sequentially
TEST_F(MemoryParserTest, ReadEntireFileSequentially) {
    MemoryParser parser(test_filename);

    // Read all instructions and verify they match our sample
    for (const auto& expected_instr : sample_lines) {
        uint32_t expected = std::stoul(expected_instr, nullptr, 16);
        uint32_t actual = parser.readNextInstruction();
        EXPECT_EQ(expected, actual) << "Instruction mismatch";
    }

    // Test end of file exception
    EXPECT_THROW({ parser.readNextInstruction(); }, std::runtime_error);
}

TEST_F(MemoryParserTest, readMemoryOutOfBounds) {
    MemoryParser parser(test_filename);
    // Test reading out of bounds
    EXPECT_THROW({ parser.readMemory(0x2000); }, std::runtime_error);
}

TEST_F(MemoryParserTest, readMemoryUnaligned) {
    MemoryParser parser(test_filename);
    // Test reading unaligned address
    EXPECT_THROW({ parser.readMemory(0x1003); }, std::runtime_error);
}

TEST_F(MemoryParserTest, readMemoryValid) {
    MemoryParser parser(test_filename);
    // Test reading valid memory
    uint32_t expected = std::stoul(sample_lines[3], nullptr, 16);
    uint32_t actual = parser.readMemory(0x000C);
    EXPECT_EQ(expected, actual) << "Memory read mismatch";
}

TEST_F(MemoryParserTest, readMemoryWithAddedZeroLines) {
    MemoryParser parser(test_filename);
    uint32_t curr_count = parser.getFileLineCount();
    EXPECT_EQ(curr_count, sample_lines.size()) << "File size mismatch";

    // Add 10 lines
    uint32_t address = ((static_cast<uint32_t>(sample_lines.size())) << 2) + (10 << 2);
    uint32_t expected = 0x00000000;
    uint32_t actual = parser.readMemory(address);

    EXPECT_EQ(expected, actual) << "Memory read mismatch";
    EXPECT_EQ(parser.getFileLineCount(), curr_count + 10) << "File size mismatch";
}
