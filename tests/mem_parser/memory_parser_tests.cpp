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
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "mips_mem_parser.h"

constexpr uint32_t ADDR_TO_INDEX(uint32_t addr) {
    return addr >> 2;  // Convert line index to address
}

constexpr uint32_t INDEX_TO_ADDR(uint32_t index) {
    return index << 2;  // Convert address to line index
}

class MemoryParserTest : public ::testing::Test {
   protected:
    // Path for our temporary test file
    std::string test_filename = "test_memory.txt";
    bool keep_file_ = false;  // This can be set to true with an env variable

    // Sample instructions to use in our tests
    // Includes a few 0x00000000 instructions to validate the read instruction will recurse over
    // these
    std::vector<std::string> sample_lines = {"040103E8", "040204B0", "00003800", "00004000",
                                             "00005000", "040B0032", "040C0020", "00000000",
                                             "00000000", "00000000", "00000000", "00000000",
                                             "040103E8", "040204B0", "00003800", "00004000"};

    void SetUp() override {
        // Check for environment variable or parse command line
        const char* keep_file_env = std::getenv("KEEP_TEST_FILE");
        keep_file_ = (keep_file_env != nullptr && std::string(keep_file_env) == "1");

        // Create a test memory file
        std::ofstream file(test_filename);
        ASSERT_TRUE(file.is_open()) << "Failed to create test file";

        for (const auto& instr : sample_lines) {
            file << instr << std::endl;
        }
        file.close();
    }

    void TearDown() override {
        // Only remove the file if we're not keeping files
        if (!keep_file_) {
            std::filesystem::remove(test_filename);
        } else {
            // Print the path to the file that was kept
            std::cout << "Test file kept at: " << std::filesystem::absolute(test_filename)
                      << std::endl;

            // Optionally dump file contents
            std::cout << "File contents:" << std::endl;
            std::ifstream file(test_filename);
            if (file.is_open()) {
                std::string line;
                while (std::getline(file, line)) {
                    std::cout << line << std::endl;
                }
                file.close();
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

TEST_F(MemoryParserTest, backwardsReadMemory) {
    // Increment PC, then read memory backwards
    MemoryParser parser(test_filename);

    // First, advance the PC by reading several instructions sequentially
    for (int i = 0; i < 10; i++) {
        parser.readNextInstruction();
    }
    uint32_t expected_pc = 10 * 4;

    // Now PC should be at position 10, let's verify the current PC
    EXPECT_EQ(expected_pc, parser.getPC()) << "Program counter not advanced correctly";

    uint32_t read_addr = INDEX_TO_ADDR(5);  // Using our new constexpr function
    uint32_t expected = std::stoul(sample_lines[5], nullptr, 16);
    uint32_t actual = parser.readMemory(read_addr);

    EXPECT_EQ(expected, actual) << "Backwards memory read mismatch";
    EXPECT_EQ(expected_pc, parser.getPC()) << "Program counter shouldn't change after memory read";

    // Test another backwards read from a different position
    read_addr = INDEX_TO_ADDR(2);  // 3rd item in sample_lines
    expected = std::stoul(sample_lines[2], nullptr, 16);
    actual = parser.readMemory(read_addr);

    EXPECT_EQ(expected, actual) << "Second backwards memory read mismatch";
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
    uint32_t write_addr = MAX_LINE_COUNT * 4 + 4;
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

    uint32_t curr_count = parser.getFileLineCount();

    // Write beyond current file size
    uint32_t write_addr = ((static_cast<uint32_t>(sample_lines.size())) << 2) + (5 << 2);
    uint32_t new_value = 0xDEADBEEF;

    EXPECT_NO_THROW(parser.writeMemory(write_addr, new_value));

    // Verify file was extended
    EXPECT_EQ(curr_count + 5, parser.getFileLineCount()) << "File size not increased after write";

    // Read back the value
    uint32_t read_value = parser.readMemory(write_addr);
    EXPECT_EQ(new_value, read_value) << "Value not correctly written beyond original file size";
}
