#include "mips_instruction.h"

#include <cstdint>

Instruction::Instruction(uint32_t instruction) : instruction_(instruction) {
    opcode_ = mips_lite::get_opcode(instruction_);
    instruction_type_ = mips_lite::get_instruction_type(opcode_);
    rs_ = mips_lite::get_rs(instruction_);
    rt_ = mips_lite::get_rt(instruction_);

    // Control word generation
    control_word_ = mips_lite::get_control_word(opcode_);

    if (instruction_type_ == mips_lite::InstructionType::R_TYPE) {
        // R-type instructions
        rd_ = mips_lite::get_rd(instruction_);
        // For R-type, immediate and address remain uninitialized (std::nullopt)
    } else {
        // I-type instructions
        immediate_ = static_cast<int32_t>(mips_lite::get_immediate(instruction_));
    }
}
