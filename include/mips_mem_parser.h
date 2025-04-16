#pragma once

#include <cstdint>
#include <fstream>
#include <string>

#include "mips_lite_defs.h"

constexpr uint32_t PC_TO_LINE(uint32_t pc) { return (pc / mips_lite::WORD_SIZE); }
constexpr uint32_t LINE_TO_PC(uint32_t line) { return (line * mips_lite::WORD_SIZE); }
constexpr uint32_t MAX_MEMORY_SIZE = 4096;  // Maximum memory size in bytes (4 KiB)
constexpr uint32_t MAX_LINE_COUNT =
    (MAX_MEMORY_SIZE / mips_lite::WORD_SIZE);  // Maximum number of lines (1 line = 4 bytes)

class MemoryParser {
   private:
    std::string filename_;
    std::fstream file_;
    uint32_t current_line_count_;
    uint32_t program_counter_;

    void seekToLine(uint32_t lineNumber);
    void writeZeroLines(uint32_t num_to_add);

   public:
    explicit MemoryParser(const std::string& filename);
    ~MemoryParser();
    uint32_t readNextInstruction();                      // For Program Counter Operations
    uint32_t readMemory(uint32_t address);               // For Memory Access
    void writeMemory(uint32_t address, uint32_t value);  // For Memory Access
    void reset();                                        // Reset the file stream to the beginning
    void jumpToInstruction(uint32_t address);            // Jump to instruction at specified address

    // Getters
    std::string getFilename() const { return filename_; }
    uint32_t getFileLineCount() const { return current_line_count_; }
    uint32_t getPC() const { return program_counter_; }
};
