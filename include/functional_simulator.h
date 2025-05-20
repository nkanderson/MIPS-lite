#pragma once

#include <array>
#include <cstdint>
#include <optional>

#include "memory_interface.h"
#include "reg_type.h"
#include "register_file.h"
#include "stats.h"

// Forward declarations of Instruction class
class Instruction;

/**
 * @struct PipelineData
 * @brief Holds intermediate pipeline data passed between stages.
 *
 * This includes:
 * - `result`: an intermediate value (e.g., ALU result or memory load)
 * - `wb_reg`: the destination register number to write to in the WB stage, if any
 */
template <typename T>
struct PipelineData {
    T result;                       ///< Intermediate result to be forwarded or written
    std::optional<uint8_t> wb_reg;  ///< Destination register number (std::nullopt if N/A)
};

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

    /**
     * @brief Advance all pipeline registers by one cycle.
     *
     * This clocks each pipeline register, promoting its next value to current.
     * Should be called at the end of each simulation cycle.
     */
    void clockPipelineRegisters();

    /**
     * @brief Pipeline registers between each stage. Each register holds intermediate
     *        values and target write-back registers.
     *
     * These are intended to model the flow of decoded operand values and intermediate
     * results between pipeline stages, enabling inspection or forwarding by register number.
     */
    reg<PipelineData<uint32_t> > ifid_reg;   ///< Between instruction fetch and decode
    reg<PipelineData<uint32_t> > idex_reg;   ///< Between decode and execute
    reg<PipelineData<uint32_t> > exmem_reg;  ///< Between execute and memory
    reg<PipelineData<uint32_t> > memwb_reg;  ///< Between memory and writeback

   private:
    /// Program Counter
    uint32_t pc;

    /// General purpose registers
    RegisterFile* register_file;

    /// 5-stage pipeline of instruction pointers
    std::array<Instruction*, 5> pipeline;

    /// Stats instance to track runtime metrics
    Stats* stats;

    /// Serves as the simulator memory
    IMemoryParser* memory_parser;

    /// Whether or not data forwarding is enabled
    bool forward;

    /// Countdown of the required stall cycles
    uint8_t stall;
};
