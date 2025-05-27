#include "functional_simulator.h"

#include <sys/types.h>

#include <cstdint>
#include <optional>

#include "iostream"
#include "memory_interface.h"
#include "mips_instruction.h"
#include "mips_lite_defs.h"
#include "register_file.h"
#include "stats.h"

FunctionalSimulator::FunctionalSimulator(RegisterFile* rf, Stats* st, IMemoryParser* mem,
                                         bool enable_forwarding)
    : pc(0), forward(enable_forwarding) {
    // Validate dependencies
    if (!rf) {
        throw std::invalid_argument("RegisterFile instance cannot be null");
    }
    if (!st) {
        throw std::invalid_argument("Stats instance cannot be null");
    }
    if (!mem) {
        throw std::invalid_argument("MemoryParser instance cannot be null");
    }
    register_file = rf;
    stats = st;
    memory_parser = mem;

    for (auto& stage : pipeline) {
        stage = nullptr;  // Initialize all pipeline stages to nullptr
    }
}

uint32_t FunctionalSimulator::getPC() const { return pc; }

bool FunctionalSimulator::isForwardingEnabled() const { return forward; }

void FunctionalSimulator::setPC(uint32_t new_pc) { pc = new_pc; }

const PipelineStageData* FunctionalSimulator::getPipelineStage(int stage) const {
    if (stage < 0 || stage >= NUM_STAGES) {
        throw std::out_of_range("Pipeline stage index out of range");
    }
    return pipeline[stage].get();  // Use .get() to return the raw pointer
}

bool FunctionalSimulator::isStageEmpty(int stage) const {
    if (stage < 0 || stage >= NUM_STAGES) {
        throw std::out_of_range("Pipeline stage index out of range");
    }
    return pipeline[stage] == nullptr;
}

void FunctionalSimulator::instructionFetch() {
    if (!isStageEmpty(PipelineStage::FETCH) || halt_pipeline) {
        // Fetch stage already has an instruction, do nothing
        // or if halt_pipeline is true, we should not fetch a new instruction
        return;
    }
    uint32_t instruction_word = memory_parser->readInstruction(pc);
    if (mips_lite::is_halt_instruction(instruction_word)) {
        // If the instruction is HALT, set the halt flag
        halt_pipeline = true;
        // Still create instruction and put in pipeline to finish instructions already
        // in the pipeline
    }
    // Object creation
    auto instruction = std::make_unique<Instruction>(instruction_word);
    auto fetch_data = std::make_unique<PipelineStageData>();
    fetch_data->instruction = std::move(instruction);
    fetch_data->pc = pc;  // Store the current PC in the fetch stage data
    // Place in pipeline
    pipeline[PipelineStage::FETCH] = std::move(fetch_data);
    // Increment PC for next instruction
    if (!halt_pipeline) {
        pc += 4;
    }
}

void FunctionalSimulator::instructionDecode() {
    // Operation: Decode instruction and access register file to read the register sources

    PipelineStageData* id_data = pipeline[PipelineStage::DECODE].get();

    if (id_data == nullptr || stall) {
        // No instruction to decode
        return;
    }
    // Track category of instructions
    // TODO: Does this make sense to keep here or should it be in another place?
    mips_lite::InstructionCategory category =
        mips_lite::get_instruction_category(id_data->instruction->getOpcode());
    stats->incrementCategory(category);

    uint8_t rs = id_data->instruction->getRs();
    uint8_t rt = id_data->instruction->getRt();

    id_data->rs_value = readRegisterValue(rs);

    // Determine register Destination & optional second source register
    if (id_data->instruction->hasRd()) {
        id_data->dest_reg = id_data->instruction->getRd();
        // R-type instruction has two source registers, get second source register
        id_data->rt_value = readRegisterValue(rt);
    } else if (!id_data->instruction->hasRd() &&
               isRegisterWriteInstruction(id_data->instruction.get())) {
        // Destination register is rt
        id_data->dest_reg = id_data->instruction->getRt();
    } else {
        // No destination register
        id_data->dest_reg = std::nullopt;
        if (id_data->instruction->getOpcode() == mips_lite::opcode::BEQ) {
            // BEQ has two source registers
            id_data->rt_value = readRegisterValue(rt);
        }
    }
}

