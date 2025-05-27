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
    std::unique_ptr<FunctionalSimulator> sim_no_forward;
    std::unique_ptr<FunctionalSimulator> sim_with_forward;

    void SetUp() override { 
        sim_no_forward = std::make_unique<FunctionalSimulator>(&rf, &stats, &mem, false);
        sim_with_forward = std::make_unique<FunctionalSimulator>(&rf, &stats, &mem, true);
    }
};

// ---------------------------
// Helper Functions (Updated for new cycle logic)
// ---------------------------

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

    EXPECT_CALL(mem, readMemory(::testing::_))
        .WillRepeatedly(::testing::Invoke(memory_lookup));
}

/**
 * @brief Helper to reset simulator state for multiple test runs
 */
void resetSimulator(std::unique_ptr<FunctionalSimulator>& sim, RegisterFile& rf, Stats& stats, 
                   MockMemoryParser& mem, bool enable_forwarding) {
    rf = RegisterFile();  // Reset register file
    stats = Stats();      // Reset stats
    sim = std::make_unique<FunctionalSimulator>(&rf, &stats, &mem, enable_forwarding);
}

// ---------------------------
// Test suite (Updated for new functionality)
// ---------------------------

/**
 * @brief Test branch not taken with both forwarding and no forwarding. 
 * Results: 
 * - No forwarding
 *    R1 = 20 , PC = 16 
 *    Cycles = 13
 *    Stalls = 4
 *    Branch Penalty = 0 
 * - Forwarding
 *    R1 = 20 , PC = 16
 *    Cycles = 9
 *    Stalls = 0
 *    Branch Penalty = 0
 *   
 */
TEST_F(IntegrationTest, BZNotTaken) {
    std::vector<uint32_t> program = {
        0x04010004,  // ADDI R1 R0 4
        0x38200002,  // BZ R1 2 
        0x04210006,  // ADDI R1 R1 6
        0x0421000a,  // ADDI R1 R1 10  
        0x44000000   // HALT 
    };

    setupMockMemory(mem, program);

    // Test without forwarding
    while(!sim_no_forward->isProgramFinished()){
        sim_no_forward->cycle();

        if(stats.getClockCycles() >= 1000) {
            ADD_FAILURE() << "Simulator did not halt within 1000 cycles";
            break;
        }
    }
    
    EXPECT_EQ(rf.read(1), 20); 
    EXPECT_EQ(stats.getClockCycles(), 13); 
    EXPECT_EQ(stats.getStalls(), 4); 
    
    // Reset and test with forwarding
    resetSimulator(sim_with_forward, rf, stats, mem, true);
    setupMockMemory(mem, program);  // Re-setup mock for new simulator instance
    
    while(!sim_with_forward->isProgramFinished()){
        sim_with_forward->cycle();

        if(stats.getClockCycles() >= 1000) {
            ADD_FAILURE() << "Simulator did not halt within 1000 cycles";
            break;
        }
    }
    
    EXPECT_EQ(rf.read(1), 20);  // Same final result
    EXPECT_EQ(stats.getClockCycles(), 9);  // Expected cycle count with forwarding
    EXPECT_EQ(stats.getStalls(), 0);  // No stalls with forwarding
    
}

/**
 * @brief Test branch taken with both forwarding and no forwarding. 
 * Results: 
 * - No forwarding
 * R1 = 10 , PC = 16
 * Cycles = 12
 * Stalls = 2
 * - Forwarding
 * R1 = 10 , PC = 16
 * Cycles = 10
 * Stalls = 0    
 */
TEST_F(IntegrationTest, BZTaken) {
    std::vector<uint32_t> program = {
        0x00000800,  // ADD R1 R0 R0
        0x38200002,  // BZ R1 2 
        0x04210006,  // ADDI R1 R1 6 <- Should get skipped
        0x0421000A,  // ADDI R1 R1 10 
        0x44000000   // HALT 
    };

    setupMockMemory(mem, program);

    // Test without forwarding
    while(!sim_no_forward->isProgramFinished()){
        sim_no_forward->cycle();

        if(stats.getClockCycles() >= 1000) {
            ADD_FAILURE() << "Simulator did not halt within 1000 cycles";
            break;
        }
    }
    
    EXPECT_EQ(rf.read(1), 10); 
    EXPECT_EQ(stats.getClockCycles(), 12); 
    EXPECT_EQ(stats.getStalls(), 2); 
    // Add check for stats instruction categories
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::CONTROL_FLOW), 2);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::ARITHMETIC), 2);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::MEMORY_ACCESS), 0);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::LOGICAL), 0);

    
    // Reset and test with forwarding
    resetSimulator(sim_with_forward, rf, stats, mem, true);
    setupMockMemory(mem, program);  // Re-setup mock for new simulator instance
    
    while(!sim_with_forward->isProgramFinished()){
        sim_with_forward->cycle();

        if(stats.getClockCycles() >= 1000) {
            ADD_FAILURE() << "Simulator did not halt within 1000 cycles";
            break;
        }
    }
    
    EXPECT_EQ(rf.read(1), 10);  // Same final result
    EXPECT_EQ(stats.getClockCycles(), 10);  // Expected cycle count with forwarding
    EXPECT_EQ(stats.getStalls(), 0);  // No stalls with forwarding
    // Add check for stats instruction categories
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::CONTROL_FLOW), 2);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::ARITHMETIC), 2);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::MEMORY_ACCESS), 0);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::LOGICAL), 0);

}