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

TEST_F(FunctionalSimulatorTest, ClockPipelineRegistersUpdatesRegisterStates) {
    // Create a known register file state for 2 different pipeline registers
    RegisterFile if_id;
    if_id.write(5, 42);  // write value 42 to R5

    RegisterFile ex_mem;
    ex_mem.write(8, 1000);  // write value 1000 to R8

    // Confirm neither pipeline register has valid values prior to clocking
    // in values from next
    sim->ifid_reg.setNext(if_id);
    EXPECT_FALSE(sim->ifid_reg.isValid());

    sim->exmem_reg.setNext(ex_mem);
    EXPECT_FALSE(sim->exmem_reg.isValid());

    // Clock all pipeline registers
    sim->clockPipelineRegisters();

    // Check that pipelines registers have been updated
    EXPECT_TRUE(sim->ifid_reg.isValid());
    EXPECT_EQ(sim->ifid_reg.current().read(5), 42);

    EXPECT_TRUE(sim->exmem_reg.isValid());
    EXPECT_EQ(sim->exmem_reg.current().read(8), 1000);
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