void FunctionalSimulator::execute() {
    if (isStageEmpty(PipelineStage::EXECUTE)) {
        // No instruction to execute
        return;
    }
    PipelineStageData* ex_data = pipeline[PipelineStage::EXECUTE].get();

    switch (ex_data->instruction->getOpcode()) {
        // Arithmetic Operations
        // Operands are always signed
        case mips_lite::opcode::ADD:
            ex_data->alu_result = ex_data->getRsValueSigned() + ex_data->getRtValueSigned();
            break;
        case mips_lite::opcode::ADDI:
            ex_data->alu_result =
                ex_data->getRsValueSigned() + ex_data->instruction->getImmediate();
            break;

        case mips_lite::opcode::SUB:
            ex_data->alu_result = ex_data->getRsValueSigned() - ex_data->getRtValueSigned();
            break;
        case mips_lite::opcode::SUBI:
            ex_data->alu_result =
                ex_data->getRsValueSigned() - ex_data->instruction->getImmediate();
            break;

        case mips_lite::opcode::MUL:
            ex_data->alu_result = ex_data->getRsValueSigned() * ex_data->getRtValueSigned();
            break;
        case mips_lite::opcode::MULI:
            ex_data->alu_result =
                ex_data->getRsValueSigned() * ex_data->instruction->getImmediate();
            break;

        // Logical Operations
        // Sign of operands doesn't matter
        case mips_lite::opcode::OR:
            ex_data->alu_result = ex_data->rs_value | ex_data->rt_value;
            break;
        case mips_lite::opcode::ORI:
            ex_data->alu_result = ex_data->rs_value | ex_data->instruction->getImmediate();
            break;

        case mips_lite::opcode::AND:
            ex_data->alu_result = ex_data->rs_value & ex_data->rt_value;
            break;
        case mips_lite::opcode::ANDI:
            ex_data->alu_result = ex_data->rs_value & ex_data->instruction->getImmediate();
            break;

        case mips_lite::opcode::XOR:
            ex_data->alu_result = ex_data->rs_value ^ ex_data->rt_value;
            break;
        case mips_lite::opcode::XORI:
            ex_data->alu_result = ex_data->rs_value ^ ex_data->instruction->getImmediate();
            break;

        // Memory Effective Address Calculation -> Load and Store
        // Effective Address Calculation is signed
        case mips_lite::opcode::LDW:
            // Load word - calculate effective address
            ex_data->alu_result =
                ex_data->getRsValueSigned() + ex_data->instruction->getImmediate();
            break;

        case mips_lite::opcode::STW:
            ex_data->alu_result =
                ex_data->getRsValueSigned() + ex_data->instruction->getImmediate();
            break;

        // Control Flow Ops
        case mips_lite::opcode::BZ:
            if (ex_data->rs_value == 0) {
                ex_data->alu_result = ex_data->pc + (ex_data->instruction->getImmediate() * 4);
                branch_taken = true;  // Indicate that a branch was taken
                                      // TODO: Controller will need to clear this flag after the
                                      // branch is taken
                break;
            }
            branch_taken = false;               // No branch taken
            ex_data->alu_result = ex_data->pc;  // No change (PC should be updated in fetch stage)
            break;

        case mips_lite::opcode::BEQ:
            if (ex_data->rs_value == ex_data->rt_value) {
                // Branch if equal - calculate target address

                ex_data->alu_result = ex_data->pc + (ex_data->instruction->getImmediate() * 4);
                branch_taken = true;  // Indicate that a branch was taken
            } else {
                // No branch, PC will be updated normally
                branch_taken = false;  // No branch taken
                ex_data->alu_result =
                    ex_data->pc;  // No change (PC should be updated in fetch stage)
            }
            break;

        case mips_lite::opcode::JR:
            // Jump register - use the value in Rs as the new PC
            ex_data->alu_result = ex_data->rs_value;  // Set PC to value in Rs
            branch_taken = true;
            break;

        case mips_lite::opcode::HALT:
            // Set a flag to indicate program termination
            // This might require adding a 'halted' flag to your FunctionalSimulator class
            // For now, we'll just pass the current PC
            ex_data->alu_result = pc;
            if (!halt_pipeline) {
                halt_pipeline = true;
                std::cerr
                    << "Warning: HALT instruction encountered in EXE stage, this should've been "
                       "detected in the FETCH stage and set the halt_pipeline flag to true."
                    << std::endl;
            }
            break;

        default:
            throw std::invalid_argument("Invalid opcode for execute stage");
    }
}

