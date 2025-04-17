/**
 * @file memory_parser_tests.cpp
 * @brief Unit tests for MemoryParser class
 *
 */

#include <gtest/gtest.h>
#include <sys/types.h>

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "mips_mem_parser.h"

class MemoryParserTest : public ::testing::Test {
   protected:
    // Path for our temporary test file
    std::string test_dir = std::filesystem::path(__FILE__).parent_path().string();
    std::string test_filename;

    // Sample instructions to use in our tests
    std::vector<std::string> sample_lines = {"040103E8", "040204B0", "00003800", "00004000",
                                             "00005000", "040B0032", "040C0020", "00000000",
                                             "00000000", "00000000", "00000000", "00000000",
                                             "040103E8", "040204B0", "00003800", "00004000"};

    void SetUp() override {
        test_filename = test_dir + "/test_memory.txt";
        std::ofstream file(test_filename);
        ASSERT_TRUE(file.is_open()) << "Failed to create test file";

        for (const auto& instr : sample_lines) {
            file << instr << std::endl;
        }
        file.close();
    }

    void TearDown() override {
        // Clean up by removing the test file
        if (std::filesystem::exists(test_filename)) {
            try {
                std::filesystem::remove(test_filename);
            } catch (const std::filesystem::filesystem_error& e) {
                std::cerr << "Error removing test file: " << e.what() << std::endl;
            }
        }
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
    EXPECT_EQ(parser.getNumMemoryElements(), sample_lines.size()) << "File size mismatch";
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
    size_t curr_size = parser.getNumMemoryElements();
    EXPECT_EQ(curr_size, sample_lines.size()) << "File size mismatch";

    // Add 10 lines
    uint32_t index = static_cast<uint32_t>(sample_lines.size() - 1 + 10);
    uint32_t address = INDEX_TO_ADDR(index);
    uint32_t expected = 0x00000000;
    uint32_t actual = parser.readMemory(address);

    EXPECT_EQ(expected, actual) << "Memory read mismatch";
    EXPECT_EQ(parser.getNumMemoryElements(), curr_size + 10) << "File size mismatch";
}

TEST_F(MemoryParserTest, jumpToInstructionValid) {
    MemoryParser parser(test_filename);

    // Test jumping to a valid address
    uint32_t jump_addr = INDEX_TO_ADDR(5);  // Jump to 6th instruction
    EXPECT_NO_THROW(parser.jumpToInstruction(jump_addr));

    // PC should be updated
    EXPECT_EQ(jump_addr, parser.getPC()) << "Program counter not updated after jump";

    // Test that the next instruction read is correct
    uint32_t expected = std::stoul(sample_lines[5], nullptr, 16);
    uint32_t actual = parser.readNextInstruction();
    EXPECT_EQ(expected, actual) << "Wrong instruction read after jump";

    // PC should be incremented after read
    EXPECT_EQ(jump_addr + 4, parser.getPC()) << "Program counter not incremented after read";
}

TEST_F(MemoryParserTest, jumpToInstructionUnaligned) {
    MemoryParser parser(test_filename);

    // Test jumping to an unaligned address
    uint32_t jump_addr = INDEX_TO_ADDR(5) + 2;  // Not divisible by 4
    EXPECT_THROW(parser.jumpToInstruction(jump_addr), std::runtime_error);

    // PC should remain unchanged
    EXPECT_EQ(0, parser.getPC()) << "Program counter changed after failed jump";
}

TEST_F(MemoryParserTest, jumpToInstructionOutOfBounds) {
    MemoryParser parser(test_filename);

    // Test jumping beyond bounds
    uint32_t jump_addr = MAX_MEMORY_SIZE + 4;
    EXPECT_THROW(parser.jumpToInstruction(jump_addr), std::runtime_error);

    // PC should remain unchanged
    EXPECT_EQ(0, parser.getPC()) << "Program counter changed after failed jump";
}

TEST_F(MemoryParserTest, jumpThenReadSequentially) {
    MemoryParser parser(test_filename);

    // Jump to the middle
    uint32_t jump_addr = INDEX_TO_ADDR(8);
    parser.jumpToInstruction(jump_addr);

    // Read several instructions sequentially after jump
    for (size_t i = 8; i < 12; i++) {
        uint32_t expected = std::stoul(sample_lines[i], nullptr, 16);
        uint32_t actual = parser.readNextInstruction();
        EXPECT_EQ(expected, actual) << "Instruction mismatch after jump at index " << i;
    }

    // Verify final PC position
    EXPECT_EQ(jump_addr + (4 * 4), parser.getPC())
        << "Final PC position incorrect after sequential reads";
}

TEST_F(MemoryParserTest, writeMemoryValid) {
    MemoryParser parser(test_filename);

    // Write to an existing memory location
    uint32_t write_addr = INDEX_TO_ADDR(3);  // 4th instruction
    uint32_t new_value = 0xAABBCCDD;

    EXPECT_NO_THROW(parser.writeMemory(write_addr, new_value));

    // Read back the value to verify
    uint32_t read_value = parser.readMemory(write_addr);
    EXPECT_EQ(new_value, read_value) << "Value not correctly written to memory";

    // Verify the program counter wasn't affected
    EXPECT_EQ(0, parser.getPC()) << "Program counter changed after memory write";
}

TEST_F(MemoryParserTest, writeMemoryUnaligned) {
    MemoryParser parser(test_filename);

    // Write to an unaligned address
    uint32_t write_addr = INDEX_TO_ADDR(3) + 1;  // Not divisible by 4
    uint32_t new_value = 0xAABBCCDD;

    EXPECT_THROW(parser.writeMemory(write_addr, new_value), std::runtime_error);
}

TEST_F(MemoryParserTest, writeMemoryOutOfBounds) {
    MemoryParser parser(test_filename);

    // Write beyond the maximum memory size
    uint32_t write_addr = MAX_VEC_SIZE * 4 + 4;
    uint32_t new_value = 0xAABBCCDD;

    EXPECT_THROW(parser.writeMemory(write_addr, new_value), std::runtime_error);
}

TEST_F(MemoryParserTest, writeReadInteraction) {
    MemoryParser parser(test_filename);

    // Read some instructions first
    for (int i = 0; i < 5; i++) {
        parser.readNextInstruction();
    }

    // Write to a previous memory location
    uint32_t write_addr = INDEX_TO_ADDR(2);
    uint32_t new_value = 0x12345678;

    parser.writeMemory(write_addr, new_value);

    // Read from the written location
    uint32_t read_value = parser.readMemory(write_addr);
    EXPECT_EQ(new_value, read_value) << "Value not correctly written/read";

    // Continue reading from current PC
    uint32_t expected_pc = 5 * 4;
    EXPECT_EQ(expected_pc, parser.getPC()) << "PC not preserved during write/read";

    uint32_t expected = std::stoul(sample_lines[5], nullptr, 16);
    uint32_t actual = parser.readNextInstruction();
    EXPECT_EQ(expected, actual) << "Incorrect instruction read after write/read operations";
}

TEST_F(MemoryParserTest, writeMemoryBeyondCurrentSize) {
    MemoryParser parser(test_filename);

    uint32_t curr_size = parser.getNumMemoryElements();

    // Write beyond current file size
    uint32_t write_addr = ((static_cast<uint32_t>(sample_lines.size() - 1)) << 2) + (5 << 2);
    uint32_t new_value = 0xDEADBEEF;

    EXPECT_NO_THROW(parser.writeMemory(write_addr, new_value));

    // Verify file was extended
    EXPECT_EQ(curr_size + 5, parser.getNumMemoryElements())
        << "File size not increased after write";

    // Read back the value
    uint32_t read_value = parser.readMemory(write_addr);
    EXPECT_EQ(new_value, read_value) << "Value not correctly written beyond original file size";
}
