#include "functional_simulator.h"

#include <cstddef>
#include <cstdint>

#include "memory_interface.h"
#include "mips_instruction.h"
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
    if (stage < 0 || stage >= PipelineStage::NUM_STAGES) {
        throw std::out_of_range("Pipeline stage index out of range");
    }
    return pipeline[stage].get();  // Use .get() to return the raw pointer
}

bool FunctionalSimulator::isStageEmpty(int stage) const {
    if (stage < 0 || stage >= PipelineStage::NUM_STAGES) {
        throw std::out_of_range("Pipeline stage index out of range");
    }
    return pipeline[stage] == nullptr;
}

void FunctionalSimulator::instructionFetch() {
    if (stall > 0) {
        return;  // Skip fetch if stall is active
    }
}

void FunctionalSimulator::instructionDecode() {
    // Stub: Decode the instruction (parse operands, maybe identify hazards)
    // Set dirty bit on registers which will be written back to later
}

void FunctionalSimulator::execute() {
    // Stub: Perform ALU operation specified or calculate branch effective address
}

void FunctionalSimulator::memory() {
    // Stub: Access memory if needed (load or store only)
}

void FunctionalSimulator::writeBack() {
    // Stub: Write result back to register file, clear dirty bit
}

void FunctionalSimulator::advancePipeline() {
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
    // Execute stages in reverse order since each stage depends on the previous one
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

uint32_t FunctionalSimulator::getForwardedValue(int stage, uint8_t reg_num) {
    (void)stage;
    (void)reg_num;
    // TODO: Implement forwarding logic
    // Check if the register is being written in the pipeline stages
    return 0;
}
