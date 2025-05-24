/**
 * @file decode_stage_tests.cpp
 * @brief Tests for Decode stage of pipeline
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>

#include "functional_simulator.h"
#include "memory_interface.h"
#include "mips_instruction.h"
#include "register_file.h"
#include "stats.h"

using ::testing::NiceMock;

// Only mock the memory parser
class MockMemoryParser : public IMemoryParser {
   public:
    MOCK_METHOD(uint32_t, readInstruction, (uint32_t address), (override));
    MOCK_METHOD(uint32_t, readMemory, (uint32_t address), (override));
    MOCK_METHOD(void, writeMemory, (uint32_t address, uint32_t value), (override));
};

class DecodeStageTest : public ::testing::Test {
   protected:
    RegisterFile rf;
    Stats stats;
    NiceMock<MockMemoryParser> mem;
    std::unique_ptr<FunctionalSimulator> sim;

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

    void SetUp() override {
        sim = std::make_unique<FunctionalSimulator>(&rf, &stats, &mem, true);

        // Initialize some register values for testing
        rf.write(1, 100);  // $1 = 100
        rf.write(2, 200);  // $2 = 200
        rf.write(3, 300);  // $3 = 300
        rf.write(4, 400);  // $4 = 400
        rf.write(5, 500);  // $5 = 500
        rf.write(6, 600);  // $6 = 600
        rf.write(7, 700);  // $7 = 700
        rf.write(8, 800);  // $8 = 800
        rf.write(9, 900);  // $9 = 900
        rf.write(10, 1000);  // $10 = 1000
        rf.write(11, 1100);  // $11 = 1100

        
    }

    // Helper to properly initialize the decode stage with an instruction
    void setupDecodeStage(uint32_t instruction_word) {
        // Create a new instruction object
        auto instr = std::make_unique<Instruction>(instruction_word);

        // Create pipeline stage data for decode stage
        auto decode_data = std::make_unique<PipelineStageData>();
        decode_data->instruction = std::move(instr);
        decode_data->pc = sim->getPC();  // Use current PC

        // Place the instruction in the decode stage using getPipeline()
        sim->getPipeline()[FunctionalSimulator::PipelineStage::DECODE] = std::move(decode_data);
    }

    // Helper to get decode stage data
    const PipelineStageData* getDecodeStageData() {
        return sim->getPipelineStage(FunctionalSimulator::PipelineStage::DECODE);
    }

    // Helper to get execute stage data (after decode processes)
    const PipelineStageData* getExecuteStageData() {
        return sim->getPipelineStage(FunctionalSimulator::PipelineStage::EXECUTE);
    }
};

// Test R-type instruction decode (ADD) - Most basic case
// This validates: register reading, destination register setting, instruction decoding
TEST_F(DecodeStageTest, DecodeBasicRTypeInstruction) {
    // This instruction reads $1 (rs=1), adds immediate 50, writes result to $3 (rt=3)
    
    // Set up the decode stage with our instruction
    setupDecodeStage(r_type_add_instr);

    // Verify the instruction is in decode stage before processing
    const PipelineStageData* decode_data = getDecodeStageData();
    ASSERT_NE(decode_data, nullptr);
    ASSERT_NE(decode_data->instruction, nullptr);

    // Decode the instruction - this should:
    // 1. Read Rs register value ($1 = 100)
    // 2. Read Rt register value ($2 = 200)
    // 3. Set destination register to Rt (register 3)
    sim->instructionDecode();

    // Verify the decode stage now has the correct values set
    EXPECT_EQ(decode_data->rs_value, 100);              // Should have read $1 value
    EXPECT_EQ(decode_data->rt_value, 200);              // Should have read $2 value
    EXPECT_TRUE(decode_data->dest_reg.has_value());     // Should have destination register
    EXPECT_EQ(decode_data->dest_reg.value(), 3);        // Destination should be $3 (rt)
    
}

TEST_F(DecodeStageTest, DecodeITypeInstructionWithImmediate) {
    // Set up the decode stage with our instruction
    setupDecodeStage(i_type_addi_pos_instr);

    // Decode the instruction
    sim->instructionDecode();

    // Verify the decode stage now has the correct values set
    const PipelineStageData* decode_data = getDecodeStageData();
    ASSERT_NE(decode_data, nullptr);
    ASSERT_NE(decode_data->instruction, nullptr);

    // This instruction should 
    // 1. Read Rs register value ($4 = 400)
    // 2. Set Rt as destination register (register 5)

    EXPECT_EQ(decode_data->rs_value, 400);              // Should have read $4 value
    EXPECT_TRUE(decode_data->dest_reg.has_value());     // Should have destination register
    EXPECT_EQ(decode_data->dest_reg.value(), 5);        // Destination should be $5 (rt)
}

TEST_F(DecodeStageTest, DecodeBEQTypeInstruction) {
    // Set up the decode stage with our instruction
    setupDecodeStage(i_type_beq_instr);

    // Decode the instruction
    sim->instructionDecode();

    // This instruction should
    // 1. Read Rs register value ($10 = 1000)
    // 2. Read Rt register value ($11 = 1100)
    // 3. Set destination register to null (no writeback)

    const PipelineStageData* decode_data = getDecodeStageData();
    ASSERT_NE(decode_data, nullptr);
    ASSERT_NE(decode_data->instruction, nullptr);
    EXPECT_EQ(decode_data->rs_value, 1000);             // Should have read $10 value
    EXPECT_EQ(decode_data->rt_value, 1100);             // Should have read $11 value
    EXPECT_FALSE(decode_data->dest_reg.has_value());    // No destination register
    
}

TEST_F(DecodeStageTest, ForwardingToRsRegister) {
    // Set up execute stage with instruction that writes to $1
    auto execute_data = std::make_unique<PipelineStageData>();
    execute_data->instruction = std::make_unique<Instruction>(0x00000000);
    execute_data->pc = sim->getPC();
    execute_data->alu_result = 1234;
    execute_data->dest_reg = 1;

    sim->getPipeline()[FunctionalSimulator::PipelineStage::EXECUTE] = std::move(execute_data);

    // Decode instruction that reads $1
    setupDecodeStage(r_type_add_instr);
    sim->instructionDecode();

    const PipelineStageData* decode_data = getDecodeStageData();
    EXPECT_EQ(decode_data->rs_value, 1234);  // Forwarded value
    EXPECT_EQ(decode_data->rt_value, 200);   // Normal register value
}

// Test forwarding to Rt register  
TEST_F(DecodeStageTest, ForwardingToRtRegister) {
    // Set up execute stage with instruction that writes to $2
    auto execute_data = std::make_unique<PipelineStageData>();
    execute_data->instruction = std::make_unique<Instruction>(0x00000000);
    execute_data->pc = sim->getPC();
    execute_data->alu_result = 5678;
    execute_data->dest_reg = 2;

    sim->getPipeline()[FunctionalSimulator::PipelineStage::EXECUTE] = std::move(execute_data);

    // Decode instruction that reads $2
    setupDecodeStage(r_type_add_instr);
    sim->instructionDecode();

    const PipelineStageData* decode_data = getDecodeStageData();
    EXPECT_EQ(decode_data->rs_value, 100);   // Normal register value
    EXPECT_EQ(decode_data->rt_value, 5678);  // Forwarded value
}

// No forwarding when register numbers don't match
TEST_F(DecodeStageTest, NoForwardingWhenRegistersDontMatch) {
    // Set up execute stage with instruction that writes to $5 (not used in our decode instruction)
    auto execute_data = std::make_unique<PipelineStageData>();
    execute_data->instruction = std::make_unique<Instruction>(0x00000000); // Dummy instruction
    execute_data->pc = sim->getPC();
    execute_data->alu_result = 9999;  // Some value that shouldn't be forwarded
    execute_data->dest_reg = 5;       // Writing to register $5

    // Place the instruction in the execute stage
    sim->getPipeline()[FunctionalSimulator::PipelineStage::EXECUTE] = std::move(execute_data);

    // Set up decode stage with ADD $3, $1, $2 (doesn't use $5)
    setupDecodeStage(r_type_add_instr);

    // Decode the instruction
    sim->instructionDecode();

    // Verify NO forwarding occurred
    const PipelineStageData* decode_data = getDecodeStageData();
    ASSERT_NE(decode_data, nullptr);
    ASSERT_NE(decode_data->instruction, nullptr);

    EXPECT_EQ(decode_data->rs_value, 100);  // Normal register file value for $1
    EXPECT_EQ(decode_data->rt_value, 200);  // Normal register file value for $2
    EXPECT_TRUE(decode_data->dest_reg.has_value());
    EXPECT_EQ(decode_data->dest_reg.value(), 3);
}