/**
 * @brief Performs memory operations for the instruction in the MEMORY stage.
 *
 * This method handles data memory access for load and store instructions.
 * For load instructions (LDW), it reads memory at the computed address and
 * stores the value in the pipeline data for use in the write-back stage.
 * For store instructions (STW), it writes the value from the rt register
 * to the computed memory address and logs the access in the stats.
 */
void FunctionalSimulator::memory() {
    // Get reference to PipelineStageData pointer in memory stage
    // using auto to automatically deduce the type
    auto& mem_data = pipeline[MEMORY];

    // In the case of a stall cycle or no instruction in
    // the memory stage, just return
    if (!mem_data || mem_data->isEmpty()) {
        return;
    }

    uint8_t opcode = mem_data->instruction->getOpcode();
    uint32_t addr = mem_data->alu_result;

    switch (opcode) {
        // Load word from memory
        case mips_lite::opcode::LDW:
            mem_data->memory_data = memory_parser->readMemory(addr);
            break;

        // Store word in memory
        case mips_lite::opcode::STW:
            // Write the value to memory and add the address to the stats tracking
            // of modified memory locations
            memory_parser->writeMemory(addr, mem_data->rt_value);
            stats->addMemoryAddress(addr);
            break;

        // All other instruction do not access memory
        default:
            // Non-memory instruction: do nothing
            break;
    }
}

/**
 * @brief Writes the ALU result to the destination register in the WB (Write Back) stage.
 *
 * This method checks the WRITEBACK stage of the pipeline for a valid instruction. If the
 * instruction has a destination register, the ALU result is written to the corresponding register
 * in the register file. It also updates the statistics by tracking the modified register.
 *
 * If the WB stage is empty, as may be the case when a stall cycle has been inserted, no action is
 * performed.
 */
void FunctionalSimulator::writeBack() {
    // Get reference to PipelineStageData pointer in WB stage
    // using auto to automatically deduce the type
    auto& wb_data = pipeline[WRITEBACK];

    // In the case of a stall cycle or no instruction in
    // the WB stage, just return
    if (!wb_data || wb_data->isEmpty()) {
        return;
    }

    // If there is a destination register value, write the
    // ALU result value to it.
    // NOTE: The alu_result member defaults to an initial value
    // of zero, so that will be seen as a valid value whether or
    // not an instruction has produced any alu_result.
    if (wb_data->dest_reg.has_value()) {
        uint8_t dest = wb_data->dest_reg.value();
        uint8_t opcode = wb_data->instruction->getOpcode();

        // For load, use memory_data, otherwise use alu_result
        uint32_t value =
            (opcode == mips_lite::opcode::LDW) ? wb_data->memory_data : wb_data->alu_result;

        // Write the value to the register file
        register_file->write(dest, value);

        // Add this to the stats tracking modified registers
        stats->addRegister(dest);
    }
}

