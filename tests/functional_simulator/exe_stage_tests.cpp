/**
 * @file exe_stage_tests.cpp
 * @brief Tests for Execute stage of pipeline
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <memory>

#include "functional_simulator.h"
#include "memory_interface.h"
#include "mips_instruction.h"
#include "mips_lite_defs.h"
#include "register_file.h"
#include "stats.h"

using ::testing::NiceMock;
using ::testing::Return;

// Only mock the memory parser
class MockMemoryParser : public IMemoryParser {
   public:
    MOCK_METHOD(uint32_t, readInstruction, (uint32_t address), (override));
    MOCK_METHOD(uint32_t, readMemory, (uint32_t address), (override));
    MOCK_METHOD(void, writeMemory, (uint32_t address, uint32_t value), (override));
};

class ExecuteStageTest : public ::testing::Test {
   protected:
    RegisterFile rf;
    Stats stats;
    NiceMock<MockMemoryParser> mem;
    std::unique_ptr<FunctionalSimulator> sim;

    static constexpr uint32_t ADD_INSTR = 0x00221800;      // ADD $3, $1, $2
    static constexpr uint32_t SUB_INSTR = 0x08221800;      // SUB $3, $1, $2
    static constexpr uint32_t MUL_INSTR = 0x10221800;      // MUL $3, $1, $2
    static constexpr uint32_t AND_INSTR = 0x20221800;      // AND $3, $1, $2
    static constexpr uint32_t BEQ_INSTR = 0x3C280032;      // BEQ $1, $8, 50
    static constexpr uint32_t BZ_INSTR = 0x38280032;       // BZ $1, 50
    static constexpr uint32_t JR_INSTR = 0x40280000;       // JR $5
    static constexpr uint32_t LDW_INSTR = 0x30480064;      // LDW $8, 100($2)

    void SetUp() override { 
        sim = std::make_unique<FunctionalSimulator>(&rf, &stats, &mem, true);
    }

    // Helper to set up execute stage with an instruction and data
    void setupExecuteStage(uint32_t instruction_word, uint32_t rs_value, uint32_t rt_value, 
                          std::optional<uint8_t> dest_reg = std::nullopt, uint32_t pc_value = 1000) {
        // Create instruction
        auto instr = std::make_unique<Instruction>(instruction_word);

        // Create pipeline stage data for execute stage
        auto execute_data = std::make_unique<PipelineStageData>();
        execute_data->instruction = std::move(instr);
        execute_data->rs_value = rs_value;
        execute_data->rt_value = rt_value;
        execute_data->dest_reg = dest_reg;
        execute_data->pc = pc_value;

        // Place the instruction in the execute stage
        sim->getPipeline()[FunctionalSimulator::PipelineStage::EXECUTE] = std::move(execute_data);
    }

    // Helper to get execute stage data
    const PipelineStageData* getExecuteStageData() {
        return sim->getPipelineStage(FunctionalSimulator::PipelineStage::EXECUTE);
    }

    // Helper to get memory stage data (after execute processes)
    const PipelineStageData* getMemoryStageData() {
        return sim->getPipelineStage(FunctionalSimulator::PipelineStage::MEMORY);
    }
};

// Test ADD instruction execution
TEST_F(ExecuteStageTest, ExecuteADD) {
    // Set up execute stage with ADD instruction and test values
    setupExecuteStage(ADD_INSTR, 10, 20, 3);  // rs=10, rt=20, dest=3

    // Verify instruction is in execute stage
    const PipelineStageData* execute_data = getExecuteStageData();
    ASSERT_NE(execute_data, nullptr);
    ASSERT_NE(execute_data->instruction, nullptr);

    // Execute the instruction
    sim->execute();

    // Verify the ALU result is correct (10 + 20 = 30)
    EXPECT_EQ(execute_data->alu_result, 30);
    EXPECT_TRUE(execute_data->dest_reg.has_value());
    EXPECT_EQ(execute_data->dest_reg.value(), 3);
}

// Test ADD instruction but make operands negative
TEST_F(ExecuteStageTest, ExecuteADDNegative) {
    // Set up execute stage with ADD instruction and negative values
    setupExecuteStage(ADD_INSTR, static_cast<uint32_t>(-10), static_cast<uint32_t>(-20), 3);  // rs=-10, rt=-20, dest=3

    // Execute the instruction
    sim->execute();

    // Verify the ALU result is correct (-10 + -20 = -30)
    const PipelineStageData* execute_data = getExecuteStageData();
    EXPECT_EQ(execute_data->alu_result, -30);
    EXPECT_TRUE(execute_data->dest_reg.has_value());
    EXPECT_EQ(execute_data->dest_reg.value(), 3);
}

// Test SUB instruction execution
TEST_F(ExecuteStageTest, ExecuteSUB) {
    // Set up execute stage with SUB instruction and test values
    setupExecuteStage(SUB_INSTR, 30, 12, 3);  // rs=30, rt=12, dest=3

    // Execute the instruction
    sim->execute();

    // Verify the ALU result is correct (30 - 12 = 18)
    const PipelineStageData* execute_data = getExecuteStageData();
    EXPECT_EQ(execute_data->alu_result, 18);
    EXPECT_TRUE(execute_data->dest_reg.has_value());
    EXPECT_EQ(execute_data->dest_reg.value(), 3);
}

// Test MUL instruction execution
TEST_F(ExecuteStageTest, ExecuteMUL) {
    // Set up execute stage with MUL instruction and test values
    setupExecuteStage(MUL_INSTR, 5, 7, 3);  // rs=5, rt=7, dest=3

    // Execute the instruction
    sim->execute();

    // Verify the ALU result is correct (5 * 7 = 35)
    const PipelineStageData* execute_data = getExecuteStageData();
    EXPECT_EQ(execute_data->alu_result, 35);
    EXPECT_TRUE(execute_data->dest_reg.has_value());
    EXPECT_EQ(execute_data->dest_reg.value(), 3);
}

// Test AND instruction execution
TEST_F(ExecuteStageTest, ExecuteAND) {
    // Set up execute stage with AND instruction and test values
    setupExecuteStage(AND_INSTR, 0b1010, 0b1100, 3);  // rs=10, rt=12, dest=3

    // Execute the instruction
    sim->execute();

    // Verify the ALU result is correct (0b1010 & 0b1100 = 0b1000 = 8)
    const PipelineStageData* execute_data = getExecuteStageData();
    EXPECT_EQ(execute_data->alu_result, 8);
    EXPECT_TRUE(execute_data->dest_reg.has_value());
    EXPECT_EQ(execute_data->dest_reg.value(), 3);
}

// Test BEQ instruction execution with branch taken
TEST_F(ExecuteStageTest, ExecuteBEQTaken) {
    // Set up execute stage with BEQ instruction and equal values
    setupExecuteStage(BEQ_INSTR, 25, 25, std::nullopt, 1000);  // rs=25, rt=25, no dest, pc=1000

    // Execute the instruction
    sim->execute();

    // Verify branch was taken and target address calculated correctly
    const PipelineStageData* execute_data = getExecuteStageData();
    EXPECT_EQ(execute_data->alu_result, 1200);  // 1000 + (50*4) = 1200
    EXPECT_TRUE(sim->isBranchTaken());          // Use getter method
    EXPECT_FALSE(execute_data->dest_reg.has_value());  // No writeback for branches
}

// Test BEQ instruction execution with branch not taken
TEST_F(ExecuteStageTest, ExecuteBEQNotTaken) {
    // Set up execute stage with BEQ instruction and unequal values
    setupExecuteStage(BEQ_INSTR, 25, 30, std::nullopt, 1000);  // rs=25, rt=30, no dest, pc=1000

    // Execute the instruction
    sim->execute();

    // Verify branch was not taken
    const PipelineStageData* execute_data = getExecuteStageData();
    EXPECT_FALSE(sim->isBranchTaken());         // Use getter method
    EXPECT_FALSE(execute_data->dest_reg.has_value());
}

// Test BZ instruction with branch taken
TEST_F(ExecuteStageTest, ExecuteBZTaken) {
    // Set up execute stage with BZ instruction and zero value
    setupExecuteStage(BZ_INSTR, 0, 50, std::nullopt, 1000);  // rs=0, rt=50 (offset), no dest, pc=1000

    // Execute the instruction
    sim->execute();

    // Verify branch was taken (50*4 + 1000 = 1200)
    const PipelineStageData* execute_data = getExecuteStageData();
    EXPECT_EQ(execute_data->alu_result, 1200);
    EXPECT_TRUE(sim->isBranchTaken());          // Use getter method
    EXPECT_FALSE(execute_data->dest_reg.has_value());
}

// Test BZ instruction with branch not taken
TEST_F(ExecuteStageTest, ExecuteBZNotTaken) {
    // Set up execute stage with BZ instruction and non-zero value
    setupExecuteStage(BZ_INSTR, 5, 50, std::nullopt, 1000);  // rs=5 (non-zero), rt=50, no dest, pc=1000

    // Execute the instruction
    sim->execute();

    // Verify branch was not taken
    const PipelineStageData* execute_data = getExecuteStageData();
    EXPECT_FALSE(sim->isBranchTaken());         // Use getter method
    EXPECT_FALSE(execute_data->dest_reg.has_value());
}

// Test JR instruction execution
TEST_F(ExecuteStageTest, ExecuteJR) {
    // Set up execute stage with JR instruction
    setupExecuteStage(JR_INSTR, 2048, 0, std::nullopt);  // rs=2048 (jump target), rt=0, no dest

    // Execute the instruction
    sim->execute();

    // Verify jump address is correct (should be value in rs)
    const PipelineStageData* execute_data = getExecuteStageData();
    EXPECT_EQ(execute_data->alu_result, 2048);
    EXPECT_TRUE(sim->isBranchTaken());          // Use getter method
    EXPECT_FALSE(execute_data->dest_reg.has_value());
}

// Test LDW instruction execution (Load Word)
TEST_F(ExecuteStageTest, ExecuteLDW) {
    // Set up execute stage with LDW instruction
    setupExecuteStage(LDW_INSTR, 1000, 100, 8);  // rs=1000 (base), rt=100 (offset), dest=8

    // Execute the instruction
    sim->execute();

    // Verify the effective address is calculated correctly (1000 + 100 = 1100)
    const PipelineStageData* execute_data = getExecuteStageData();
    EXPECT_EQ(execute_data->alu_result, 1100);
    EXPECT_TRUE(execute_data->dest_reg.has_value());
    EXPECT_EQ(execute_data->dest_reg.value(), 8);
}