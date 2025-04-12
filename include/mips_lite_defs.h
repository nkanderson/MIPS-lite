/**
 * mips_lite_defs.h
 *
 * Defines for MIPS-lite ISA Simulator
 *
 * Instruction Formats:
 * 1. R-type Format (6 bits opcode, 5 bits Rs, 5 bits Rt, 5 bits Rd, 11 bits
 * unused) Used by: ADD, SUB, MUL, OR, AND, XOR
 *
 * 2. I-type Format (6 bits opcode, 5 bits Rs, 5 bits Rt, 16 bits immediate)
 *    Used by: ADDI, SUBI, MULI, ORI, ANDI, XORI, LDW, STW, BZ, BEQ, JR, HALT
 */

#pragma once

#include <cstdint>

namespace mips_lite {

// Instruction Type Identifiers
enum class InstructionType { R_TYPE, I_TYPE };

// Instruction Category Types
enum class InstructionCategory { ARITHMETIC, LOGICAL, MEMORY_ACCESS, CONTROL_FLOW };

// Opcode Definitions
namespace opcode {
// Arithmetic Instructions
constexpr uint8_t ADD = 0b000000;   // 0
constexpr uint8_t ADDI = 0b000001;  // 1
constexpr uint8_t SUB = 0b000010;   // 2
constexpr uint8_t SUBI = 0b000011;  // 3
constexpr uint8_t MUL = 0b000100;   // 4
constexpr uint8_t MULI = 0b000101;  // 5

// Logical Instructions
constexpr uint8_t OR = 0b000110;    // 6
constexpr uint8_t ORI = 0b000111;   // 7
constexpr uint8_t AND = 0b001000;   // 8
constexpr uint8_t ANDI = 0b001001;  // 9
constexpr uint8_t XOR = 0b001010;   // 10
constexpr uint8_t XORI = 0b001011;  // 11

// Memory Access Instructions
constexpr uint8_t LDW = 0b001100;  // 12
constexpr uint8_t STW = 0b001101;  // 13

// Control Flow Instructions
constexpr uint8_t BZ = 0b001110;    // 14
constexpr uint8_t BEQ = 0b001111;   // 15
constexpr uint8_t JR = 0b010000;    // 16
constexpr uint8_t HALT = 0b010001;  // 17
}  // namespace opcode

// Register Constants
constexpr uint8_t NUM_REGISTERS = 32;
// Memory Constants
constexpr uint8_t WORD_SIZE = 4;  // 4 bytes per word

// Pipeline Stage Definitions
enum class PipelineStage { FETCH, DECODE, EXECUTE, MEMORY, WRITEBACK };

// Bit Manipulation Helper Functions
inline uint32_t extract_bits(uint32_t value, int start, int length) {
    return (value >> start) & ((1 << length) - 1);
}

// Instruction Field Extraction Functions
inline uint8_t get_opcode(uint32_t instruction) { return extract_bits(instruction, 26, 6); }

inline uint8_t get_rs(uint32_t instruction) { return extract_bits(instruction, 21, 5); }

inline uint8_t get_rt(uint32_t instruction) { return extract_bits(instruction, 16, 5); }

inline uint8_t get_rd(uint32_t instruction) { return extract_bits(instruction, 11, 5); }

inline int16_t get_immediate(uint32_t instruction) {
    // Extract 16-bit immediate value and sign-extend it
    return static_cast<int16_t>(extract_bits(instruction, 0, 16));
}

// Get instruction type from opcode
inline InstructionType get_instruction_type(uint8_t opcode) {
    switch (opcode) {
        case opcode::ADD:
            [[fallthrough]];
        case opcode::SUB:
            [[fallthrough]];
        case opcode::MUL:
            [[fallthrough]];
        case opcode::OR:
            [[fallthrough]];
        case opcode::AND:
            [[fallthrough]];
        case opcode::XOR:
            return InstructionType::R_TYPE;
        default:
            return InstructionType::I_TYPE;
    }
}

}  // namespace mips_lite
