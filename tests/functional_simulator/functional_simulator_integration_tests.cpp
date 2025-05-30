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
/**
 * @brief Set up mocking for an array of hex-encoded instructions with optional data memory.
 * Does not require calls to be made in order.
 *
 * @param mem Mock memory parser instance
 * @param instructions Vector of instruction words
 * @param data_memory Optional map of address -> value for data memory (for load/store tests)
 * @param instruction_base Base address for instructions (default 0x0)
 */
void setupMockMemory(MockMemoryParser& mem, const std::vector<uint32_t>& instructions,
                     const std::map<uint32_t, uint32_t>& data_memory = {},
                     uint32_t instruction_base = 0x0) {
    // Instruction fetch handler
    auto instruction_lookup = [&instructions, instruction_base](uint32_t addr) -> uint32_t {
        size_t index = (addr - instruction_base) / 4;
        if ((addr - instruction_base) % 4 != 0) {
            ADD_FAILURE() << "Unaligned instruction fetch at address: 0x" << std::hex << addr;
            throw std::runtime_error("Unaligned access");
        }
        if (index >= instructions.size()) {
            ADD_FAILURE() << "Instruction fetch out of bounds at address: 0x" << std::hex << addr;
            throw std::out_of_range("Access outside instruction memory");
        }
        return instructions[index];
    };

    // Data memory handler
    auto data_lookup = [&instructions, &data_memory, instruction_base](uint32_t addr) -> uint32_t {
        // If data_memory is provided and not empty, use separate data memory logic
        if (!data_memory.empty()) {
            // Align address to word boundary by converting word index to byte address
            uint32_t word_index = addr / 4;
            uint32_t aligned_addr = word_index << 2;  // word_index * 4

            auto it = data_memory.find(aligned_addr);
            if (it != data_memory.end()) {
                return it->second;
            } else {
                ADD_FAILURE()
                    << "Data memory access to uninitialized address: 0x" << std::hex << aligned_addr
                    << " (original: 0x" << std::hex << addr
                    << "). Please add this address to your data_memory map in the test setup.";
                throw std::runtime_error("Access to uninitialized data memory");
            }
        } else {
            // If data memory is not provided, use instruction memory. This is a fallback, and
            // ensures tests can still read instructions as data if no specific data memory is set
            // up. Hence, load/stores can still read or write to instruction memory.
            size_t index = (addr - instruction_base) / 4;
            if ((addr - instruction_base) % 4 != 0) {
                ADD_FAILURE() << "Unaligned memory access at address: 0x" << std::hex << addr;
                throw std::runtime_error("Unaligned access");
            }
            if (index >= instructions.size()) {
                ADD_FAILURE() << "Memory access out of bounds at address: 0x" << std::hex << addr;
                throw std::out_of_range("Access outside memory vector");
            }
            return instructions[index];
        }
    };

    EXPECT_CALL(mem, readInstruction(::testing::_))
        .WillRepeatedly(::testing::Invoke(instruction_lookup));

    EXPECT_CALL(mem, readMemory(::testing::_)).WillRepeatedly(::testing::Invoke(data_lookup));
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
// Test suite
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
    if (!sim_no_forward || !sim_with_forward) {
        ADD_FAILURE() << "Simulator instances not initialized properly";
        FAIL();
    }
    std::vector<uint32_t> program = {
        0x04010004,  // ADDI R1 R0 4
        0x38200002,  // BZ R1 2
        0x04210006,  // ADDI R1 R1 6
        0x0421000a,  // ADDI R1 R1 10
        0x44000000   // HALT
    };

    setupMockMemory(mem, program);

    // Test without forwarding
    while (!sim_no_forward->isProgramFinished()) {
        sim_no_forward->cycle();

        if (stats.getClockCycles() >= 1000) {
            ADD_FAILURE() << "Simulator did not halt within 1000 cycles";
            break;
        }
    }

    EXPECT_EQ(rf.read(1), 20);
    EXPECT_EQ(stats.getClockCycles(), 13);
    EXPECT_EQ(stats.getStalls(), 4);
    EXPECT_EQ(sim_no_forward->getPC(), 16);  // PC should be at HALT instruction

    // Reset and test with forwarding
    resetSimulator(sim_with_forward, rf, stats, mem, true);
    setupMockMemory(mem, program);  // Re-setup mock for new simulator instance

    while (!sim_with_forward->isProgramFinished()) {
        sim_with_forward->cycle();

        if (stats.getClockCycles() >= 1000) {
            ADD_FAILURE() << "Simulator did not halt within 1000 cycles";
            break;
        }
    }

    EXPECT_EQ(rf.read(1), 20);                 // Same final result
    EXPECT_EQ(stats.getClockCycles(), 9);      // Expected cycle count with forwarding
    EXPECT_EQ(stats.getStalls(), 0);           // No stalls with forwarding
    EXPECT_EQ(sim_with_forward->getPC(), 16);  // PC should be at HALT instruction
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
    if (!sim_no_forward || !sim_with_forward) {
        ADD_FAILURE() << "Simulator instances not initialized properly";
        FAIL();
    }
    std::vector<uint32_t> program = {
        0x00000800,  // ADD R1 R0 R0
        0x38200002,  // BZ R1 2
        0x04210006,  // ADDI R1 R1 6 <- Should get skipped
        0x0421000A,  // ADDI R1 R1 10
        0x44000000   // HALT
    };

    setupMockMemory(mem, program);

    // Test without forwarding
    while (!sim_no_forward->isProgramFinished()) {
        sim_no_forward->cycle();

        if (stats.getClockCycles() >= 1000) {
            ADD_FAILURE() << "Simulator did not halt within 1000 cycles";
            break;
        }
    }

    EXPECT_EQ(rf.read(1), 10);
    EXPECT_EQ(stats.getClockCycles(), 12);
    EXPECT_EQ(stats.getStalls(), 2);
    EXPECT_EQ(sim_no_forward->getPC(), 16);  // PC should be at HALT instruction
    // Add check for stats instruction categories
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::CONTROL_FLOW), 2);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::ARITHMETIC), 2);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::MEMORY_ACCESS), 0);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::LOGICAL), 0);

    // Reset and test with forwarding
    resetSimulator(sim_with_forward, rf, stats, mem, true);
    setupMockMemory(mem, program);  // Re-setup mock for new simulator instance

    while (!sim_with_forward->isProgramFinished()) {
        sim_with_forward->cycle();

        if (stats.getClockCycles() >= 1000) {
            ADD_FAILURE() << "Simulator did not halt within 1000 cycles";
            break;
        }
    }

    EXPECT_EQ(rf.read(1), 10);                 // Same final result
    EXPECT_EQ(stats.getClockCycles(), 10);     // Expected cycle count with forwarding
    EXPECT_EQ(stats.getStalls(), 0);           // No stalls with forwarding
    EXPECT_EQ(sim_with_forward->getPC(), 16);  // PC should be at HALT instruction
    // Add check for stats instruction categories
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::CONTROL_FLOW), 2);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::ARITHMETIC), 2);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::MEMORY_ACCESS), 0);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::LOGICAL), 0);
}

