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
#include <stdexcept>

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
inline uint8_t get_opcode(uint32_t instruction) {
    return static_cast<uint8_t>(extract_bits(instruction, 26, 6));
}

inline uint8_t get_rs(uint32_t instruction) {
    return static_cast<uint8_t>(extract_bits(instruction, 21, 5));
}

inline uint8_t get_rt(uint32_t instruction) {
    return static_cast<uint8_t>(extract_bits(instruction, 16, 5));
}

inline uint8_t get_rd(uint32_t instruction) {
    return static_cast<uint8_t>(extract_bits(instruction, 11, 5));
}

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

inline InstructionCategory get_instruction_category(uint8_t opcode) {
    switch (opcode) {
        case opcode::ADD:
        case opcode::ADDI:
        case opcode::SUB:
        case opcode::SUBI:
        case opcode::MUL:
        case opcode::MULI:
            return InstructionCategory::ARITHMETIC;

        case opcode::OR:
        case opcode::ORI:
        case opcode::AND:
        case opcode::ANDI:
        case opcode::XOR:
        case opcode::XORI:
            return InstructionCategory::LOGICAL;

        case opcode::LDW:
        case opcode::STW:
            return InstructionCategory::MEMORY_ACCESS;

        case opcode::BZ:
        case opcode::BEQ:
        case opcode::JR:
        case opcode::HALT:
            return InstructionCategory::CONTROL_FLOW;

        default:
            throw std::invalid_argument("Invalid opcode for instruction category");
    }
}
// Control Word Bit Positions
// Control Word Bit Positions
namespace control {
// Example control signals - TODO: adjust based on what we decide to implement
constexpr uint16_t REG_DST = 0x0001;     // 0: RT as destination, 1: RD as destination
constexpr uint16_t ALU_SRC = 0x0002;     // 0: Register, 1: Immediate
constexpr uint16_t MEM_TO_REG = 0x0004;  // 0: ALU result, 1: Memory data
constexpr uint16_t REG_WRITE = 0x0008;   // 0: No write, 1: Write to register
constexpr uint16_t MEM_READ = 0x0010;    // 0: No read, 1: Read from memory
constexpr uint16_t MEM_WRITE = 0x0020;   // 0: No write, 1: Write to memory
constexpr uint16_t BRANCH = 0x0040;      // 0: No branch, 1: Branch
constexpr uint16_t JUMP = 0x0080;        // 0: No jump, 1: Jump

// ALU operation codes (4 bits for operation selection)
constexpr uint16_t ALU_OP_ADD = 0x0000;   // Addition
constexpr uint16_t ALU_OP_SUB = 0x0100;   // Subtraction
constexpr uint16_t ALU_OP_MUL = 0x0200;   // Multiplication
constexpr uint16_t ALU_OP_OR = 0x0300;    // Logical OR
constexpr uint16_t ALU_OP_AND = 0x0400;   // Logical AND
constexpr uint16_t ALU_OP_XOR = 0x0500;   // Logical XOR
constexpr uint16_t ALU_OP_MASK = 0x0F00;  // Mask for ALU operation bits
}  // namespace control

// Function to get control word based on opcode
inline uint16_t get_control_word(uint8_t opcode) {
    uint16_t control_word = 0;

    switch (opcode) {
        // R-type arithmetic operations
        case opcode::ADD:
            control_word = control::REG_DST | control::REG_WRITE | control::ALU_OP_ADD;
            break;
        case opcode::SUB:
            control_word = control::REG_DST | control::REG_WRITE | control::ALU_OP_SUB;
            break;
        case opcode::MUL:
            control_word = control::REG_DST | control::REG_WRITE | control::ALU_OP_MUL;
            break;

        // I-type arithmetic operations
        case opcode::ADDI:
            control_word = control::ALU_SRC | control::REG_WRITE | control::ALU_OP_ADD;
            break;
        case opcode::SUBI:
            control_word = control::ALU_SRC | control::REG_WRITE | control::ALU_OP_SUB;
            break;
        case opcode::MULI:
            control_word = control::ALU_SRC | control::REG_WRITE | control::ALU_OP_MUL;
            break;

        // R-type logical operations
        case opcode::OR:
            control_word = control::REG_DST | control::REG_WRITE | control::ALU_OP_OR;
            break;
        case opcode::AND:
            control_word = control::REG_DST | control::REG_WRITE | control::ALU_OP_AND;
            break;
        case opcode::XOR:
            control_word = control::REG_DST | control::REG_WRITE | control::ALU_OP_XOR;
            break;

        // I-type logical operations
        case opcode::ORI:
            control_word = control::ALU_SRC | control::REG_WRITE | control::ALU_OP_OR;
            break;
        case opcode::ANDI:
            control_word = control::ALU_SRC | control::REG_WRITE | control::ALU_OP_AND;
            break;
        case opcode::XORI:
            control_word = control::ALU_SRC | control::REG_WRITE | control::ALU_OP_XOR;
            break;

        // Memory access operations
        case opcode::LDW:
            control_word = control::ALU_SRC | control::MEM_READ | control::MEM_TO_REG |
                           control::REG_WRITE | control::ALU_OP_ADD;
            break;
        case opcode::STW:
            control_word = control::ALU_SRC | control::MEM_WRITE | control::ALU_OP_ADD;
            break;

        // Control flow operations
        case opcode::BZ:
            control_word = control::ALU_SRC | control::BRANCH | control::ALU_OP_SUB;
            break;
        case opcode::BEQ:
            control_word = control::BRANCH | control::ALU_OP_SUB;
            break;
        case opcode::JR:
            control_word = control::JUMP;
            break;
        case opcode::HALT:
            // No control signals needed for HALT
            break;

        default:
            // Invalid opcode
            break;
    }

    return control_word;
}

// Helper functions for control flow instructions
inline bool is_branch_instruction(uint8_t opcode) {
    return opcode == opcode::BZ || opcode == opcode::BEQ;
}

inline bool is_jump_instruction(uint8_t opcode) { return opcode == opcode::JR; }

inline bool is_memory_instruction(uint8_t opcode) {
    return opcode == opcode::LDW || opcode == opcode::STW;
}
inline bool is_halt_instruction(uint32_t instruction) {
    return get_opcode(instruction) == opcode::HALT;
}

}  // namespace mips_lite
