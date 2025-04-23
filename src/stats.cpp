/**
 * @file stats.cpp
 * @brief Implements the Stats class used to track runtime metrics
 *        for a MIPS-lite instruction-level simulator.
 *
 * This includes instruction category counts, registers accessed,
 * memory addresses used, and performance-related metrics such as
 * stalls, clock cycles, and data hazards.
 */

#include "stats.h"

#include "mips_lite_defs.h"

using mips_lite::InstructionCategory;

// Constructor: Initializes all counters and sets up default categories.
Stats::Stats() : stalls(0), clockCycles(0), dataHazards(0) {
    // Initialize all instruction categories with 0 count
    for (InstructionCategory category :
         {InstructionCategory::ARITHMETIC, InstructionCategory::LOGICAL,
          InstructionCategory::MEMORY_ACCESS, InstructionCategory::CONTROL_FLOW}) {
        instructionCounts[category] = 0;
    }
}

void Stats::incrementCategory(InstructionCategory category) { instructionCounts[category]++; }

uint32_t Stats::getCategoryCount(InstructionCategory category) const {
    auto inst = instructionCounts.find(category);
    return (inst != instructionCounts.end()) ? inst->second : 0;
}

/**
 * @brief Returns the total number of instructions recorded across all categories.
 */
uint32_t Stats::totalInstructions() const {
    uint32_t total = 0;
    for (const auto& pair : instructionCounts) {
        total += pair.second;
    }
    return total;
}

/**
 * @brief Tracks a register that was accessed by an instruction.
 *
 * @param reg The register number (0-31).
 */
void Stats::addRegister(uint8_t reg) { registers.insert(reg); }

/**
 * @brief Tracks a memory address accessed by an instruction.
 *
 * @param addr The 32-bit memory address.
 */
void Stats::addMemoryAddress(uint32_t addr) { memoryAddresses.insert(addr); }

const std::unordered_set<uint8_t>& Stats::getRegisters() const { return registers; }

const std::unordered_set<uint32_t>& Stats::getMemoryAddresses() const { return memoryAddresses; }

void Stats::incrementStalls() { stalls++; }

void Stats::incrementClockCycles() { clockCycles++; }

void Stats::incrementDataHazards() { dataHazards++; }

uint32_t Stats::getStalls() const { return stalls; }

uint32_t Stats::getClockCycles() const { return clockCycles; }

uint32_t Stats::getDataHazards() const { return dataHazards; }

/**
 * @brief Computes the average number of stalls per data hazard.
 *
 * @return 0.0 if there are no data hazards; otherwise, stalls / hazards.
 */
float Stats::averageStallsPerHazard() const {
    if (dataHazards == 0) {
        return 0.0f;
    }
    return static_cast<float>(stalls) / dataHazards;
}
