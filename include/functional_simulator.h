#pragma once

#include <array>
#include <cstdint>
#include <memory>  // For smart pointers
#include <optional>

#include "memory_interface.h"
#include "register_file.h"
#include "stats.h"

class Instruction;
/**
 * @struct PipelineStageData
 * @brief Holds all data related to an instruction as it moves through the pipeline.
 *
 * This structure encapsulates an instruction pointer along with all intermediate values
 * and control signals needed as the instruction flows through the pipeline stages.
 */
struct PipelineStageData {
    std::unique_ptr<Instruction> instruction;  ///< Instruction object
    uint32_t pc;           ///< Program counter value when instruction was fetched
                           ///< TODO: I think we need to store PC at the time of the instruction
                           ///< to accurately calculate PC relative addressing
    uint32_t rs_value;     ///< Value read from rs register
    uint32_t rt_value;     ///< Value read from rt register
    int32_t alu_result;    ///< Result of ALU operation or effective address (signed)
    uint32_t memory_data;  ///< Data read from memory (if applicable)
    std::optional<uint8_t> dest_reg;  ///< Destination register (if any)
    uint32_t branch_target;           ///< Target address for branch/jump

    // Constructor to initialize with default values
    PipelineStageData()
        : instruction(nullptr),
          pc(0),
          rs_value(0),
          rt_value(0),
          alu_result(0),
          memory_data(0),
          dest_reg(std::nullopt),
          branch_target(0) {}

    // Constructor with instruction
    explicit PipelineStageData(Instruction* instr, uint32_t program_counter)
        : instruction(std::move(instr)),
          pc(program_counter),
          rs_value(0),
          rt_value(0),
          alu_result(0),
          memory_data(0),
          dest_reg(std::nullopt),
          branch_target(0) {}

    // Check if stage is a bubble (no instruction)
    bool isEmpty() const { return instruction == nullptr; }
    int32_t getRsValueSigned() const { return static_cast<int32_t>(rs_value); }
    int32_t getRtValueSigned() const { return static_cast<int32_t>(rt_value); }
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

    bool isProgramFinished() const { return program_finished; }

    /**
     * @brief Get the stall signal.
     * @return True if the pipeline is stalled, false otherwise.
     */
    bool getStall() const;

    bool isHalted() const { return halt_pipeline; }

    static constexpr int getNumStages() { return NUM_STAGES; }

    /**
     * @brief Get the pipeline stage data at the given pipeline stage.
     * @param stage Index (0 to 4) of the pipeline stage.
     * @return Pointer to the PipelineStageData at that stage, or nullptr if empty.
     * @throws std::out_of_range if the stage index is invalid.
     */
    const PipelineStageData* getPipelineStage(int stage) const;

    bool isBranchTaken() const { return branch_taken; }

    // Setter methods

    /**
     * @brief Set the Program Counter.
     * @param new_pc New value for the PC.
     */
    void setPC(uint32_t new_pc);

    // Pipeline stage methods

    /**
     * @brief Fetch the next instruction.
     */
    void instructionFetch();

    /**
     * @brief Decode by reading registers referenced by the instruction.
     * @param instr Pointer to the instruction to decode.
     */
    void instructionDecode();

    /**
     * @brief Execute by performing ALU operation from the instruction.
     * @param instr Pointer to the instruction to execute.
     */
    void execute();

    /**
     * @brief Perform memory access for the instruction.
     * @param instr Pointer to the instruction.
     */
    void memory();

    /**
     * @brief Write the result back to the register file.
     * @param instr Pointer to the instruction.
     */
    void writeBack();

    /**
     * @brief Advance all pipeline stages by one cycle.
     *
     * This shifts each pipeline stage, moving instructions forward in the pipeline.
     * Should be called at the end of each simulation cycle.
     */
    void advancePipeline();

    /**
     * @brief Perform one complete cycle of the pipeline.
     *
     * This method should handle execution of the active pipeline stages in the correct order. Note
     * that writes must happen before reads
     * 1. Execution of active pipeline stages
     * 2. Advancing the pipeline
     * 3. Maybe handle stalls or flushing unless we want to implement elsewhere
     * 4. Update the program counter
     */
    void cycle();

    /**
     * @brief Check for data hazards and stall if necessary.
     * @return True if a stall is needed, false otherwise.
     */
    bool detectHazards();

    /**
     * @brief Check if instruction at given stage is a bubble.
     * @param stage Index of the pipeline stage to check.
     * @return True if the stage contains no valid instruction.
     */
    bool isStageEmpty(int stage) const;

    /**
     * @brief Enumeration for pipeline stages
     */
    enum PipelineStage {
        FETCH = 0,
        DECODE = 1,
        EXECUTE = 2,
        MEMORY = 3,
        WRITEBACK = 4,
    };

   private:
    /// Program Counter
    static constexpr int NUM_STAGES = 5;
    uint32_t pc;
    bool branch_taken;

    /// General purpose registers
    RegisterFile* register_file;

    /// 5-stage pipeline array
    std::array<std::unique_ptr<PipelineStageData>, NUM_STAGES> pipeline;

    /// Stats instance to track runtime metrics
    Stats* stats;

    /// Serves as the simulator memory
    IMemoryParser* memory_parser;

    /// Whether or not data forwarding is enabled
    bool forward;

    /// Add flag for halt instruction
    bool halt_pipeline = false;

    /// Stall signal
    bool stall = false;

    bool program_finished = false; 

    /**
     * @brief Helper method to check if an instruction writes to a register.
     * @param instr Pointer to the instruction to check.
     * @return True if the instruction writes to a register, false otherwise.
     */
    bool isRegisterWriteInstruction(const Instruction* instr) const;


    /**
     * @brief Helper method to get the value of a register.
     * @param reg_num Register number to read. Handles forwarding if needed. Note that this
     * function should only be called when the pipeline is not stalled. If stalled we shouldn't be 
     * reading any registers.
     * @return Value of the register.
     */
    uint32_t readRegisterValue(uint8_t reg_num);

    /* @brief Detects hazards in the pipeline and returns the number of stall cycles needed.
    * 
    * This method checks for data hazards between the stages of the pipeline. If a hazard is detected,
    * it returns the number of stall cycles required to resolve it. If no hazards are detected, it
    * returns 0.
    *
    * @return Number of stall cycles needed to resolve hazards, or 0 if no hazards are detected.
    */
    bool detectStalls(void);

    // determines if the instruction needs the Rt register value as a source operand
    bool needsRtValue(const Instruction* instr) const;

    /** check for program completion
     * @brief Check if the program has finished executing. If so it will set 
     * program_finished to true 
     */
    void checkProgramCompletion(void);




#ifdef UNIT_TEST
     // Allow functional simulator tests access to private class member pipeline
   public:
    std::array<std::unique_ptr<PipelineStageData>, NUM_STAGES>& getPipeline() { return pipeline; }
#endif

};
