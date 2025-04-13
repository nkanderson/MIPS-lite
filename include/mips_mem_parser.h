#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

class MemoryParser {
   private:
    std::string filename_;
    std::ifstream file_;
    size_t memory_size_;  // Size of memory file in bytes

   public:
    explicit MemoryParser(const std::string& filename);
    ~MemoryParser();
    uint32_t readNextInstruction();                      // For Program Counter Operations
    uint32_t readMemory(uint32_t address);               // For Memory Access
    void writeMemory(uint32_t address, uint32_t value);  // For Memory Access
    void reset();                                        // Reset the file stream to the beginning
};