void FunctionalSimulator::advancePipeline() {
    // Release writeback stage
    pipeline[PipelineStage::WRITEBACK].reset();  // Smart pointer will delete the object
    pipeline[PipelineStage::WRITEBACK] =
        std::move(pipeline[PipelineStage::MEMORY]);  // Move MEM to WB
    pipeline[PipelineStage::MEMORY] =
        std::move(pipeline[PipelineStage::EXECUTE]);  // Move EXE to MEM
    if (stall) {
        stats->incrementStalls();
        // bubble insert
        pipeline[PipelineStage::EXECUTE].reset();
        // Do not advance ID or IF stages
    } else {
        pipeline[PipelineStage::EXECUTE] =
            std::move(pipeline[PipelineStage::DECODE]);  // Move ID to EXE
        pipeline[PipelineStage::DECODE] =
            std::move(pipeline[PipelineStage::FETCH]);  // Move IF to ID
        // Note: IF is now empty, so we can fetch a new instruction next cycle
    }
}

void FunctionalSimulator::cycle() {
    // Execute stages in reverse order since writeback should happen first
    // TODO: Might need to add other control logic here
    if (program_finished) {
        return;
    }
    stats->incrementClockCycles();
    writeBack();
    memory();
    execute();

    // Check for branch taken, if so, update PC and reset IF & ID stages
    if (isBranchTaken()) {
        // Update PC to the branch target
        setPC(pipeline[PipelineStage::EXECUTE]->alu_result);
        stall = false;  // Reset stall signal
        // Flush IF and ID stages
        pipeline[PipelineStage::FETCH].reset();
        pipeline[PipelineStage::DECODE].reset();
        // TODO: Do we need to track branch penalties? I couldn't find any mention in project specs
        // Reset branch taken flag
        branch_taken = false;
        advancePipeline();
        return;
    }
    // If no branch taken, continue with normal pipeline operation
    // Check for stalls and set stall signal
    // if stall is set, we will not advance IF or ID stages
    detectStalls() ? stall = true : stall = false;

    instructionDecode();
    instructionFetch();
    advancePipeline();
    // Check if the program has finished executing
    checkProgramCompletion();
}

bool FunctionalSimulator::isRegisterWriteInstruction(const Instruction* instr) const {
    if (instr == nullptr) {
        return false;
    }

    uint8_t opcode = instr->getOpcode();

    // R-type instructions always write to a register
    if (instr->getInstructionType() == mips_lite::InstructionType::R_TYPE) {
        return true;
    }

    // Only some I-type instructions write to a register
    return opcode == mips_lite::opcode::ADDI || opcode == mips_lite::opcode::SUBI ||
           opcode == mips_lite::opcode::MULI || opcode == mips_lite::opcode::ORI ||
           opcode == mips_lite::opcode::ANDI || opcode == mips_lite::opcode::XORI ||
           opcode == mips_lite::opcode::LDW;
}

uint32_t FunctionalSimulator::readRegisterValue(uint8_t reg_num) {
    if (reg_num == 0) {
        return 0;  // $0 register always returns 0
    }  // And never has hazards
    if (stall) {
        throw std::runtime_error(
            "Stall detected in ID stage but wasn't properly handled by control logic. Should never "
            "attempt to read during a stall");
    }
    // Check EXE stages for forwarding if necessary
    if (!isStageEmpty(PipelineStage::EXECUTE)) {
        PipelineStageData* ex_data = pipeline[PipelineStage::EXECUTE].get();
        if (ex_data->dest_reg.has_value() && ex_data->dest_reg.value() == reg_num) {
            // Hazard detected, return the ALU result from EX stage
            return ex_data->alu_result;
        }
    }
    // Check MEM stage for forwarding if necessary
    if (!isStageEmpty(PipelineStage::MEMORY)) {
        PipelineStageData* mem_data = pipeline[PipelineStage::MEMORY].get();
        if (mem_data->dest_reg.has_value() && mem_data->dest_reg.value() == reg_num) {
            // Hazard detected, return the ALU result from MEM stage
            return mem_data->alu_result;
        }
    }
    // No hazard, read from register file
    return register_file->read(reg_num);
}

