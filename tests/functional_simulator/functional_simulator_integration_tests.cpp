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
 * @brief Run simulator until HALT instruction reaches writeback or max cycles
 * Uses the new cycle() method which handles all pipeline stages automatically
 */
int runUntilHalt(FunctionalSimulator* sim, int max_cycles = 1000) {
    int cycle_count = 0;
    
    while (cycle_count < max_cycles) {
        // Check if HALT has reached writeback stage
        const PipelineStageData* wb_stage = sim->getPipelineStage(FunctionalSimulator::WRITEBACK);
        if (wb_stage && wb_stage->instruction && 
            wb_stage->instruction->getOpcode() == mips_lite::opcode::HALT) {
            break;
        }
        
        // Check if pipeline is completely empty (all instructions completed)
        bool pipeline_empty = true;
        for (int i = 0; i < sim->getNumStages(); ++i) {
            if (!sim->isStageEmpty(i)) {
                pipeline_empty = false;
                break;
            }
        }
        if (pipeline_empty && sim->isHalted()) {
            break;
        }
        
        // Use the new cycle() method
        sim->cycle();
        cycle_count++;
    }
    
    return cycle_count;
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
    int cycles_no_forward = runUntilHalt(sim_no_forward.get());
    
    EXPECT_EQ(rf.read(1), 20);  // Same final result
    EXPECT_EQ(cycles_no_forward, 12); 
    EXPECT_EQ(stats.getStalls(), 4); // Stalls without forwarding
    
    // Reset and test with forwarding
    resetSimulator(sim_with_forward, rf, stats, mem, true);
    setupMockMemory(mem, program);  // Re-setup mock for new simulator instance
    
    int cycles_with_forward = runUntilHalt(sim_with_forward.get());
    
    
    EXPECT_EQ(rf.read(1), 20);  // Same final result
    EXPECT_EQ(cycles_with_forward, 8);  // Expected cycle count with forwarding
    EXPECT_EQ(stats.getStalls(), 0);  // No stalls with forwarding
    
}

