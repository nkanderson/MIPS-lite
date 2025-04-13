#include "memory_parser.h"

/**
 * @brief MemoryParser: constructor that dynamically calculates the memory size
 * which shouldn't exceed 4KiB (4096 bytes), per project specs.
 * @param filename: The name/relative path to the file to be parsed
 * @throws std::runtime_error if the file cannot be opened
 */
MemoryParser::MemoryParser(const std::string& filename)
    : filename_(filename), file_size_(0), is_file_loaded_(false) {
    // Open file in text mode
    file_.open(filename_);
    if (!file_.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename_);
    }

    // Count the number of lines in the file
    size_t line_count = 0;
    std::string line;

    // Save the current position
    std::streampos start_pos = file_.tellg();

    // Count non-empty lines
    while (std::getline(file_, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (!line.empty()) {
            line_count++;
        }
    }

    // Calculate memory size in bytes (4 bytes per instruction)
    memory_size_ = line_count * 4;

    // Reset file position to the beginning
    file_.clear();  // Clear EOF flag
    file_.seekg(start_pos);

    // Clear instruction buffer
    instruction_buffer_.clear();
    memory_buffer_.clear();
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

    // Skip empty lines
    if (line.empty()) {
        return readNextInstruction();  // Recursively try the next line
    }

    uint32_t instruction;

    // Convert a line of characters to hex and store it in ss
    std::stringstream ss;
    ss << std::hex << line;
    // Extract the instruction from the stream
    if (!(ss >> instruction)) {
        throw std::runtime_error("Failed to parse instruction: " + line);
    }

    return instruction;
}

/**
 * @brief Read a 32-bit value from a specific memory address
 * @param address Memory address to read from
 * @return The 32-bit value at the specified address
 * @throws std::runtime_error if address is invalid or file access fails
 */
uint32_t MemoryParser::readMemory(uint32_t address) {
    // Check if address is aligned to 4 bytes
    if (address % 4 != 0) {
        throw std::runtime_error("Unaligned memory access: " + std::to_string(address));
    }
    if (address >= memory_size_) {
        throw std::runtime_error("Memory address out of bounds: " + std::to_string(address));
    }

    // Calculate line number (divide by 4 since each address increments by 4)
    size_t lineNumber = address / 4;

    // Save current position for program counter
    std::streampos currentPos = file_.tellg();

    // Go to beginning of file
    file_.clear();  // Clear any error flags
    file_.seekg(0, std::ios::beg);

    // Skip to the desired line
    std::string line;
    for (size_t i = 0; i < lineNumber; ++i) {
        if (!std::getline(file_, line)) {
            // Restore position and throw
            file_.clear();
            file_.seekg(currentPos);
            throw std::runtime_error("Failed to reach memory address: " + std::to_string(address));
        }
    }

    if (!std::getline(file_, line)) {
        // Restore position and throw
        file_.clear();
        file_.seekg(currentPos);
        throw std::runtime_error("Failed to read memory at address: " + std::to_string(address));
    }

    // Convert hex string to uint32_t
    uint32_t value = 0;
    std::stringstream ss(line);
    ss >> std::hex >> value;

    if (ss.fail()) {
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
 * @brief Write a 32-bit value to a specific memory address
 * @param address Memory address to write to
 * @param value The 32-bit value to write
 * @throws std::runtime_error if address is invalid or file access fails
 */
void MemoryParser::writeMemory(uint32_t address, uint32_t value) {
    // Check if address is aligned to 4 bytes
    if (address % 4 != 0) {
        throw std::runtime_error("Unaligned memory access: " + std::to_string(address));
    }
    if (address >= memory_size_) {
        throw std::runtime_error("Memory address out of bounds: " + std::to_string(address));
    }

    // Calculate line number (divide by 4 since each address increments by 4)
    size_t lineNumber = address / 4;

    // Check if address is within range
    if (lineNumber >= 1024) {
        throw std::runtime_error("Memory address out of bounds: " + std::to_string(address));
    }

    // We need to read all lines into a buffer, modify the target line,
    // and then write everything back to the file
    std::vector<std::string> lines;
    std::string line;

    // Save current position
    std::streampos currentPos = file_.tellg();

    // Go to beginning of file
    file_.clear();
    file_.seekg(0, std::ios::beg);

    // Read all lines
    while (std::getline(file_, line)) {
        lines.push_back(line);
    }

    // Check if we have enough lines
    if (lineNumber >= lines.size()) {
        // Need to add lines until we reach the desired address
        while (lines.size() <= lineNumber) {
            lines.push_back("00000000");  // Add empty (zero) instructions
        }
    }

    // Convert the value to a hex string with 8 digits
    std::stringstream ss;
    ss << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << value;

    // Update the line
    lines[lineNumber] = ss.str();

    // Reopen the file for writing
    file_.close();
    std::ofstream outFile(filename_);

    if (!outFile.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + filename_);
    }

    // Write all lines back to the file
    for (const auto& l : lines) {
        outFile << l << std::endl;
    }

    outFile.close();

    // Reopen the file for reading
    file_.open(filename_);

    if (!file_.is_open()) {
        throw std::runtime_error("Failed to reopen file after writing: " + filename_);
    }

    // Restore original position if possible
    if (currentPos <= lines.size()) {
        file_.clear();
        file_.seekg(currentPos);
    }
}

void MemoryParser::reset() {
    if (file_.is_open()) {
        file_.clear();                  // Clear EOF flag
        file_.seekg(0, std::ios::beg);  // Reset to the beginning of the file
    }
}
