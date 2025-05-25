#include <iostream>
#include <string>
#include <stdexcept>
#include <unordered_set>
#include <set>
#include <filesystem>

// Program Libraries
#include "functional_simulator.h"
#include "stats.h"
#include "mips_mem_parser.h"
#include "register_file.h"
#include "mips_lite_defs.h"
#include "mips_instruction.h"


/**
 * @brief main: main program for MIPS-lite simulator
 * @param -i: The filepath to the input trace file
 * @param -o: Filename for output memory trace, if passed, memory contents get saved to the file
 * @param -m: Enables printing of memory content to stdout
 * @param -t: Enables printing of timing information for functional simulator
 * @param -f: Enables forwarding for functional simulator
 * @throws std::invalid_arguement if program is passed invalid values
 */
int main (int argc, char* argv[]) {

    std::string input_tracename_, output_tracename_;

    // Default settings for no args
    input_tracename_ = "traces/hex/randomtrace.txt";
    output_tracename_ = "output/traceout.txt";
    bool time_info_ = false;
    bool forward_ = false;
    bool enable_mem_save_ = false;
    bool enable_mem_print_ = false;

    // Parse Input Arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-i") {
            // Check if next arg exists and check if next arg is not an flag
            if (i + 1 >= argc) {
                throw std::invalid_argument("Output filepath must be provided after -i argument.");
            } else if (argv[i+1][0] == '-') {
                throw std::invalid_argument("Missing filepath after -i argument.");
            }

            input_tracename_ = argv[i+1];   // Saves input filepath into inFile

            // Verify that path to file exists
            if (!std::filesystem::exists(input_tracename_)) {
                throw std::invalid_argument("Input file \"" + input_tracename_ +"\" does not exists.");
            }

            i++;                            // Skips arg with filepath
        } else if (arg == "-o") {
            // Check if next arg exists and check if next arg is not an flag
            if (i + 1 >= argc) {
                throw std::invalid_argument("Output filepath must be provided after -o argument.");
            } else if (argv[i+1][0] == '-') {
                throw std::invalid_argument("Missing filepath after -o argument.");
            }
            output_tracename_ = argv[i+1];  // Saves output filepath into outFile
            enable_mem_save_ = true;        // Enable memory save to file
            i++;                            // Skips arg with filepath
        } else if (arg == "-m") {
            enable_mem_print_ = true;       // Enable memory print to stdout
        } else if (arg == "-t") {
            time_info_ = true;              // Enable printing of timing information
        } else if (arg == "-f") {
            forward_ = true;                // Enable forwarding for functional simulator
        } else {
            throw std::invalid_argument("Argument \"" + arg + "\" to program is invalid, try again.");
        }
    }

    // Print Current Settings to stdout
    #ifdef DEBUG_MODE
    std::cout << "Current Settings: " << "\n";
    std::cout << "\t Input Filepath:\t" << input_tracename_ << "\n";
    std::cout << "\t Output Filepath:\t" << output_tracename_ << "\n";
    std::cout << "\t Print Memory Contents:\t" << (enable_mem_print_ ? "ENABLED" : "DISABLED") << "\n";
    std::cout << "\t Print Timing Info:\t" << (time_info_ ? "ENABLED" : "DISABLED") << "\n";
    std::cout << "\t Forwarding:\t\t" << (forward_ ? "ENABLED" : "DISABLED") << "\n";
    #endif

    // Create Stats, Register File, and Memory Parser class instance
    Stats stats;
    RegisterFile rf;
    MemoryParser mp(input_tracename_);

    // Pass to Functional Simulator
    FunctionalSimulator fs(&rf, &stats, &mp, forward_);

    // If memory save is enabled
    if (enable_mem_save_) {
        mp.setOutputFilename(output_tracename_);
        mp.setOutputFileOnModified(true);
    }

    // If printing to stdout is desired
    if (enable_mem_print_) {
        mp.printMemoryContent();
    }

    // Print Instruction Counts
    std::cout << "\nInstruction Counts:\n\n";
    std::cout << "\tTotal number of instructions:\t" << stats.totalInstructions() << "\n";
    std::cout << "\tArithmetic instructions:\t" << stats.getCategoryCount(mips_lite::InstructionCategory::ARITHMETIC) << "\n";
    std::cout << "\tLogical instructions:\t\t" << stats.getCategoryCount(mips_lite::InstructionCategory::LOGICAL) << "\n";
    std::cout << "\tMemory Access instructions:\t" << stats.getCategoryCount(mips_lite::InstructionCategory::MEMORY_ACCESS) << "\n";
    std::cout << "\tControl Flow instructions:\t" << stats.getCategoryCount(mips_lite::InstructionCategory::CONTROL_FLOW) << "\n";

    // Print Final Register State
    std::set<uint8_t> final_registers_(stats.getRegisters().begin(), stats.getRegisters().end());

    std::cout << "\nFinal Register State:\n\n";

    // Print Program Counter
    std::cout << "\tProgram Counter:\t" << fs.getPC() << "\n";

    // Print registers that have been used
    for (auto item = final_registers_.begin(); item != final_registers_.end(); item++) {
        std::cout << "\tR" << *item << ": " << rf.read(*item) << "\n";
    }

    // Print Total Number of Stalls
    if (time_info_) {
        std::cout << "\tTotal Stalls:\t" << stats.getStalls() << "\n";
    }

    // Print Final Memory State
    std::set<uint8_t> final_memory_(stats.getMemoryAddresses().begin(), stats.getMemoryAddresses().end());

    // Print memory locations and values that have been accessed
    for (auto item = final_memory_.begin(); item != final_memory_.end(); item++) {
        std::cout << "\tAddress: " << *item << ", Contents: " << mp.readMemory(*item) << "\n";
    }

    // Print timing info if enabled
    if (time_info_) {
        std::cout << "\nTiming Simulator:\n\n";
        std::cout << "\tTotal number of clock cycles: " << stats.getClockCycles() << "\n";
    } 

    return 0;
}