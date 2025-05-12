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

TEST_F(FunctionalSimulatorTest, ConstructorInitializesMembers) {
    EXPECT_EQ(sim->getPC(), 0);
    EXPECT_EQ(sim->getStall(), 0);
    EXPECT_FALSE(sim->isForwardingEnabled());

    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(sim->getPipelineStage(i), nullptr);
    }
}

TEST_F(FunctionalSimulatorTest, ConstructorEnablesForwarding) {
    FunctionalSimulator forward_sim(&rf, &stats, &mem, true);
    EXPECT_TRUE(forward_sim.isForwardingEnabled());
}

TEST_F(FunctionalSimulatorTest, SettersUpdateInternalState) {
    sim->setPC(0x00400020);
    sim->setStall(3);

    EXPECT_EQ(sim->getPC(), 0x00400020);
    EXPECT_EQ(sim->getStall(), 3);
}

TEST_F(FunctionalSimulatorTest, ThrowsOnInvalidPipelineStageIndex) {
    EXPECT_THROW(sim->getPipelineStage(-1), std::out_of_range);
    EXPECT_THROW(sim->getPipelineStage(5), std::out_of_range);
}

/*
// The following or something similar may be used to test individual methods
// with mocked memory parser methods like readInstruction
// Requires addition of `using ::testing::Return;` above
TEST_F(FunctionalSimulatorTest, FetchReadsInstructionFromMemory) {
    EXPECT_CALL(mem, readInstruction(0)).Times(1).WillOnce(Return(0x040103E8));
    sim->instructionFetch();  // Assuming it internally calls readInstruction
}
*/