/**
 * @brief RAW Depedency caused by load instruction. Special case where forwarding doesn't completely
 * resolve the stall penalty. With forwarding, stall = 1, without forwarding stall = 2. The value
 * forwarded should come from mem_data in pipleinstage structure.
 *
 *
 */
TEST_F(IntegrationTest, rawCausedByLoad) {
    if (!sim_no_forward || !sim_with_forward) {
        ADD_FAILURE() << "Simulator instances not initialized properly";
        FAIL();
    }
    std::vector<uint32_t> program = {
        0x04630064,  // ADDI R3 R3 #100
        0x3062003c,  // LDW R2 R3 60 (Effective Address 100 + 60 = 160)
        0x0c49001e,  // SUBI R9 R2 30 (RAW dependency on R2)
        0x44000000   // HALT
    };

    // Prepare data memory for load instruction
    std::map<uint32_t, uint32_t> data_memory = {
        {160, 40}  // Address 160 contains value 40
    };
    setupMockMemory(mem, program, data_memory);

    while (!sim_no_forward->isProgramFinished()) {
        sim_no_forward->cycle();

        if (stats.getClockCycles() >= 1000) {
            ADD_FAILURE() << "Simulator did not halt within 1000 cycles";
            break;
        }
    }

    EXPECT_EQ(rf.read(3), 100);
    EXPECT_EQ(rf.read(2), 40);  // R2 should have loaded 40 from memory
    EXPECT_EQ(rf.read(9), 10);  // R9 = 40 - 30 = 10
    EXPECT_EQ(stats.getStalls(), 4);
    EXPECT_EQ(stats.getClockCycles(), 12);
    EXPECT_EQ(sim_no_forward->getPC(), 12);

    // Test with forwarding
    resetSimulator(sim_with_forward, rf, stats, mem, true);
    setupMockMemory(mem, program, data_memory);  // Re-setup mock for new simulator instance
    while (!sim_with_forward->isProgramFinished()) {
        sim_with_forward->cycle();

        if (stats.getClockCycles() >= 1000) {
            ADD_FAILURE() << "Simulator did not halt within 1000 cycles";
            break;
        }
    }

    EXPECT_EQ(rf.read(3), 100);
    EXPECT_EQ(rf.read(2), 40);        // R2 should have loaded 40 from memory
    EXPECT_EQ(rf.read(9), 10);        // R9 = 40 - 30 = 10
    EXPECT_EQ(stats.getStalls(), 1);  // Only 1 stall due to load
    EXPECT_EQ(stats.getClockCycles(), 9);
    EXPECT_EQ(sim_with_forward->getPC(), 12);  // PC should be at HALT instruction
}