#include <gtest/gtest.h>

#include "logical_instr_set.h"
#include "mips_instruction.h"
#include "mips_lite_defs.h"
#include "register_file.h"

using namespace mips_lite;

// Helper function for R-type instruction encoding
uint32_t make_rtype(uint8_t opcode, uint8_t rs, uint8_t rt, uint8_t rd) {
    return (opcode << 26) | (rs << 21) | (rt << 16) | (rd << 11);
}

// Helper function for I-type instruction encoding
uint32_t make_itype(uint8_t opcode, uint8_t rs, uint8_t rt, uint16_t imm) {
    return (opcode << 26) | (rs << 21) | (rt << 16) | (imm & 0xFFFF);
}

//
// OR / ORI Tests
//
TEST(LogicalInstructionTest, OR_PerBit) {
    RegisterFile rf;
    rf.write(1, 0b10101010);
    rf.write(2, 0b11001100);

    Instruction instr(make_rtype(opcode::OR, 1, 2, 3));
    execute_logical_instruction(instr, rf);

    EXPECT_EQ(rf.read(3), 0b11101110);
}

TEST(LogicalInstructionTest, OR_WithZeros) {
    RegisterFile rf;
    rf.write(1, 0b11110000);
    rf.write(2, 0b00000000);

    Instruction instr(make_rtype(opcode::OR, 1, 2, 3));
    execute_logical_instruction(instr, rf);

    EXPECT_EQ(rf.read(3), rf.read(1));
}

TEST(LogicalInstructionTest, OR_WithOnes) {
    RegisterFile rf;
    rf.write(1, 0b00001111);
    rf.write(2, 0xFFFFFFFF);

    Instruction instr(make_rtype(opcode::OR, 1, 2, 3));
    execute_logical_instruction(instr, rf);

    EXPECT_EQ(rf.read(3), 0xFFFFFFFF);
}

//
// AND / ANDI Tests
//
TEST(LogicalInstructionTest, AND_PerBit) {
    RegisterFile rf;
    rf.write(1, 0b10101010);
    rf.write(2, 0b11001100);

    Instruction instr(make_rtype(opcode::AND, 1, 2, 3));
    execute_logical_instruction(instr, rf);

    EXPECT_EQ(rf.read(3), 0b10001000);
}

TEST(LogicalInstructionTest, AND_WithZeros) {
    RegisterFile rf;
    rf.write(1, 0b11110000);
    rf.write(2, 0x00000000);

    Instruction instr(make_rtype(opcode::AND, 1, 2, 3));
    execute_logical_instruction(instr, rf);

    EXPECT_EQ(rf.read(3), 0x00000000);
}

TEST(LogicalInstructionTest, AND_WithOnes) {
    RegisterFile rf;
    rf.write(1, 0x12345678);
    rf.write(2, 0xFFFFFFFF);

    Instruction instr(make_rtype(opcode::AND, 1, 2, 3));
    execute_logical_instruction(instr, rf);

    EXPECT_EQ(rf.read(3), 0x12345678);
}

//
// XOR / XORI Tests
//
TEST(LogicalInstructionTest, XOR_PerBit) {
    RegisterFile rf;
    rf.write(1, 0b10101010);
    rf.write(2, 0b11001100);

    Instruction instr(make_rtype(opcode::XOR, 1, 2, 3));
    execute_logical_instruction(instr, rf);

    EXPECT_EQ(rf.read(3), 0b01100110);
}

TEST(LogicalInstructionTest, XOR_WithZeros) {
    RegisterFile rf;
    rf.write(1, 0b10101010);

    Instruction instr(make_itype(opcode::XORI, 1, 2, 0x0000));
    execute_logical_instruction(instr, rf);

    EXPECT_EQ(rf.read(2), rf.read(1));  // XOR with 0 returns same number
}

TEST(LogicalInstructionTest, XOR_WithOnes) {
    RegisterFile rf;
    rf.write(1, 0x000000AA);

    Instruction instr(make_itype(opcode::XORI, 1, 2, 0x0000FFFF));
    execute_logical_instruction(instr, rf);

    EXPECT_EQ(rf.read(2), 0x000000AA ^ 0x0000FFFF);  // XORI zero-extends immediate to 32 bits
}
