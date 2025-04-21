#include "mips_mem_parser.h"

#include <sys/types.h>

#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

/**
 * @brief MemoryParser: constructor that reads the entire file into a vector
 * @param input_filename: The name/relative path to the input file to be parsed
 * @param output_filename: The name/relative path to the output file (optional)
 * @throws std::runtime_error if the file cannot be opened
 */
MemoryParser::MemoryParser(const std::string& input_filename, const std::string& output_filename)
    : input_filename_(input_filename),
      output_filename_(output_filename.empty() ? input_filename + ".out" : output_filename) {
    // Open input file in read mode
    std::ifstream file(input_filename_);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open input file: " + input_filename_);
    }

    // Read file content into memory_content_ vector
    std::string line;
    while (std::getline(file, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (!line.empty()) {
            // Convert hex string to uint32_t
            uint32_t value = 0;
            std::stringstream ss;
            ss << std::hex << line;
            if (!(ss >> value)) {
                throw std::runtime_error("Failed to parse instruction: " + line);
            }
            memory_content_.push_back(value);
        }
    }

    if (memory_content_.size() > MAX_VEC_SIZE) {
        throw std::runtime_error("File exceeds maximum memory size of 4KiB");
    }
    modified_ = false;
    write_file_on_modified_ = true;

    file.close();
}

/**
 * MemoryParser destructor
 * Writes back to output file if content was modified and write_file_on_modified_ is true
 */
MemoryParser::~MemoryParser() {
    if (modified_ && write_file_on_modified_) {
        writeToFile();
    }
}

/**
 * @brief Ensures that the vector has enough space for the requested index by resizing vec if
 *          necessary
 * @param index: the index that will be requested
 */
void MemoryParser::ensureIndexExists(uint32_t index) {
    if (index >= MAX_VEC_SIZE) {
        throw std::runtime_error("Line number exceeds maximum memory size of 4KiB");
    }

    // Expand the vector if necessary by adding zeros
    if (index >= memory_content_.size()) {
        memory_content_.resize(index + 1, 0);
        current_line_count_ = memory_content_.size();
        modified_ = true;
    }
}

/**
 * @brief Writes memory content to an output file
 */
void MemoryParser::writeToFile() {
    std::ofstream file(output_filename_);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open output file for writing: " + output_filename_);
    }

    // Write each memory line as an 8-digit hex string
    for (const auto& value : memory_content_) {
        std::stringstream ss;
        ss << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << value;
        file << ss.str() << std::endl;
    }

    file.close();
    modified_ = false;  // Reset modified flag
}

/**
 * @brief Reads the next instruction from memory and advances the program counter
 * @return uint32_t instruction read from memory
 */
uint32_t MemoryParser::readInstruction(uint32_t address) {
    if (address % 4 != 0) {
        throw std::runtime_error("Unaligned memory access: " + std::to_string(address));
    }

    uint32_t index = ADDR_TO_INDEX(address);

    if (index >= memory_content_.size()) {
        throw std::runtime_error("Invalid instruction address: " + std::to_string(address));
    } else {
        return memory_content_[index];
    }
}

/**
 * @brief Read a 32-bit value from a specific memory address
 * @param address Memory address to read from
 * @return The 32-bit value at the specified address
 */
uint32_t MemoryParser::readMemory(uint32_t address) {
    if (address % 4 != 0) {
        throw std::runtime_error("Unaligned memory access: " + std::to_string(address));
    }
    if (address >= MAX_MEMORY_SIZE) {
        throw std::runtime_error("Memory address out of bounds: " + std::to_string(address));
    }

    uint32_t index = ADDR_TO_INDEX(address);
    ensureIndexExists(index);

    return memory_content_[index];
}

/**
 * @brief Write a 32-bit value to a specific memory address
 * @param address Memory address to write to
 * @param value The 32-bit value to write
 * @throws std::runtime_error if address is invalid
 */
void MemoryParser::writeMemory(uint32_t address, uint32_t value) {
    if (address % 4 != 0) {
        throw std::runtime_error("Unaligned memory access: " + std::to_string(address));
    }
    if (address >= MAX_MEMORY_SIZE) {
        throw std::runtime_error("Memory address out of bounds: " + std::to_string(address));
    }

    uint32_t index = ADDR_TO_INDEX(address);
    ensureIndexExists(index);

    memory_content_[index] = value;
    modified_ = true;  // Mark as modified
}

void MemoryParser::printMemoryContent() {
    std::cout << "Memory Content: Vec Index (dec)   :   Hex Address   :   Hex Value   "
              << std::endl;
    for (size_t i = 0; i < memory_content_.size(); ++i) {
        // Print each memory line as an 8-digit hex string
        std::cout << std::dec << i << " : " << "0x" << std::setw(8) << std::setfill('0') << std::hex
                  << INDEX_TO_ADDR(i) << ": 0x" << std::setw(8) << std::setfill('0')
                  << memory_content_[i] << std::endl;
    }
}
