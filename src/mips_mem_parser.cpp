#include "mips_mem_parser.h"

#include <sys/types.h>

#include <cstdint>
#include <iomanip>  // Add this for std::setw and std::setfill
#include <iostream>
#include <sstream>  // Add this for std::stringstream
#include <stdexcept>
#include <string>

// TODO: We need to first copy the file so we aren't writting to a test case. Therefore, we have an
// input and output file

/**
 * @brief MemoryParser: constructor that dynamically calculates the memory size
 * which shouldn't exceed 4KiB (4096 bytes), per project specs.
 * @param filename: The name/relative path to the file to be parsed
 * @throws std::runtime_error if the file cannot be opened
 */
MemoryParser::MemoryParser(const std::string& filename) : filename_(filename) {
    // Open file in read/write mode
    file_.open(filename_, std::ios::in | std::ios::out);
    if (!file_.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename_);
    }
    program_counter_ = 0;  // Initialize program counter

    // Count the current number of lines in file
    uint32_t line_count = 0;
    std::string line;

    // Ensure on first line
    file_.seekg(0, std::ios::beg);

    // Count current lines
    while (std::getline(file_, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (!line.empty()) {
            line_count++;
        }
    }

    // Check if the file exceeds 4KiB
    if (line_count > MAX_LINE_COUNT) {
        throw std::runtime_error("File exceeds maximum memory size of 4KiB");
    }

    // Set the current line count
    current_line_count_ = line_count;

    // Reset file position to the beginning
    file_.clear();  // Clear EOF flag
    file_.seekg(0, std::ios::beg);
}

/**
 * MemoryParser destructor
 */
MemoryParser::~MemoryParser() {
    if (file_.is_open()) {
        file_.close();
    }
}

/**
 * @brief Helper method to seek to a specific line efficiently by using the current line position.
 *          We know that the file cannot exceed 4 KiB but it might start with fewer lines. As a side
 *          effect this method will increase the size of the file up to 4 KiB if the target line
 *          number exceeds the current line count.
 * @param lineNumber: The target line number to seek to
 */
void MemoryParser::seekToLine(uint32_t lineNumber) {
    uint32_t curr_line_offset = PC_TO_LINE(program_counter_);
    if (lineNumber == curr_line_offset) {
        return;
    }

    if (lineNumber >= MAX_LINE_COUNT) {
        throw std::runtime_error("Line number exceeds maximum memory size of 4KiB");
    }
    // Clear any error flags
    file_.clear();
    std::streampos currentPos = file_.tellg();

    std::string line;

    if (lineNumber > curr_line_offset) {
        // Seek forward
        for (uint32_t i = curr_line_offset; i < lineNumber; ++i) {
            if (!std::getline(file_, line)) {
                // If we hit EOF, we need to add zero lines until we reach the target
                if (file_.eof()) {
                    int line_diff = lineNumber - current_line_count_;
                    // Go to end of file to append zero lines
                    file_.clear();
                    file_.seekp(0, std::ios::end);

                    // Add zero lines
                    writeZeroLines(line_diff);
                    // Update current line count
                    current_line_count_ += line_diff;

                    // Reset to the beginning and move to the target line
                    file_.clear();
                    file_.seekg(0, std::ios::beg);
                    // Position the cursor right before target line
                    for (uint32_t k = 0; k < lineNumber - 1; ++k) {
                        if (!std::getline(file_, line)) {
                            throw std::runtime_error("Failed to seek to newly added line: " +
                                                     std::to_string(lineNumber));
                        }
                    }
                    return;
                } else {
                    // Some other error occurred
                    file_.clear();
                    file_.seekg(currentPos);
                    throw std::runtime_error("Failed to seek to line: " +
                                             std::to_string(lineNumber));
                }
            }
        }
    } else {
        // Target seek is less than current line
        file_.seekg(0, std::ios::beg);
        for (uint32_t i = 0; i < lineNumber; ++i) {
            if (!std::getline(file_, line)) {
                file_.clear();
                file_.seekg(currentPos);
                throw std::runtime_error("Failed to seek to line: " + std::to_string(lineNumber));
            }
        }
    }
}

/**
 * @brief Helper function expand the file and fill with zeros
 */
void inline MemoryParser::writeZeroLines(uint32_t num_to_add) {
    for (uint32_t i = 0; i < num_to_add; ++i) {
        file_ << "00000000" << std::endl;
    }

    file_.flush();
}

/**
 * @brief Reads the next line from the file and converts it to a 32-bit instruction
 * @return The 32-bit instruction read from the file
 * @throws std::runtime_error if file reading fails or end of file is reached
 */
