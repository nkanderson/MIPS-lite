#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "functional_simulator.h"
#include "memory_interface.h"
#include "mips_instruction.h"
#include "register_file.h"
#include "stats.h"

using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

// ---------------------------
// Basic setup
// ---------------------------

class MockMemoryParser : public IMemoryParser {
   public:
    MOCK_METHOD(uint32_t, readInstruction, (uint32_t address), (override));
    MOCK_METHOD(uint32_t, readMemory, (uint32_t address), (override));
    MOCK_METHOD(void, writeMemory, (uint32_t address, uint32_t value), (override));
};

class IntegrationTest : public ::testing::Test {
   protected:
    RegisterFile rf;
    Stats stats;
    NiceMock<MockMemoryParser> mem;
    std::unique_ptr<FunctionalSimulator> sim;

    void SetUp() override { sim = std::make_unique<FunctionalSimulator>(&rf, &stats, &mem); }
};

// ---------------------------
// Helper Functions
// ---------------------------

/**
 * @brief Runs through a single simulator clock cycle.
 * Should be the same sequence of calls as in our main program loop.
 */
void advancePipeline(FunctionalSimulator* sim) {
    sim->writeBack(sim->getPipelineStage(4));
    sim->memory(sim->getPipelineStage(3));
    sim->execute(sim->getPipelineStage(2));
    sim->instructionDecode(sim->getPipelineStage(1));
    // FIXME: Update instructionFetch to accept a pointer (which might be null)
    sim->instructionFetch(sim->getPipelineStage(0));
    // FIXME: Create functional simulator clock method
    sim->clock();
}

/**
 * @brief Set up mocking for an array of hex-encoded instructions.
 * Does not require calls to be made in order.
 */
void setupMockMemory(MockMemoryParser& mem, const std::vector<uint32_t>& memory,
                     uint32_t base = 0x0) {
    // Lambda function for our expectations below
    auto memory_lookup = [&memory, base](uint32_t addr) -> uint32_t {
        size_t index = (addr - base) / 4;
        if ((addr - base) % 4 != 0) {
            ADD_FAILURE() << "Unaligned memory access at address: 0x" << std::hex << addr;
            throw std::runtime_error("Unaligned access");
        }
        if (index >= memory.size()) {
            ADD_FAILURE() << "Memory access out of bounds at address: 0x" << std::hex << addr;
            throw std::out_of_range("Access outside memory vector");
        }
        return memory[index];
    };

    EXPECT_CALL(mem, readInstruction(::testing::_))
        .WillRepeatedly(::testing::Invoke(memory_lookup));

    EXPECT_CALL(mem, readMemory(::testing::_)).WillRepeatedly(::testing::Invoke(memory_lookup));
}

// ---------------------------
// Test suite
// ---------------------------

/**
 * @brief Example showing required in-order instruction access
 * Seems less likely we'll use this option, but we have it here
 * as an example in case.
 */
TEST_F(IntegrationTest, BeqNotTakenInOrderExample) {
    // This is an alternative to using setupMockMemory, in a case where we want
    // to test that readInstruction and readMemory are called in a particular order.
    {
        // seq ensures calls happen in order. The test will fail if the methods
        // are called out of the order specified below.
        InSequence seq;

        // Fetch first instruction: ADDI R1 R0 4
        EXPECT_CALL(mem, readInstruction(0x0)).WillOnce(Return(0x04010004));
        // Fetch second instruction: ADDI R2 R0 4
        EXPECT_CALL(mem, readInstruction(0x4)).WillOnce(Return(0x04020004));
        // Fetch third instruction: BEQ R1 R2 2
        EXPECT_CALL(mem, readInstruction(0x4)).WillOnce(Return(0x3c410002));
        // Fetch fourth instruction: ADDI R1 R0 6
        EXPECT_CALL(mem, readInstruction(0x4)).WillOnce(Return(0x04010006));
        // Fetch fifth instruction: ADDI R1 R0 10
        EXPECT_CALL(mem, readInstruction(0x4)).WillOnce(Return(0x0401000a));
        // Fetch sixth instruction: HALT
        EXPECT_CALL(mem, readInstruction(0x4)).WillOnce(Return(0x44000000));

        // Examples for a test that reads memory, not used in BeqNotTaken test
        // Simulated memory at 0x1000 returns 42 for lw
        // EXPECT_CALL(mem, readMemory(0x1000)).WillOnce(Return(42));
    }

    while (!sim->getPipelineStage(4)->isHaltInstruction()) {
        advancePipeline(sim.get());
    }

    // ---------------------
    // Check Results
    // ---------------------

    EXPECT_EQ(rf.read(1), 20);
    EXPECT_EQ(rf.read(2), 8);
    EXPECT_EQ(stats.totalInstructions(), 6);
}

TEST_F(IntegrationTest, BeqNotTakenNoForwarding) {
    std::vector<uint32_t> program = {
        0x04010004,  // ADDI R1 R0 4
        0x04020004,  // ADDI R2 R0 4
        0x3c410002,  // BEQ R1 R2 2
        0x04010006,  // ADDI R1 R0 6
        0x0401000a,  // ADDI R1 R0 10
        0x44000000   // HALT
    };

    setupMockMemory(mem, program);

    // Examples for a test that reads / writes memory, not used in BeqNotTaken test
    // EXPECT_CALL(mem, readMemory(0x0)).WillOnce(Return(42));
    // EXPECT_CALL(mem, writeMemory(0x4, 100)).Times(1);

    // Example of directly writing to the register file as setup, instead of including
    // all the instructions necessary for a small program. Might be useful for more
    // complex tests where we don't want to include all instructions for setup.
    // rf.write(9, 58);  // R9 = 58

    // TODO: Determine when HALT takes effect - when it's in writeback, or immediately
    // after being fetched, or in decode? And adjust this or above instructions as
    // needed to ensure all of our desired test instruction complete execution.
    while (!sim->getPipelineStage(4)->isHaltInstruction()) {
        advancePipeline(sim.get());
    }

    // ---------------------
    // Check Results
    // ---------------------

    EXPECT_EQ(rf.read(1), 20);
    EXPECT_EQ(rf.read(2), 8);
    EXPECT_EQ(stats.totalInstructions(), 6);
    // TODO: Add checks by category
    // TODO: Add check for registers R1 and R2 listed as changed
    // TODO: Add check that no memory address was listed as changed
    // TODO: Add check for number of stalls
}
