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

    void SetUp() override {
        sim = std::make_unique<FunctionalSimulator>(&rf, &stats, &mem);

        // Initialize some register values for testing
        rf.write(1, 100);  // $1 = 100
        rf.write(2, 200);  // $2 = 200
        rf.write(3, 300);  // $3 = 300
        rf.write(4, 400);  // $4 = 400
        rf.write(5, 500);  // $5 = 500
    }

    // Helper to properly initialize a pipeline register with an instruction
    void setupDecodeStage(uint32_t instruction_word) {
        // Create a new instruction object
        Instruction* instr = new Instruction(instruction_word);

        // Create pipeline data for IF/ID register
        PipelineData<uint32_t> data;
        data.instr = instr;
        data.result = 0;

        // Initialize the pipeline register
        sim->ifid_reg.setNext(data);

        // Clock the pipeline to make it current
        sim->clockPipelineRegisters();

        // Verify the register is now valid
        ASSERT_TRUE(sim->ifid_reg.isValid()) << "IF/ID register wasn't properly initialized";
    }
};

// Test R-type instruction decode (ADD)
TEST_F(DecodeStageTest, DecodeRTypeInstruction) {
    // Using ADD instruction: ADD $3, $1, $2
    uint32_t add_instr = 0x00221800;  // opcode=0, rs=1, rt=2, rd=3

    // Set up the IF/ID register
    setupDecodeStage(add_instr);

    // Decode the instruction
    sim->instructionDecode(sim->ifid_reg.current().instr);

    // Clock the pipeline
    sim->clockPipelineRegisters();

    // Verify decode results in ID/EX register
    EXPECT_EQ(sim->idex_reg.current().reg_a, 100);  // Value of $1
    EXPECT_EQ(sim->idex_reg.current().reg_b, 200);  // Value of $2
    EXPECT_TRUE(sim->idex_reg.current().wb_reg.has_value());
    EXPECT_EQ(sim->idex_reg.current().wb_reg.value(), 3);            // Write-back to $3
    EXPECT_FALSE(sim->idex_reg.current().branch_taken.has_value());  // Not a branch
}

// Test I-type instruction decode (ADDI)
TEST_F(DecodeStageTest, DecodeITypeInstruction) {
    // Using ADDI instruction: ADDI $3, $1, 50
    uint32_t addi_instr = 0x04230032;  // opcode=1, rs=1, rt=3, imm=50

    // Set up the IF/ID register
    setupDecodeStage(addi_instr);

    // Decode the instruction
    sim->instructionDecode(sim->ifid_reg.current().instr);

    // Clock the pipeline
    sim->clockPipelineRegisters();

    // Verify decode results in ID/EX register
    EXPECT_EQ(sim->idex_reg.current().reg_a, 100);  // Value of $1
    EXPECT_EQ(sim->idex_reg.current().reg_b, 50);   // Immediate value
    EXPECT_TRUE(sim->idex_reg.current().wb_reg.has_value());
    EXPECT_EQ(sim->idex_reg.current().wb_reg.value(), 3);            // Write-back to $3
    EXPECT_FALSE(sim->idex_reg.current().branch_taken.has_value());  // Not a branch
}

// Test memory instruction decode (LDW)
TEST_F(DecodeStageTest, DecodeMemoryInstruction) {
    // Using LDW instruction: LDW $3, 100($1)
    uint32_t ldw_instr = 0x30230064;  // opcode=12, rs=1, rt=3, imm=100

    // Set up the IF/ID register
    setupDecodeStage(ldw_instr);

    // Decode the instruction
    sim->instructionDecode(sim->ifid_reg.current().instr);

    // Clock the pipeline
    sim->clockPipelineRegisters();

    // Verify decode results in ID/EX register
    EXPECT_EQ(sim->idex_reg.current().reg_a, 100);  // Value of $1 (base address)
    EXPECT_EQ(sim->idex_reg.current().reg_b, 100);  // Immediate value (offset)
    EXPECT_TRUE(sim->idex_reg.current().wb_reg.has_value());
    EXPECT_EQ(sim->idex_reg.current().wb_reg.value(), 3);            // Write-back to $3
    EXPECT_FALSE(sim->idex_reg.current().branch_taken.has_value());  // Not a branch
}

// Test store instruction decode (STW)
TEST_F(DecodeStageTest, DecodeStoreInstruction) {
    // Using STW instruction: STW $3, 100($1)
    uint32_t stw_instr = 0x34230064;  // opcode=13, rs=1, rt=3, imm=100

    // Set up the IF/ID register
    setupDecodeStage(stw_instr);

    // Decode the instruction
    sim->instructionDecode(sim->ifid_reg.current().instr);

    // Clock the pipeline
    sim->clockPipelineRegisters();

    // Verify decode results in ID/EX register
    EXPECT_EQ(sim->idex_reg.current().reg_a, 100);                   // Value of $1 (base address)
    EXPECT_EQ(sim->idex_reg.current().reg_b, 100);                   // Immediate value (offset)
    EXPECT_FALSE(sim->idex_reg.current().wb_reg.has_value());        // No write-back for store
    EXPECT_FALSE(sim->idex_reg.current().branch_taken.has_value());  // Not a branch
}

