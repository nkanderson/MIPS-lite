/**
 * @file exe_stage_tests.cpp
 * @brief Tests for Execute stage of pipeline
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

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

    void SetUp() override { sim = std::make_unique<FunctionalSimulator>(&rf, &stats, &mem); }

    // Helper to properly initialize a pipeline register with an instruction
    void setupExecuteStage(uint32_t instruction_word, int32_t reg_a_value, int32_t reg_b_value,
                           std::optional<uint8_t> wb_reg) {
        // Create a new instruction object
        Instruction* instr = new Instruction(instruction_word);

        // Create pipeline data
        PipelineData<uint32_t> data;
        data.instr = instr;
        data.reg_a = reg_a_value;
        data.reg_b = reg_b_value;
        data.wb_reg = wb_reg;
        data.branch_taken = std::nullopt;
        data.result = 0;

        // Initialize the pipeline register
        sim->idex_reg.setNext(data);

        // Clock the pipeline
        sim->clockPipelineRegisters();

        // Verify the register is now valid
        ASSERT_TRUE(sim->idex_reg.isValid()) << "ID/EX register wasn't properly initialized";
    }
};

// Test ADD instruction execution
TEST_F(ExecuteStageTest, ExecuteADD) {
    // Using the correct opcode for ADD (0x00)
    uint32_t add_instr = 0x00221800;  // ADD $3, $1, $2

    // Set up the ID/EX register with test values
    setupExecuteStage(add_instr, 10, 20, 3);  // reg_a=10, reg_b=20, wb_reg=3

    // Execute the instruction
    sim->execute(sim->idex_reg.current().instr);

    // Clock the pipeline to move result to exmem_reg.current()
    sim->clockPipelineRegisters();

    // Verify the result is correct (10 + 20 = 30)
    EXPECT_EQ(sim->exmem_reg.current().result, 30);
    EXPECT_TRUE(sim->exmem_reg.current().wb_reg.has_value());
    EXPECT_EQ(sim->exmem_reg.current().wb_reg.value(), 3);
}

// Test SUB instruction execution
TEST_F(ExecuteStageTest, ExecuteSUB) {
    // Using the correct opcode for SUB (0x02)
    uint32_t sub_instr = 0x08221800;  // SUB $3, $1, $2

    // Set up the ID/EX register with test values
    setupExecuteStage(sub_instr, 30, 12, 3);  // reg_a=30, reg_b=12, wb_reg=3

    // Execute the instruction
    sim->execute(sim->idex_reg.current().instr);

    // Clock the pipeline
    sim->clockPipelineRegisters();

    // Verify the result is correct (30 - 12 = 18)
    EXPECT_EQ(sim->exmem_reg.current().result, 18);
    EXPECT_TRUE(sim->exmem_reg.current().wb_reg.has_value());
    EXPECT_EQ(sim->exmem_reg.current().wb_reg.value(), 3);
}

// Test MUL instruction execution
TEST_F(ExecuteStageTest, ExecuteMUL) {
    // Using the correct opcode for MUL (0x04)
    uint32_t mul_instr = 0x10221800;  // MUL $3, $1, $2

    // Set up the ID/EX register with test values
    setupExecuteStage(mul_instr, 5, 7, 3);  // reg_a=5, reg_b=7, wb_reg=3

    // Execute the instruction
    sim->execute(sim->idex_reg.current().instr);

    // Clock the pipeline
    sim->clockPipelineRegisters();

    // Verify the result is correct (5 * 7 = 35)
    EXPECT_EQ(sim->exmem_reg.current().result, 35);
    EXPECT_TRUE(sim->exmem_reg.current().wb_reg.has_value());
    EXPECT_EQ(sim->exmem_reg.current().wb_reg.value(), 3);
}

// Test BEQ instruction execution with branch taken
TEST_F(ExecuteStageTest, ExecuteBEQTaken) {
    // Using the correct opcode for BEQ (0x0F)
    uint32_t beq_instr = 0x3C280032;  // BEQ $1, $8, 50 (branch if $1 == $8, offset 50)

    // Set up simulator's PC
    sim->setPC(1000);

    // Set up the ID/EX register with equal values to trigger branch
    setupExecuteStage(beq_instr, 25, 25, std::nullopt);  // reg_a=25, reg_b=25, no wb_reg

    // Execute the instruction
    sim->execute(sim->idex_reg.current().instr);

    // Clock the pipeline
    sim->clockPipelineRegisters();

    // Verify branch was taken (50*4 + 1000 = 1200)
    EXPECT_EQ(sim->exmem_reg.current().result, 1200);
    EXPECT_TRUE(sim->exmem_reg.current().branch_taken.has_value());
    EXPECT_TRUE(sim->exmem_reg.current().branch_taken.value());
    EXPECT_FALSE(sim->exmem_reg.current().wb_reg.has_value());
}

// Test BEQ instruction execution with branch not taken
TEST_F(ExecuteStageTest, ExecuteBEQNotTaken) {
    // Using the correct opcode for BEQ (0x0F)
    uint32_t beq_instr = 0x3C280032;  // BEQ $1, $8, 50 (branch if $1 == $8, offset 50)

    // Set up simulator's PC
    sim->setPC(1000);

    // Set up the ID/EX register with unequal values to avoid branch
    setupExecuteStage(beq_instr, 25, 30, std::nullopt);  // reg_a=25, reg_b=30, no wb_reg

    // Execute the instruction
    sim->execute(sim->idex_reg.current().instr);

    // Clock the pipeline
    sim->clockPipelineRegisters();

    // Verify branch was not taken
    EXPECT_TRUE(sim->exmem_reg.current().branch_taken.has_value());
    EXPECT_FALSE(sim->exmem_reg.current().branch_taken.value());
    EXPECT_FALSE(sim->exmem_reg.current().wb_reg.has_value());
}

// Test JR instruction execution
TEST_F(ExecuteStageTest, ExecuteJR) {
    // Using the correct opcode for JR (0x10)
    uint32_t jr_instr = 0x40280000;  // JR $5 (jump to address in $5)

    // Set up the ID/EX register with jump target address
    setupExecuteStage(jr_instr, 2048, 0, std::nullopt);  // reg_a=2048 (jump target), no wb_reg

    // Execute the instruction
    sim->execute(sim->idex_reg.current().instr);

    // Clock the pipeline
    sim->clockPipelineRegisters();

    // Verify jump address is correct (should be value in reg_a)
    EXPECT_EQ(sim->exmem_reg.current().result, 2048);
    EXPECT_TRUE(sim->exmem_reg.current().branch_taken.has_value());
    EXPECT_TRUE(sim->exmem_reg.current().branch_taken.value());
    EXPECT_FALSE(sim->exmem_reg.current().wb_reg.has_value());
}

// Test logical operation: AND instruction
TEST_F(ExecuteStageTest, ExecuteAND) {
    // Using the correct opcode for AND (0x08)
    uint32_t and_instr = 0x20221800;  // AND $3, $1, $2

    // Set up the ID/EX register with test values
    setupExecuteStage(and_instr, 0b1010, 0b1100, 3);  // reg_a=10, reg_b=12, wb_reg=3

    // Execute the instruction
    sim->execute(sim->idex_reg.current().instr);

    // Clock the pipeline
    sim->clockPipelineRegisters();

    // Verify the result is correct (0b1010 & 0b1100 = 0b1000 = 8)
    EXPECT_EQ(sim->exmem_reg.current().result, 8);
    EXPECT_TRUE(sim->exmem_reg.current().wb_reg.has_value());
    EXPECT_EQ(sim->exmem_reg.current().wb_reg.value(), 3);
}

// Test memory operation: LDW instruction (Load Word)
TEST_F(ExecuteStageTest, ExecuteLDW) {
    // Using the correct opcode for LDW (0x0C)
    uint32_t ldw_instr = 0x30480064;  // LDW $8, 100($2) (load word from address $2+100 into $8)

    // Set up the ID/EX register with test values
    setupExecuteStage(ldw_instr, 1000, 100,
                      8);  // reg_a=1000 (base), reg_b=100 (offset), wb_reg=8 (dest)

    // Execute the instruction
    sim->execute(sim->idex_reg.current().instr);

    // Clock the pipeline
    sim->clockPipelineRegisters();

    // Verify the effective address is correct (1000 + 100 = 1100)
    EXPECT_EQ(sim->exmem_reg.current().result, 1100);
    EXPECT_TRUE(sim->exmem_reg.current().wb_reg.has_value());
    EXPECT_EQ(sim->exmem_reg.current().wb_reg.value(), 8);
}

// Test BZ instruction with branch taken
TEST_F(ExecuteStageTest, ExecuteBZTaken) {
    // Using the correct opcode for BZ (0x0E)
    uint32_t bz_instr = 0x38280032;  // BZ $1, 50 (branch if $1 == 0, offset 50)

    // Set up simulator's PC
    sim->setPC(1000);

    // Set up the ID/EX register with zero to trigger branch
    setupExecuteStage(bz_instr, 0, 50, std::nullopt);  // reg_a=0, reg_b=50 (offset), no wb_reg

    // Execute the instruction
    sim->execute(sim->idex_reg.current().instr);

    // Clock the pipeline
    sim->clockPipelineRegisters();

    // Verify branch was taken (50*4 + 1000 = 1200)
    EXPECT_EQ(sim->exmem_reg.current().result, 1200);
    EXPECT_TRUE(sim->exmem_reg.current().branch_taken.has_value());
    EXPECT_TRUE(sim->exmem_reg.current().branch_taken.value());
    EXPECT_FALSE(sim->exmem_reg.current().wb_reg.has_value());
}
