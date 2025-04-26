#pragma once
#include <sys/types.h>

#include <cstdint>
#include <optional>

#include "mips_lite_defs.h"

class Instruction {
   private:
    uint32_t instruction_;
    uint8_t opcode_;                    // The opcode of the instruction
    uint8_t rs_;                        // Source register
    uint8_t rt_;                        // Target register or second source register
    std::optional<uint8_t> rd_;         // Destination register (only for R-type)
    std::optional<int32_t> immediate_;  // Sign extended immediate value (only for I-type)
    uint16_t control_word_;  // Control word for the instruction TODO: Do we need this here?
    mips_lite::InstructionType instruction_type_;  // Type of instruction (R, I)

   public:
    explicit Instruction(uint32_t instruction);
    ~Instruction() = default;  // TODO: Is this correct? Any other destructor needed?

    // Getters
    uint8_t getOpcode() const { return opcode_; }
    uint8_t getRs() const { return rs_; }
    uint8_t getRt() const { return rt_; }

    // Getters for optional fields
    bool hasRd() const { return rd_.has_value(); }
    uint8_t getRd() const { return rd_.value_or(0); }  // Returns 0 if not present
    bool hasImmediate() const { return immediate_.has_value(); }
    int32_t getImmediate() const { return immediate_.value_or(0); }  // Returns 0 if not present
    // Other getters
    uint32_t getInstruction() const { return instruction_; }
    mips_lite::InstructionType getInstructionType() const { return instruction_type_; }
    uint16_t getControlWord() const { return control_word_; }
    // Helper to check HALT instruction
    bool isHaltInstruction() const { return opcode_ == mips_lite::opcode::HALT; }
};
