/**
 * @file mips_instruction_tests.cpp
 * @brief Unit tests for Instruction class
 */

#include <gtest/gtest.h>
#include <sys/types.h>

#include <cstdint>
#include <optional>

#include "mips_instruction.h"
#include "mips_lite_defs.h"

class InstructionTest : public ::testing::Test {
   protected:
    // Sample instructions to use in tests
    // R-type: ADD $rd, $rs, $rt (opcode=0, rs=1, rt=2, rd=3)
    const uint32_t r_type_add_instr = 0x00221800;  // ADD $3, $1, $2

    // I-type: ADDI $rt, $rs, immediate (opcode=1, rs=4, rt=5, imm=100)
    const uint32_t i_type_addi_pos_instr = 0x04850064;  // ADDI $5, $4, 100

    // I-type with negative immediate: ADDI $rt, $rs, immediate (opcode=1, rs=6, rt=7, imm=-100)
    const uint32_t i_type_addi_neg_instr = 0x04C7FF9C;  // ADDI $7, $6, -100

    // I-type memory: LDW $rt, imm($rs) (opcode=12, rs=8, rt=9, imm=200)
    const uint32_t i_type_ldw_instr = 0x310900C8;  // LDW $9, 200($8)

    // I-type branch: BEQ $rs, $rt, imm (opcode=15, rs=10, rt=11, imm=-50)
    const uint32_t i_type_beq_instr = 0x3D4BFFCE;  // BEQ $10, $11, -50
};

// Test R-type instruction construction
TEST_F(InstructionTest, RTypeAddInstruction) {
    Instruction instr(r_type_add_instr);

    // Verify instruction type
    EXPECT_EQ(instr.getInstructionType(), mips_lite::InstructionType::R_TYPE);

    // Verify opcode
    EXPECT_EQ(instr.getOpcode(), mips_lite::opcode::ADD);

    // Verify registers
    EXPECT_EQ(instr.getRs(), 1);
    EXPECT_EQ(instr.getRt(), 2);
    EXPECT_TRUE(instr.hasRd());
    EXPECT_EQ(instr.getRd(), 3);

    // Verify immediate is not set for R-type
    EXPECT_FALSE(instr.hasImmediate());

    // Verify control word
    uint32_t expected_control = mips_lite::control::REG_DST | mips_lite::control::REG_WRITE |
                                mips_lite::control::ALU_OP_ADD;
    EXPECT_EQ(instr.getControlWord(), expected_control);
}

// Test I-type instruction with positive immediate
TEST_F(InstructionTest, ITypeInstructionPositiveImmediate) {
    Instruction instr(i_type_addi_pos_instr);

    // Verify instruction type
    EXPECT_EQ(instr.getInstructionType(), mips_lite::InstructionType::I_TYPE);

    // Verify opcode
    EXPECT_EQ(instr.getOpcode(), mips_lite::opcode::ADDI);

    // Verify registers
    EXPECT_EQ(instr.getRs(), 4);
    EXPECT_EQ(instr.getRt(), 5);
    EXPECT_FALSE(instr.hasRd());  // No rd in I-type

    // Verify immediate
    EXPECT_TRUE(instr.hasImmediate());
    EXPECT_EQ(instr.getImmediate(), 100);

    // Verify control word
    uint32_t expected_control = mips_lite::control::ALU_SRC | mips_lite::control::REG_WRITE |
                                mips_lite::control::ALU_OP_ADD;
    EXPECT_EQ(instr.getControlWord(), expected_control);
}

// Test I-type instruction with negative immediate (sign extension)
TEST_F(InstructionTest, ITypeInstructionNegativeImmediate) {
    Instruction instr(i_type_addi_neg_instr);

    // Verify instruction type
    EXPECT_EQ(instr.getInstructionType(), mips_lite::InstructionType::I_TYPE);

    // Verify opcode
    EXPECT_EQ(instr.getOpcode(), mips_lite::opcode::ADDI);

    // Verify registers
    EXPECT_EQ(instr.getRs(), 6);
    EXPECT_EQ(instr.getRt(), 7);

    // Verify immediate with sign extension
    EXPECT_TRUE(instr.hasImmediate());
    EXPECT_EQ(instr.getImmediate(), -100);

    // Verify the sign extension worked correctly by checking bit pattern
    // -100 in 32-bit two's complement should have upper 16 bits all set to 1
    uint32_t imm_value = static_cast<uint32_t>(instr.getImmediate());
    EXPECT_EQ(imm_value & 0xFFFF0000, 0xFFFF0000) << "Sign extension failed";
}

// Test Memory access I-type instruction
TEST_F(InstructionTest, MemoryInstructionLDW) {
    Instruction instr(i_type_ldw_instr);

    // Verify instruction type
    EXPECT_EQ(instr.getInstructionType(), mips_lite::InstructionType::I_TYPE);

    // Verify opcode
    EXPECT_EQ(instr.getOpcode(), mips_lite::opcode::LDW);

    // Verify registers
    EXPECT_EQ(instr.getRs(), 8);
    EXPECT_EQ(instr.getRt(), 9);

    // Verify immediate
    EXPECT_TRUE(instr.hasImmediate());
    EXPECT_EQ(instr.getImmediate(), 200);

    // Verify control word for memory load
    uint32_t expected_control = mips_lite::control::ALU_SRC | mips_lite::control::MEM_READ |
                                mips_lite::control::MEM_TO_REG | mips_lite::control::REG_WRITE |
                                mips_lite::control::ALU_OP_ADD;
    EXPECT_EQ(instr.getControlWord(), expected_control);
}

// Test Branch I-type instruction with negative offset
TEST_F(InstructionTest, BranchInstructionBEQ) {
    Instruction instr(i_type_beq_instr);

    // Verify instruction type
    EXPECT_EQ(instr.getInstructionType(), mips_lite::InstructionType::I_TYPE);

    // Verify opcode
    EXPECT_EQ(instr.getOpcode(), mips_lite::opcode::BEQ);

    // Verify registers
    EXPECT_EQ(instr.getRs(), 10);
    EXPECT_EQ(instr.getRt(), 11);

    // Verify immediate with sign extension
    EXPECT_TRUE(instr.hasImmediate());
    EXPECT_EQ(instr.getImmediate(), -50);

    // Verify the sign extension worked correctly
    uint32_t imm_value = static_cast<uint32_t>(instr.getImmediate());
    EXPECT_EQ(imm_value & 0xFFFF0000, 0xFFFF0000) << "Sign extension failed";

    // Verify control word for branch
    uint32_t expected_control = mips_lite::control::BRANCH | mips_lite::control::ALU_OP_SUB;
    EXPECT_EQ(instr.getControlWord(), expected_control);
}