/**
 * @brief Detects hazards in the pipeline
 * Stall rules:
 * - EX stage hazard: 2 stalls without forwarding, 0 with forwarding
 * - MEM stage hazard: 1 stall without forwarding, 0 with forwarding
 * - Load-use hazard: 2 stalls without forwarding, 1 stall with forwarding (special case)
 *
 * @return returns ture if a hazard is detected, false otherwise. Takes into account forwarding.
 * @note The return number will be > 0 for as long as there is a hazard detected.
 */
bool FunctionalSimulator::detectStalls(void) {
    if (isStageEmpty(PipelineStage::DECODE)) {
        return false;  // No instruction to decode, no stalls needed
    }

    const auto& decode_stage = pipeline[PipelineStage::DECODE];
    uint8_t rs = decode_stage->instruction->getRs();
    uint8_t rt = decode_stage->instruction->getRt();
    bool needs_rt = needsRtValue(decode_stage->instruction.get());

    // Helper lambda to check if a register causes a hazard
    // R0 never causes hazards since it's always 0
    auto causesHazard = [](uint8_t reg, uint8_t dest_reg) -> bool {
        return reg != 0 && reg == dest_reg;
    };

    // Early exit if no meaningful source registers
    if (rs == 0 && (!needs_rt || rt == 0)) {
        return false;  // No hazard possible with only R0 sources
    }

    // Check for hazards with the EX stage
    if (!isStageEmpty(PipelineStage::EXECUTE)) {
        const auto& ex_data = pipeline[PipelineStage::EXECUTE];

        if (ex_data->dest_reg.has_value()) {
            uint8_t dest_reg = ex_data->dest_reg.value();

            // Check if any source register causes a hazard
            bool rs_hazard = causesHazard(rs, dest_reg);
            bool rt_hazard = needs_rt && causesHazard(rt, dest_reg);

            if (rs_hazard || rt_hazard) {
                if (forward) {
                    if (ex_data->instruction &&
                        ex_data->instruction->getOpcode() == mips_lite::opcode::LDW) {
                        // Load-use hazard with forwarding
                        return true;  // Forwarding can't resolve this, so return true until load is
                                      // done in MEM stage
                    }
                    return false;  // No stalls needed with forwarding
                } else {
                    return true;  // EX stage hazard without forwarding
                }
            }
        }
    }

    // Check for hazards with the MEM stage
    if (!isStageEmpty(PipelineStage::MEMORY)) {
        const auto& mem_data = pipeline[PipelineStage::MEMORY];

        if (mem_data->dest_reg.has_value()) {
            uint8_t dest_reg = mem_data->dest_reg.value();

            // Check if any source register causes a hazard
            bool rs_hazard = causesHazard(rs, dest_reg);
            bool rt_hazard = needs_rt && causesHazard(rt, dest_reg);

            if (rs_hazard || rt_hazard) {
                return !forward;
            }
        }
    }
    return 0;  // No hazards detected
}

/**
 * @brief Helper function to determine if instruction needs Rt register value (source operand).
 * @param instr Instruction to check
 * @return true if Rt value is needed as a source operand
 */
bool FunctionalSimulator::needsRtValue(const Instruction* instr) const {
    if (!instr) return false;

    uint8_t opcode = instr->getOpcode();

    // R-type instructions use both Rs and Rt as source operands
    if (instr->getInstructionType() == mips_lite::InstructionType::R_TYPE) {
        return true;
    }

    // I-type instructions that use Rt as source operand
    switch (opcode) {
        case mips_lite::opcode::BEQ:  // BEQ compares Rs and Rt
        case mips_lite::opcode::STW:  // STW stores value from Rt
            return true;
        default:
            return false;  // Most I-type instructions only use Rs
    }
}

void FunctionalSimulator::checkProgramCompletion() {
    if (!halt_pipeline) {
        return;  // Halt instruction not encountered, program not finished
    }
    // If halt instruction was encountered, check if the pipeline is empty
    for (const auto& stage : pipeline) {
        if (stage && !stage->isEmpty()) {
            return;  // Pipeline is not empty, program still running
        }
    }
    // If we reach here, pipeline is empty and halt encountered
    program_finished = true;  // Set program finished flag
}