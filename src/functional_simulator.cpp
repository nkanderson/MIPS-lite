#include "functional_simulator.h"

#include <cstdint>

#include <sys/types.h>

#include <optional>

#include "memory_interface.h"
#include "mips_instruction.h"
#include "mips_lite_defs.h"
#include "register_file.h"
#include "stats.h"

FunctionalSimulator::FunctionalSimulator(RegisterFile* rf, Stats* st, IMemoryParser* mem,
                                         bool enable_forwarding)
    : pc(0),
      register_file(rf),
      stats(st),
      memory_parser(mem),
      forward(enable_forwarding),
      stall(0) {}

uint32_t FunctionalSimulator::getPC() const { return pc; }

bool FunctionalSimulator::isForwardingEnabled() const { return forward; }

uint8_t FunctionalSimulator::getStall() const { return stall; }

void FunctionalSimulator::setPC(uint32_t new_pc) { pc = new_pc; }

void FunctionalSimulator::setStall(uint8_t cycles) { stall = cycles; }

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
    // Stub: TODO
}

void FunctionalSimulator::instructionDecode() {
    // Operation: Decode instruction and access register file to read the register sources
    
    PipelineStageData* id_data = pipeline[PipelineStage::DECODE].get();

    if (id_data == nullptr || stall > 0) {
        // No instruction to decode
        return;
    }

    uint8_t rs = id_data->instruction->getRs();
    uint8_t rt = id_data->instruction->getRt();
    
    id_data->rs_value = readRegisterValue(rs);

    // Determine register Destination & optional second source register
    if(id_data->instruction->hasRd()) {
        id_data->dest_reg = id_data->instruction->getRd();
        //R-type instruction has two source registers, get second source register
        id_data->rt_value = readRegisterValue(rt);
    }
    else if(!id_data->instruction->hasRd() && isRegisterWriteInstruction(id_data->instruction.get())) {
        // Destination register is rt 
        id_data->dest_reg = id_data->instruction->getRt();
    }
    else {
        // No destination register
        id_data->dest_reg = std::nullopt;
        if(id_data->instruction->getOpcode() == mips_lite::opcode::BEQ) {
            // BEQ has two source registers
            id_data->rt_value = readRegisterValue(rt);
        }
    }
}

void FunctionalSimulator::execute() {
    if(isStageEmpty(PipelineStage::EXECUTE)) {
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
            ex_data->alu_result = ex_data->getRsValueSigned() + ex_data->instruction->getImmediate();
            break;

        case mips_lite::opcode::SUB:
            ex_data->alu_result = ex_data->getRsValueSigned() - ex_data->getRtValueSigned();
            break;
        case mips_lite::opcode::SUBI:
            ex_data->alu_result = ex_data->getRsValueSigned() - ex_data->instruction->getImmediate();
            break;

        case mips_lite::opcode::MUL:
            ex_data->alu_result = ex_data->getRsValueSigned() * ex_data->getRtValueSigned();
            break;
        case mips_lite::opcode::MULI:
            ex_data->alu_result = ex_data->getRsValueSigned() * ex_data->instruction->getImmediate();
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
            ex_data->alu_result = ex_data->getRsValueSigned() + ex_data->instruction->getImmediate();
            break;

        case mips_lite::opcode::STW:
            ex_data->alu_result = ex_data->getRsValueSigned() + ex_data->instruction->getImmediate();
            break;

        // Control Flow Ops
        case mips_lite::opcode::BZ:
            if (ex_data->rs_value == 0) {
                ex_data->alu_result = ex_data->pc + (ex_data->instruction->getImmediate() * 4);
                branch_taken = true;  // Indicate that a branch was taken
                                      // TODO: Controller will need to clear this flag after the branch is taken
                break;
            }
            branch_taken = false;  // No branch taken
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
                ex_data->alu_result = ex_data->pc;  // No change (PC should be updated in fetch stage)
            }
            break;

        case mips_lite::opcode::JR:
            // Jump register - use the value in Rs as the new PC
            ex_data->alu_result = ex_data->rs_value;
            branch_taken = true;
            break;

        case mips_lite::opcode::HALT:
            // Set a flag to indicate program termination
            // This might require adding a 'halted' flag to your FunctionalSimulator class
            // For now, we'll just pass the current PC
            ex_data->alu_result = pc;
            break;

        default:
            throw std::invalid_argument("Invalid opcode for execute stage");
    }
}

void FunctionalSimulator::memory() {
    // Stub: Access memory if needed (load or store only)
}

void FunctionalSimulator::writeBack() {
    // Stub: Write result back to register file, clear dirty bit
}

void FunctionalSimulator::advancePipeline() {
    // TODO: Might need to add more checks here
    // Release writeback stage
    pipeline[PipelineStage::WRITEBACK].reset();  // Smart pointer will delete the object

    // Shift instructions forward in the pipeline (from higher to lower to avoid overwriting)
    for (int i = PipelineStage::WRITEBACK; i > PipelineStage::FETCH; i--) {
        pipeline[i] = std::move(pipeline[i - 1]);
    }
    // Note because of std::move Fetch stage is now empty

    // Decrement stall counter if active
    if (stall > 0) {
        stall--;
    }
    // TODO: Maybe increase tick counter here for stats
}

void FunctionalSimulator::cycle() {
    // Execute stages in reverse order since writeback should happen first
    // TODO: Might need to add other control logic here
    writeBack();
    memory();
    execute();
    instructionDecode();
    instructionFetch();

    // Advance pipeline
    advancePipeline();
}

bool FunctionalSimulator::detectHazards() {
    // If forwarding is enabled, fewer stalls are needed
    if (forward || isStageEmpty(PipelineStage::DECODE)) {
        // TODO: Implement hazard detection with forwarding
        return false;
    } else {
        // Simple hazard detection for no forwarding:
        // Check if instruction in decode stage depends on result of instruction in execute or
        // memory stage
        return false;
    }
    return false;
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

std::optional<uint8_t> FunctionalSimulator::getDestinationRegister(const Instruction* instr) const {
    if (instr == nullptr || !isRegisterWriteInstruction(instr)) {
        return std::nullopt;
    }

    // R-type instructions write to rd
    if (instr->getInstructionType() == mips_lite::InstructionType::R_TYPE) {
        return instr->getRd();
    }

    // I-type instructions that write to a register use rt as destination
    return instr->getRt();
}

uint32_t FunctionalSimulator::readRegisterValue(uint8_t reg_num) {
    
    if (reg_num == 0) {
        return 0;  // $0 register always returns 0
    }               // And never has hazards
    if(stall > 0) {
        throw std::runtime_error("Stall detected in ID stage but wasn't properly handled by control logic. Should never attempt to read during a stall");
    }
    // Check EXE stage for hazards
    if(!isStageEmpty(PipelineStage::EXECUTE) && pipeline[PipelineStage::EXECUTE]->dest_reg.value() == reg_num) {
        return pipeline[PipelineStage::EXECUTE]->alu_result;
    }
    // Check MEM stage for hazards
    if(!isStageEmpty(PipelineStage::MEMORY) && pipeline[PipelineStage::MEMORY]->dest_reg.value() == reg_num) {
        return pipeline[PipelineStage::MEMORY]->alu_result;
    }
    else {
        return register_file->read(reg_num);
    }
}


uint32_t FunctionalSimulator::getForwardedValue(int stage, uint8_t reg_num) {
    (void)stage;
    (void)reg_num;
    // TODO: Delete Me
    return 0;
}