// Test branch instruction decode (BEQ)
TEST_F(DecodeStageTest, DecodeBranchEqualInstruction) {
    // Using BEQ instruction: BEQ $1, $2, 50
    uint32_t beq_instr = 0x3C220032;  // opcode=15, rs=1, rt=2, imm=50

    // Set up the IF/ID register
    setupDecodeStage(beq_instr);

    // Decode the instruction
    sim->instructionDecode(sim->ifid_reg.current().instr);

    // Clock the pipeline
    sim->clockPipelineRegisters();

    // Verify decode results in ID/EX register
    EXPECT_EQ(sim->idex_reg.current().reg_a, 100);             // Value of $1
    EXPECT_EQ(sim->idex_reg.current().reg_b, 200);             // Value of $2 (for comparison)
    EXPECT_FALSE(sim->idex_reg.current().wb_reg.has_value());  // No write-back for branch
    EXPECT_TRUE(sim->idex_reg.current().branch_taken.has_value());
    EXPECT_FALSE(sim->idex_reg.current().branch_taken.value());  // Initially false
}

// Test branch zero instruction decode (BZ)
TEST_F(DecodeStageTest, DecodeBranchZeroInstruction) {
    // Using BZ instruction: BZ $1, 50
    uint32_t bz_instr = 0x38220032;  // opcode=14, rs=1, rt=x, imm=50

    // Set up the IF/ID register
    setupDecodeStage(bz_instr);

    // Decode the instruction
    sim->instructionDecode(sim->ifid_reg.current().instr);

    // Clock the pipeline
    sim->clockPipelineRegisters();

    // Verify decode results in ID/EX register
    EXPECT_EQ(sim->idex_reg.current().reg_a, 100);             // Value of $1
    EXPECT_EQ(sim->idex_reg.current().reg_b, 50);              // Immediate value (offset)
    EXPECT_FALSE(sim->idex_reg.current().wb_reg.has_value());  // No write-back for branch
    EXPECT_TRUE(sim->idex_reg.current().branch_taken.has_value());
    EXPECT_FALSE(sim->idex_reg.current().branch_taken.value());  // Initially false
}

// Test jump register instruction decode (JR)
TEST_F(DecodeStageTest, DecodeJumpRegisterInstruction) {
    // Using JR instruction: JR $5
    uint32_t jr_instr = 0x40A00000;  // opcode=16, rs=5

    // Set up the IF/ID register
    setupDecodeStage(jr_instr);

    // Decode the instruction
    sim->instructionDecode(sim->ifid_reg.current().instr);

    // Clock the pipeline
    sim->clockPipelineRegisters();

    // Verify decode results in ID/EX register
    EXPECT_EQ(sim->idex_reg.current().reg_a, 500);             // Value of $5 (jump target)
    EXPECT_FALSE(sim->idex_reg.current().wb_reg.has_value());  // No write-back for jump
    EXPECT_TRUE(sim->idex_reg.current().branch_taken.has_value());
    EXPECT_FALSE(sim->idex_reg.current().branch_taken.value());  // Initially false
}

// Test stall behavior with hazards
TEST_F(DecodeStageTest, StallOnDataHazard) {
    // First, set up a register in the EX/MEM stage that will create a hazard
    PipelineData<uint32_t> exmem_data;
    exmem_data.wb_reg = 1;  // Write-back to register $1
    exmem_data.result = 999;
    exmem_data.instr = new Instruction(0);  // Any instruction

    sim->exmem_reg.setNext(exmem_data);
    sim->clockPipelineRegisters();

    // Now set up an instruction that uses $1
    uint32_t add_instr = 0x00221800;  // ADD $3, $1, $2

    // Set up the IF/ID register
    setupDecodeStage(add_instr);

    // Call decode - should detect hazard and set stall
    sim->instructionDecode(sim->ifid_reg.current().instr);

    // Verify a stall was set
    if (!sim->isForwardingEnabled()) {
        EXPECT_EQ(sim->getStall(), 2);  // Two cycles of stall for hazard
    } else {
        EXPECT_EQ(sim->getStall(), 0);  // No stall if forwarding is enabled
    }

    // Clock pipleine and check if stall is decremented
    if (!sim->isForwardingEnabled()) {
        sim->clockPipelineRegisters();
        sim->instructionDecode(sim->ifid_reg.current().instr);
        EXPECT_EQ(sim->getStall(), 1);  // Stall should be decremented
        sim->clockPipelineRegisters();
        sim->instructionDecode(sim->ifid_reg.current().instr);
        EXPECT_EQ(sim->getStall(), 0);  // Stall should be decremented to 0
    }
}

// Test forwarding is disabled by default
TEST_F(DecodeStageTest, ForwardingDisabledByDefault) { EXPECT_FALSE(sim->isForwardingEnabled()); }

// Test bubble insertion during stall
TEST_F(DecodeStageTest, BubbleInsertionDuringStall) {
    // Set a stall count
    sim->setStall(2);

    // Set up any instruction
    uint32_t add_instr = 0x00221800;  // ADD $3, $1, $2
    setupDecodeStage(add_instr);

    // Call decode - should insert bubble due to stall
    sim->instructionDecode(sim->ifid_reg.current().instr);

    // Clock the pipeline
    sim->clockPipelineRegisters();

    // Verify a bubble was inserted (null instruction)
    EXPECT_EQ(sim->idex_reg.current().instr, nullptr);
    EXPECT_EQ(sim->getStall(), 1);  // Stall should be decremented
}
