#include "functional_simulator.h"

#include <cstddef>
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

void FunctionalSimulator::instructionDecode(Instruction* instr) {
    // Operation: Decode instruction and access register file to read the register sources
    // Store in temporary A and B registers
    // Checks Stall conditions
    (void)instr;  // TODO: Probably remove this

    // TODO: Implement forwarding logic but raise error for now
    if (forward) {
        throw std::runtime_error("Forwarding not implemented yet");
    }

    PipelineData data = ifid_reg.current();
    if (stall > 0) {
        stall--;
        // Bubble insert
        data.instr = nullptr;  // Set instruction to null
        data.reg_a = 0;
        data.reg_b = 0;
        data.result = 0;
        data.wb_reg = 0;
        idex_reg.setNext(data);
        return;
    }

    // Identify source registers
    uint8_t rs = data.instr->getRs();
    uint8_t rt = data.instr->getRt();
    bool rs_hazard = false;
    bool rt_hazard = false;

    // TODO: Consider putting this in a separate function
    // TODO: Must consider all cases of hazards even with forwarding
    // Note: Need to consider data hazards on $0 which will always = 0, hence is not
    // a true dependency and needs to be ignored.
    // TODO: Load instructions still result in a one cycle stall even with forwarding
    // be sure to check this when implmenting forwarding
    // Check EX/MEM pipeline register for hazards
    if (rs != 0) {
        if (exmem_reg.current().instr && exmem_reg.current().wb_reg.has_value()) {
            if (rs == exmem_reg.current().wb_reg.value()) rs_hazard = true;
            if (rt == exmem_reg.current().wb_reg.value()) rt_hazard = true;
            if ((rs_hazard || rt_hazard) && !forward) setStall(2);
            return;
        }
    }
    // Check MEM/WB pipeline register for hazards
    else if (rt != 0) {
        if (memwb_reg.current().instr && memwb_reg.current().wb_reg.has_value()) {
            if (rs == memwb_reg.current().wb_reg.value()) rs_hazard = true;
            if (rt == memwb_reg.current().wb_reg.value()) rt_hazard = true;
            if ((rs_hazard || rt_hazard) && !forward) setStall(1);
            return;
        }
    }

    // No hazards or forwarding is ON, continue with decode
    data.reg_a = register_file->read(rs);
    data.branch_taken = std::nullopt;

    uint8_t opcode = data.instr->getOpcode();
    // Determine what to put in reg_b (i.e. ALU Source 2) and wb_reg based on instruction type
    if (mips_lite::is_branch_instruction(opcode)) {
        // Branch instructions
        data.wb_reg = std::nullopt;  // No write-back for branches
        data.branch_taken = false;   // Initialize branch flag

        if (opcode == mips_lite::opcode::BEQ) {
            // BEQ needs rt value for comparison
            data.reg_b = register_file->read(rt);
        } else {
            // BZ uses immediate for branch offset
            data.reg_b = data.instr->getImmediate();
        }
    } else if (mips_lite::is_jump_instruction(opcode)) {
        // Jump instructions
        data.wb_reg = std::nullopt;  // No write-back for jumps
        data.branch_taken = false;   // Initialize branch flag

        // JR uses rs as jump target
        // No need to set reg_b for JR
    } else if (opcode == mips_lite::opcode::STW) {
        // Store instructions
        data.wb_reg = std::nullopt;               // No write-back for stores
        data.reg_b = data.instr->getImmediate();  // Offset
        data.str_val = register_file->read(rt);   // Value to store
    } else if (data.instr->hasRd()) {
        // R-type instructions (register-register)
        data.reg_b = register_file->read(rt);
        data.wb_reg = data.instr->getRd();
    } else if (data.instr->hasImmediate()) {
        // I-type instructions (register-immediate)
        data.reg_b = data.instr->getImmediate();
        data.wb_reg = data.instr->getRt();
    } else {
        throw std::invalid_argument("Invalid instruction type for decode stage");
    }

    // Pass data to ID/EX pipeline register
    idex_reg.setNext(data);
}

void FunctionalSimulator::execute(Instruction* instr) {
    (void)instr;  // TODO: Probably remove this
    PipelineData data = idex_reg.current();
    if (data.instr == nullptr) {
        exmem_reg.setNext(data);
        return;
    }

    switch (data.instr->getOpcode()) {
        // Arithmetic Operations
        case mips_lite::opcode::ADD:
        case mips_lite::opcode::ADDI:
            data.result = data.reg_a + data.reg_b;
            break;

        case mips_lite::opcode::SUB:
        case mips_lite::opcode::SUBI:
            data.result = data.reg_a - data.reg_b;
            break;

        case mips_lite::opcode::MUL:
        case mips_lite::opcode::MULI:
            data.result = data.reg_a * data.reg_b;
            break;

        // Logical Operations
        case mips_lite::opcode::OR:
        case mips_lite::opcode::ORI:
            data.result = data.reg_a | data.reg_b;
            break;

        case mips_lite::opcode::AND:
        case mips_lite::opcode::ANDI:
            data.result = data.reg_a & data.reg_b;
            break;

        case mips_lite::opcode::XOR:
        case mips_lite::opcode::XORI:
            data.result = data.reg_a ^ data.reg_b;
            break;

        // Memory Effective Address Calculation -> Load and Store
        case mips_lite::opcode::LDW:
            data.result = data.reg_a + data.reg_b;
            break;

        case mips_lite::opcode::STW:
            data.result = data.reg_a + data.reg_b;
            break;

        // Control Flow Ops
        case mips_lite::opcode::BZ:
            if (data.reg_a == 0) {
                data.result = pc + (data.reg_b * 4);
                data.branch_taken = true;  // Indicate that a branch was taken
                break;
            }
            data.branch_taken = false;
            break;

        case mips_lite::opcode::BEQ:
            if (data.reg_a == data.reg_b) {
                // Branch if equal - calculate target address
                int32_t offset = data.instr->getImmediate();
                data.result = pc + (offset * 4);
                data.branch_taken = true;  // Indicate that a branch was taken
            } else {
                // No branch, PC will be updated normally
                data.branch_taken = false;
                data.result = pc;  // No change (PC should be updated in fetch stage)
            }
            break;

        case mips_lite::opcode::JR:
            // Jump register - use the value in Rs as the new PC
            data.result = data.reg_a;
            data.branch_taken = true;
            break;

        case mips_lite::opcode::HALT:
            // Set a flag to indicate program termination
            // This might require adding a 'halted' flag to your FunctionalSimulator class
            // For now, we'll just pass the current PC
            data.result = pc;
            break;

        default:
            throw std::invalid_argument("Invalid opcode for execute stage");
    }

    // Set the next value for the EX/MEM pipeline register
    exmem_reg.setNext(data);
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

uint32_t FunctionalSimulator::getForwardedValue(int stage, uint8_t reg_num) {
    (void)stage;
    (void)reg_num;
    // TODO: Implement forwarding logic
    // Check if the register is being written in the pipeline stages
    return 0;
}
