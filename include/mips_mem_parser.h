/**
 * @file mips_mem_parser.h
 * @brief Memory parser for MIPS-lite processor simulation
 *
 * This module provides an abstraction for memory operations in a MIPS processor
 * simulation. It handles reading from and writing to memory, with support for
 * program counter operations (read next instruction, jump) and general memory
 * access. The memory contents are stored in a vector for fast access, with the
 * ability to read from an input file and write to an output file.
 *
 * Memory is word-addressable (4 bytes per word) with a maximum size of 4 KiB.
 * The class provides bounds checking, alignment validation, and dynamic vector
 * expansion as needed.
 *
 */

#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "mips_lite_defs.h"

constexpr uint32_t ADDR_TO_INDEX(uint32_t addr) { return (addr >> 2); }
constexpr uint32_t INDEX_TO_ADDR(uint32_t index) { return (index << 2); }
constexpr uint32_t MAX_MEMORY_SIZE = 4096;  // Maximum memory size in bytes (4 KiB)
constexpr uint32_t MAX_VEC_SIZE = (MAX_MEMORY_SIZE / mips_lite::WORD_SIZE);

class MemoryParser {
   private:
    std::string input_filename_;            // Input file to read from
    std::string output_filename_;           // Output file to write to
    std::vector<uint32_t> memory_content_;  // Vector to store file content
    uint32_t current_line_count_;
    uint32_t program_counter_;
    bool modified_;
    bool write_file_on_modified_;  // Flag to track if memory has been modified; if it has
                                   // the destructor will write to the output file

    void ensureIndexExists(uint32_t lineNumber);  // Dynamic Vector Allicator
    void writeToFile();

   public:
    explicit MemoryParser(const std::string& input_filename,
                          const std::string& output_filename = "");
    ~MemoryParser();

    uint32_t readNextInstruction();                      // PC Access
    uint32_t readMemory(uint32_t address);               // For Memory Access
    void writeMemory(uint32_t address, uint32_t value);  // For Memory Access
    void reset();                                        // PC Reset
    void jumpToInstruction(uint32_t address);            // PC Jump
    void printMemoryContent();                           // For debugging

    // Getters
    std::string getInputFilename() const { return input_filename_; }
    std::string getOutputFilename() const { return output_filename_; }
    size_t getNumMemoryElements() const { return memory_content_.size(); }
    uint32_t getPC() const { return program_counter_; }

    // Setters
    void setOutputFilename(const std::string& output_filename) {
        if (!output_filename_.empty()) {
            output_filename_ = output_filename;
        }
    }
    void setOutputFileOnModified(bool mode) { write_file_on_modified_ = mode; }
};
