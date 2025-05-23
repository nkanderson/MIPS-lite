#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "functional_simulator.h"
#include "memory_interface.h"
#include "mips_instruction.h"
#include "register_file.h"
#include "stats.h"

using ::testing::NiceMock;

// ---------------------------
// Mock classes
// ---------------------------

/**
 * @brief Only MemoryParser is mocked for now.
 *
 * We use a mock for IMemoryParser to avoid file I/O during testing and to simulate
 * custom memory access behavior (e.g., returning specific instruction values, simulating faults).
 *
 * The Stats and RegisterFile classes are not mocked because:
 *  - They are small, fast, and side-effect-free.
 *  - Their state is easy to inspect and assert directly (e.g., instruction counts, register
 * values).
 *
 * We might choose to mock Stats or RegisterFile later if:
 *  - We want to test interactions (e.g., how often `incrementStalls()` is called).
 *  - Forwarding logic or hazard detection requires precise call tracking.
 */

class MockMemoryParser : public IMemoryParser {
   public:
    MOCK_METHOD(uint32_t, readInstruction, (uint32_t address), (override));
    MOCK_METHOD(uint32_t, readMemory, (uint32_t address), (override));
    MOCK_METHOD(void, writeMemory, (uint32_t address, uint32_t value), (override));
};

// ---------------------------
// Test fixture
// ---------------------------

class FunctionalSimulatorTest : public ::testing::Test {
   protected:
    RegisterFile rf;
    Stats stats;
    NiceMock<MockMemoryParser> mem;
    std::unique_ptr<FunctionalSimulator> sim;

    void SetUp() override { sim = std::make_unique<FunctionalSimulator>(&rf, &stats, &mem); }
};

// ---------------------------
// Test suite
// ---------------------------

TEST_F(FunctionalSimulatorTest, Initialization) {
    EXPECT_EQ(sim->getPC(), 0);
    EXPECT_EQ(sim->getStall(), 0);
    EXPECT_FALSE(sim->isForwardingEnabled());
}

// Test forwarding flag
TEST_F(FunctionalSimulatorTest, ForwardingFlag) {
    FunctionalSimulator forward_sim(&rf, &stats, &mem, true);
    EXPECT_TRUE(forward_sim.isForwardingEnabled());
}

// Test setters and getters
TEST_F(FunctionalSimulatorTest, SettersAndGetters) {
    sim->setPC(0x00400020);
    sim->setStall(3);

    EXPECT_EQ(sim->getPC(), 0x00400020);
    EXPECT_EQ(sim->getStall(), 3);
}

// Test pipeline stages initially empty
TEST_F(FunctionalSimulatorTest, PipelineInitiallyEmpty) {
    for (int i = 0; i < FunctionalSimulator::getNumStages(); ++i) {
        EXPECT_TRUE(sim->isStageEmpty(i));
        EXPECT_EQ(sim->getPipelineStage(i), nullptr);
    }
}

// Test bounds checking
TEST_F(FunctionalSimulatorTest, BoundsChecking) {
    EXPECT_THROW(sim->isStageEmpty(-1), std::out_of_range);
    EXPECT_THROW(sim->isStageEmpty(5), std::out_of_range);
    EXPECT_THROW(sim->getPipelineStage(-1), std::out_of_range);
    EXPECT_THROW(sim->getPipelineStage(5), std::out_of_range);
}

// Test stall counter decrements
TEST_F(FunctionalSimulatorTest, StallDecrement) {
    sim->setStall(2);
    EXPECT_EQ(sim->getStall(), 2);

    sim->advancePipeline();
    EXPECT_EQ(sim->getStall(), 1);

    sim->advancePipeline();
    EXPECT_EQ(sim->getStall(), 0);

    // Should not go below zero
    sim->advancePipeline();
    EXPECT_EQ(sim->getStall(), 0);
}

// Test that getPipelineStage returns nullptr for empty stages
TEST_F(FunctionalSimulatorTest, GetPipelineStage) {
    for (int i = 0; i < FunctionalSimulator::getNumStages(); ++i) {
        EXPECT_EQ(sim->getPipelineStage(i), nullptr);
    }
}

/**
 * @test WriteBackWritesToRegisterAndUpdatesStats
 * @brief Verifies that the writeBack() method correctly writes the ALU result to the
 * destination register and records the register in the stats tracker.
 */
TEST_F(FunctionalSimulatorTest, WriteBackWritesToRegisterAndUpdatesStats) {
    // Set up known values
    uint8_t dest_reg = 8;
    uint32_t expected_value = 0x12345678;

    // Manually populate WRITEBACK stage with a PipelineStageData pointer
    // that has our desired data.
    auto data = std::make_unique<PipelineStageData>();
    data->alu_result = expected_value;
    data->dest_reg = dest_reg;
    data->instruction = std::make_unique<Instruction>(0x6789ABCD);
    sim->advancePipeline();
    sim->getPipeline()[FunctionalSimulator::WRITEBACK] = std::move(data);

    sim->writeBack();

    // Check if value was written to the correct register
    EXPECT_EQ(rf.read(dest_reg), expected_value);

    // Check if stats updated with the same register
    const auto& modified_regs = stats.getRegisters();
    EXPECT_EQ(modified_regs.size(), 1);
    EXPECT_TRUE(modified_regs.count(dest_reg));
}

/**
 * @test WriteBackEmptyDestRegReturns
 * @brief Ensures that writeBack() performs no action if the destination register is not set,
 * leaving the register file and stats unchanged.
 */
TEST_F(FunctionalSimulatorTest, WriteBackEmptyDestRegReturns) {
    // Set up known value for ALU result
    uint32_t expected_value = 0x12345678;

    // NOTE: No dest_reg is set here, so writeBack should return without
    // writing to the register file.
    auto data = std::make_unique<PipelineStageData>();
    data->alu_result = expected_value;
    data->instruction = std::make_unique<Instruction>(0x6789ABCD);
    sim->advancePipeline();
    sim->getPipeline()[FunctionalSimulator::WRITEBACK] = std::move(data);

    sim->writeBack();

    // stats should show no modified registers
    const auto& modified_regs = stats.getRegisters();
    EXPECT_EQ(modified_regs.size(), 0);
}

/*
// The following or something similar may be used to test individual methods
// with mocked memory parser methods like readInstruction
// Requires addition of `using ::testing::Return;` above
TEST_F(FunctionalSimulatorTest, FetchReadsInstructionFromMemory) {
    EXPECT_CALL(mem, readInstruction(0)).Times(1).WillOnce(Return(0x040103E8));
    sim->instructionFetch();  // Assuming it internally calls readInstruction
*/
