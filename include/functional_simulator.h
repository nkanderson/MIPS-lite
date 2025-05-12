#pragma once

#include <array>
#include <cstdint>

#include "memory_interface.h"
#include "register_file.h"
#include "stats.h"

// Forward declarations of Instruction class
class Instruction;

/**
 * @class FunctionalSimulator
 * @brief Simulates a simplified MIPS pipelined processor with optional data forwarding.
 *
 * This class is constructed using dependency injection to supply instances of
 * RegisterFile, Stats, and MemoryParser. Upon construction, the simulator sets
 * the program counter to 0, disables forwarding by default, clears the pipeline,
 * and resets the stall counter.
 */
class FunctionalSimulator {
   public:
    /**
     * @brief Constructor using dependency injection.
     * @param rf Pointer to the RegisterFile instance.
     * @param st Pointer to the Stats instance.
     * @param mem Pointer to the MemoryParser instance.
     * @param enable_forwarding Optional flag to enable data forwarding.
     */
    FunctionalSimulator(RegisterFile* rf, Stats* st, IMemoryParser* mem,
                        bool enable_forwarding = false);

    // Getter methods

    /**
     * @brief Get the current Program Counter.
     * @return Current PC value.
     */
    uint32_t getPC() const;

    /**
     * @brief Check if forwarding is enabled.
     * @return True if forwarding is enabled.
     */
    bool isForwardingEnabled() const;

    /**
     * @brief Get the current stall count.
     * @return Number of remaining stall cycles.
     */
    uint8_t getStall() const;

    /**
     * @brief Get the instruction pointer at the given pipeline stage.
     * @param stage Index (0 to 4) of the pipeline stage.
     * @return Instruction pointer at that stage, or nullptr if uninitialized.
     */
    Instruction* getPipelineStage(int stage) const;

    // Setter methods

    /**
     * @brief Set the Program Counter.
     * @param new_pc New value for the PC.
     */
    void setPC(uint32_t new_pc);

    /**
     * @brief Set the number of stall cycles.
     * @param cycles Number of stall cycles.
     */
    void setStall(uint8_t cycles);

    // Pipeline stage methods

    /**
     * @brief Fetch the next instruction.
     */
    void instructionFetch();

    /**
     * @brief Decode by reading registers referenced by the instruction.
     * @param instr Pointer to the instruction to decode.
     */
    void instructionDecode(Instruction* instr);

    /**
     * @brief Execute by performing ALU operation from the instruction.
     * @param instr Pointer to the instruction to execute.
     */
    void execute(Instruction* instr);

    /**
     * @brief Perform memory access for the instruction.
     * @param instr Pointer to the instruction.
     */
    void memory(Instruction* instr);

    /**
     * @brief Write the result back to the register file.
     * @param instr Pointer to the instruction.
     */
    void writeBack(Instruction* instr);

   private:
    /// Program Counter
    uint32_t pc;

    /// General purpose registers
    RegisterFile* register_file;

    /// 5-stage pipeline of instruction pointers
    std::array<Instruction*, 5> pipeline;

    /// Stats instance to track runtime metrics
    Stats* stats;

    /// Whether or not data forwarding is enabled
    bool forward;

    /// Countdown of the required stall cycles
    uint8_t stall;

    /// Serves as the simulator memory
    IMemoryParser* memory_parser;
};