uint32_t MemoryParser::readNextInstruction() {
    std::string line;

    if (!file_.is_open()) {
        throw std::runtime_error("File is not open");
    }

    /* The getline function automatically advances the file cursor to the next line.
     * This means each call to this method reads the next line in sequence,
     * similar to how a program counter advances in a processor.
     * The file cursor's position is maintained by the ifstream object (file_)
     * between function calls.
     */
    if (!std::getline(file_, line)) {
        // Check if we reached end of file
        if (file_.eof()) {
            throw std::runtime_error("End of file reached");
        } else {
            throw std::runtime_error("Error reading file");
        }
    }

    // Trim leading and trailing whitespace
    line.erase(0, line.find_first_not_of(" \t\r\n"));  // Leading whitespace
    line.erase(line.find_last_not_of(" \t\r\n") + 1);  // Trailing whitespace

    if (line.empty()) {
        throw std::runtime_error("Empty line encountered in middle of program");
    }

    uint32_t instruction;

    // Convert a line of characters to hex and store it in ss
    std::stringstream ss;
    ss << std::hex << line;
    // Extract the instruction from the stream
    if (!(ss >> instruction)) {
        throw std::runtime_error("Failed to parse instruction: " + line);
    }

    program_counter_ += 4;

    return instruction;
}

/**
 * @brief Read a 32-bit value from a specific memory address, then returns file position to
            original spot.
 * @param address Memory address to read from
 * @return The 32-bit value at the specified address
 * @throws std::runtime_error if address is invalid or file access fails
 */
uint32_t MemoryParser::readMemory(uint32_t address) {
    if (address % 4 != 0) {
        throw std::runtime_error("Unaligned memory access: " + std::to_string(address));
    }
    if (address >= MAX_LINE_COUNT * 4) {
        throw std::runtime_error("Memory address out of bounds: " + std::to_string(address));
    }

    // Calculate line number (divide by 4 since each address increments by 4)
    uint32_t lineNumber = address / 4;

    // Save current position for program counter
    std::streampos currentPos = file_.tellg();

    seekToLine(lineNumber);

    std::string line;
    try {
        std::getline(file_, line);
    } catch (...) {
        // Restore position and throw
        std::cerr << "DEBUG: After getline - EOF: " << file_.eof() << ", FAIL: " << file_.fail()
                  << ", BAD: " << file_.bad() << std::endl;
        file_.clear();
        file_.seekg(currentPos);
        throw std::runtime_error("Failed to read memory at address: " + std::to_string(address));
    }

    // Trim any whitespace that might have been added
    line.erase(0, line.find_first_not_of(" \t\r\n"));
    line.erase(line.find_last_not_of(" \t\r\n") + 1);

    // Convert hex string to uint32_t
    uint32_t value = 0;
    std::stringstream ss;
    ss << std::hex << line;
    if (!(ss >> value)) {
        // Restore position and throw
        file_.clear();
        file_.seekg(currentPos);
        throw std::runtime_error("Failed to parse memory at address: " + std::to_string(address));
    }

    // Restore original position
    file_.clear();
    file_.seekg(currentPos);

    return value;
}

/**
 * @brief Write a 32-bit value to a specific memory address, note that writing is much more
 expensive then reading.
 * @param address Memory address to write to
 * @param value The 32-bit value to write
 * @throws std::runtime_error if address is invalid or file access fails
 */
void MemoryParser::writeMemory(uint32_t address, uint32_t value) {
    // Checks for alignment and bounds
    if (address % 4 != 0) {
        throw std::runtime_error("Unaligned memory access: " + std::to_string(address));
    }
    if (address > MAX_LINE_COUNT * 4) {
        throw std::runtime_error("Memory address out of bounds: " + std::to_string(address));
    }

    // Save current position
    std::streampos currentPos = file_.tellg();

    // Format the value as an 8-digit hex string
    std::stringstream ss;
    ss << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << value;
    std::string hexValue = ss.str();

    // Go to the target line
    seekToLine(address / 4);

    // Need to increase the curser to next line

    // Overwrite the line
    file_ << hexValue << std::endl;

    // Restore position
    file_.clear();
    try {
        file_.seekg(currentPos);
    } catch (...) {
        throw std::runtime_error("Failed to restore file position after write");
    }
}

/**
 * @brief Moves the file cursor to the instruction at the specified address, which
            would be used in the case of a jump or branch
 * @param address The memory address to jump to
 * @throws std::runtime_error if address is invalid or file access fails
 */
void MemoryParser::jumpToInstruction(uint32_t address) {
    // Check if address is aligned to 4 bytes (instructions are 4 bytes each)
    if (address % 4 != 0) {
        throw std::runtime_error("Unaligned instruction address: " + std::to_string(address));
    }
    if (address > MAX_MEMORY_SIZE) {
        throw std::runtime_error("Instruction address out of bounds: " + std::to_string(address));
    }

    uint32_t lineNumber = address / 4;

    seekToLine(lineNumber);
    // Now the file cursor is positioned at the desired instruction
    // The next call to readNextInstruction() will read this instruction
    program_counter_ = address;
}

void MemoryParser::reset() {
    if (file_.is_open()) {
        file_.clear();                  // Clear EOF flag
        file_.seekg(0, std::ios::beg);  // Reset to the beginning of the file
    }

    program_counter_ = 0;  // Reset program counter
}
