/**
 * @file fetch_stage_test.cpp
 * @brief Simple test for Fetch stage of pipeline
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
using ::testing::Return;

// Mock the memory parser
class MockMemoryParser : public IMemoryParser {
   public:
    MOCK_METHOD(uint32_t, readInstruction, (uint32_t address), (override));
    MOCK_METHOD(uint32_t, readMemory, (uint32_t address), (override));
    MOCK_METHOD(void, writeMemory, (uint32_t address, uint32_t value), (override));
};

class FetchStageTest : public ::testing::Test {
   protected:
    RegisterFile rf;
    Stats stats;
    NiceMock<MockMemoryParser> mem;
    std::unique_ptr<FunctionalSimulator> sim;

    // Reusing instruction encodings from your existing tests
    static constexpr uint32_t ADD_INSTR = 0x00221800;   // ADD $3, $1, $2
    static constexpr uint32_t ADDI_INSTR = 0x04850064;  // ADDI $5, $4, 100
    static constexpr uint32_t SUB_INSTR = 0x08221800;   // SUB $3, $1, $2
    static constexpr uint32_t HALT_INSTR = 0x44000000;  // HALT

    void SetUp() override { sim = std::make_unique<FunctionalSimulator>(&rf, &stats, &mem, false); }
};

TEST_F(FetchStageTest, FetchFourInstructions) {
    // Set up memory to return our 4 instructions in sequence
    EXPECT_CALL(mem, readInstruction(0x0)).WillOnce(Return(ADD_INSTR));
    EXPECT_CALL(mem, readInstruction(0x4)).WillOnce(Return(ADDI_INSTR));
    EXPECT_CALL(mem, readInstruction(0x8)).WillOnce(Return(SUB_INSTR));
    EXPECT_CALL(mem, readInstruction(0xC)).WillOnce(Return(HALT_INSTR));

    // Initially, PC should be 0 and fetch stage should be empty
    EXPECT_EQ(sim->getPC(), 0);
    EXPECT_TRUE(sim->isStageEmpty(FunctionalSimulator::PipelineStage::FETCH));

    // Fetch first instruction (ADD)
    sim->instructionFetch();
    EXPECT_EQ(sim->getPC(), 4);  // PC should increment
    EXPECT_FALSE(sim->isStageEmpty(FunctionalSimulator::PipelineStage::FETCH));

    const PipelineStageData* fetch_data =
        sim->getPipelineStage(FunctionalSimulator::PipelineStage::FETCH);
    ASSERT_NE(fetch_data, nullptr);
    EXPECT_EQ(fetch_data->instruction->getOpcode(), mips_lite::opcode::ADD);
    EXPECT_EQ(fetch_data->pc, 0);  // Should store the PC where instruction was fetched

    // Advance pipeline to make room for next fetch
    sim->advancePipeline();
    EXPECT_TRUE(sim->isStageEmpty(FunctionalSimulator::PipelineStage::FETCH));

    // Fetch second instruction (ADDI)
    sim->instructionFetch();
    EXPECT_EQ(sim->getPC(), 8);
    fetch_data = sim->getPipelineStage(FunctionalSimulator::PipelineStage::FETCH);
    ASSERT_NE(fetch_data, nullptr);
    EXPECT_EQ(fetch_data->instruction->getOpcode(), mips_lite::opcode::ADDI);
    EXPECT_EQ(fetch_data->pc, 4);

    // Advance pipeline again
    sim->advancePipeline();

    // Fetch third instruction (SUB)
    sim->instructionFetch();
    EXPECT_EQ(sim->getPC(), 12);
    fetch_data = sim->getPipelineStage(FunctionalSimulator::PipelineStage::FETCH);
    ASSERT_NE(fetch_data, nullptr);
    EXPECT_EQ(fetch_data->instruction->getOpcode(), mips_lite::opcode::SUB);
    EXPECT_EQ(fetch_data->pc, 8);

    // Advance pipeline again
    sim->advancePipeline();

    // Fetch fourth instruction (HALT) - should set halt_pipeline flag
    EXPECT_FALSE(sim->isHalted());  // Should not be halted yet
    sim->instructionFetch();
    EXPECT_EQ(sim->getPC(), 16);   // PC should increment once more after HALT
    EXPECT_TRUE(sim->isHalted());  // Now should be set to halt

    fetch_data = sim->getPipelineStage(FunctionalSimulator::PipelineStage::FETCH);
    ASSERT_NE(fetch_data, nullptr);
    EXPECT_EQ(fetch_data->instruction->getOpcode(), mips_lite::opcode::HALT);
    EXPECT_EQ(fetch_data->pc, 12);

    // Try to fetch again - should do nothing because halt_pipeline is true
    sim->advancePipeline();
    sim->instructionFetch();
    EXPECT_EQ(sim->getPC(), 16);  // PC should increment once more after HALT
    EXPECT_TRUE(sim->isStageEmpty(FunctionalSimulator::PipelineStage::FETCH));  // Should not fetch
}
