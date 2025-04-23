/**
 * @file stats.h
 * @brief Declares the Stats class, which collects runtime metrics
 *        for a MIPS-lite instruction-level simulator.
 *
 * The Stats class provides counters and tracking mechanisms for:
 * - Instruction categories executed
 * - Registers and memory addresses accessed
 * - Pipeline stalls, clock cycles, and data hazards
 */

#ifndef STATS_H
#define STATS_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "mips_lite_defs.h"

/**
 * @class Stats
 * @brief Collects and exposes runtime statistics for instruction execution.
 *
 * This class is used by the simulator to keep track of various execution metrics,
 * such as instruction mix, register and memory usage, and hazard-related delays.
 */
class Stats {
   public:
    /// Constructs a Stats object and initializes all counters.
    Stats();

    /// Increments the count for a specific instruction category.
    void incrementCategory(mips_lite::InstructionCategory category);

    /// Returns the number of instructions seen for a given category.
    uint32_t getCategoryCount(mips_lite::InstructionCategory category) const;

    /// Returns the total number of instructions recorded across all categories.
    uint32_t totalInstructions() const;

    /// Tracks a register that was accessed during execution.
    void addRegister(uint8_t reg);

    /// Tracks a memory address that was accessed during execution.
    void addMemoryAddress(uint32_t addr);

    /// Returns the set of registers accessed so far.
    const std::unordered_set<uint8_t>& getRegisters() const;

    /// Returns the set of memory addresses accessed so far.
    const std::unordered_set<uint32_t>& getMemoryAddresses() const;

    /// Increments the pipeline stall count.
    void incrementStalls();

    /// Increments the total clock cycle count.
    void incrementClockCycles();

    /// Increments the count of data hazards encountered.
    void incrementDataHazards();

    /// Returns the number of recorded stalls.
    uint32_t getStalls() const;

    /// Returns the total number of clock cycles.
    uint32_t getClockCycles() const;

    /// Returns the number of data hazards recorded.
    uint32_t getDataHazards() const;

    /// Computes the average number of stalls per data hazard.
    float averageStallsPerHazard() const;

   private:
    std::unordered_map<mips_lite::InstructionCategory, uint32_t> instructionCounts;

    // The following 2 attributes (registers and memoryAddresses) use the unordered_set type
    // in order to prevent duplication of values, since we only need to know which registers
    // and memory addresses have ever been changed throughout the run of the program.
    // Tracks all registers that have been accessed by an instruction.
    std::unordered_set<uint8_t> registers;

    // Tracks all memory addresses that have been accessed by an instruction.
    std::unordered_set<uint32_t> memoryAddresses;

    uint32_t stalls;
    uint32_t clockCycles;
    uint32_t dataHazards;
};

#endif  // STATS_H
