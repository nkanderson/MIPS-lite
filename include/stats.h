#ifndef STATS_H
#define STATS_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "mips_lite_defs.h"

class Stats {
   public:
    Stats();

    void incrementCategory(mips_lite::InstructionCategory category);
    uint32_t getCategoryCount(mips_lite::InstructionCategory category) const;
    uint32_t totalInstructions() const;

    void addRegister(uint8_t reg);
    void addMemoryAddress(uint32_t addr);
    const std::unordered_set<uint8_t>& getRegisters() const;
    const std::unordered_set<uint32_t>& getMemoryAddresses() const;

    void incrementStalls();
    void incrementClockCycles();
    void incrementDataHazards();

    uint32_t getStalls() const;
    uint32_t getClockCycles() const;
    uint32_t getDataHazards() const;
    float averageStallsPerHazard() const;

   private:
    std::unordered_map<mips_lite::InstructionCategory, uint32_t> instructionCounts;
    std::unordered_set<uint8_t> registers;
    std::unordered_set<uint32_t> memoryAddresses;

    uint32_t stalls;
    uint32_t clockCycles;
    uint32_t dataHazards;
};

#endif  // STATS_H
