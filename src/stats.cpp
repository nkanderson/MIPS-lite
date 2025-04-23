#include "stats.h"

#include "mips_lite_defs.h"

using mips_lite::InstructionCategory;

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

uint32_t Stats::totalInstructions() const {
    uint32_t total = 0;
    for (const auto& pair : instructionCounts) {
        total += pair.second;
    }
    return total;
}

void Stats::addRegister(uint8_t reg) { registers.insert(reg); }

void Stats::addMemoryAddress(uint32_t addr) { memoryAddresses.insert(addr); }

const std::unordered_set<uint8_t>& Stats::getRegisters() const { return registers; }

const std::unordered_set<uint32_t>& Stats::getMemoryAddresses() const { return memoryAddresses; }

void Stats::incrementStalls() { stalls++; }

void Stats::incrementClockCycles() { clockCycles++; }

void Stats::incrementDataHazards() { dataHazards++; }

uint32_t Stats::getStalls() const { return stalls; }

uint32_t Stats::getClockCycles() const { return clockCycles; }

uint32_t Stats::getDataHazards() const { return dataHazards; }

float Stats::averageStallsPerHazard() const {
    if (dataHazards == 0) {
        return 0.0f;
    }
    return static_cast<float>(stalls) / dataHazards;
}
