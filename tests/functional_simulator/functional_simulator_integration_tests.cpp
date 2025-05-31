#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>

#include "functional_simulator.h"
#include "gtest/gtest.h"
#include "memory_interface.h"
#include "mips_instruction.h"
#include "mips_lite_defs.h"
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
    auto data_lookup = [&data_memory, &instruction_lookup](uint32_t addr) -> uint32_t {
        // If data_memory is provided and not empty, use separate data memory logic
        if (!data_memory.empty()) {
            auto it = data_memory.find(addr);
            if (it != data_memory.end()) {
                return it->second;
            } else {
                ADD_FAILURE()
                    << "Data memory access to uninitialized address: 0x" << std::hex << addr
                    << " (original: 0x" << std::hex << addr
                    << "). Please add this address to your data_memory map in the test setup.";
                throw std::runtime_error("Access to uninitialized data memory");
            }
        } else {  //[DEFAULT]
            // If data memory is not provided, use instruction memory. This is a fallback, and
            // ensures tests can still read instructions as data if no specific data memory is set
            // up. Hence, load/stores can still read or write to instruction memory as in project
            // specs.
            return instruction_lookup(addr);
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
 * @brief ADD Instruction Test: Performs various ADD sequences to produce a variety of results
 *
 * Results:
 * R1 = 5
 * R2 = 6
 * R3 = -5
 * R4 = -6
 * R5 = 11
 * R6 = -11
 * R7 = -1
 * R8 = 0
 *
 * - No forwarding
 * PC = 32, Cycles = 14, Stalls = 1
 * - Forwarding
 * PC = 32, Cycles = 13, Stalls = 0
 */
TEST_F(IntegrationTest, ADDSeq) {
    if (!sim_no_forward || !sim_with_forward) {
        ADD_FAILURE() << "Simulator instances not initialized properly";
        FAIL();
    }
    std::vector<uint32_t> program = {
        0x04010005,  // ADDI R1 R0 5
        0x04020006,  // ADDI R2 R0 6
        0x0403fffb,  // ADDI R3 R0 -5
        0x0404fffa,  // ADDI R4 R0 -6
        0x00222800,  // ADD R5 R1 R2
        0x00643000,  // ADD R6 R3 R4
        0x00243800,  // ADD R7 R1 R4
        0x00234000,  // ADD R8 R1 R3
        0x44000000   // HALT
    };

    setupMockMemory(mem, program);

    while (!sim_no_forward->isProgramFinished()) {
        sim_no_forward->cycle();

        if (stats.getClockCycles() >= 1000) {
            ADD_FAILURE() << "Simulator did not halt within 1000 cycles";
            break;
        }
    }

    EXPECT_EQ(rf.read(1), 5);
    EXPECT_EQ(rf.read(2), 6);
    EXPECT_EQ(rf.read(3), -5);
    EXPECT_EQ(rf.read(4), -6);
    EXPECT_EQ(rf.read(5), 11);
    EXPECT_EQ(rf.read(6), -11);
    EXPECT_EQ(rf.read(7), -1);
    EXPECT_EQ(rf.read(8), 0);
    EXPECT_EQ(stats.getClockCycles(), 14);
    EXPECT_EQ(stats.getStalls(), 1);
    EXPECT_EQ(sim_no_forward->getPC(), 32);  // PC should be at HALT instruction
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::CONTROL_FLOW), 1);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::ARITHMETIC), 8);
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

    EXPECT_EQ(rf.read(1), 5);
    EXPECT_EQ(rf.read(2), 6);
    EXPECT_EQ(rf.read(3), -5);
    EXPECT_EQ(rf.read(4), -6);
    EXPECT_EQ(rf.read(5), 11);
    EXPECT_EQ(rf.read(6), -11);
    EXPECT_EQ(rf.read(7), -1);
    EXPECT_EQ(rf.read(8), 0);
    EXPECT_EQ(stats.getClockCycles(), 13);
    EXPECT_EQ(stats.getStalls(), 0);
    EXPECT_EQ(sim_no_forward->getPC(), 32);  // PC should be at HALT instruction
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::CONTROL_FLOW), 1);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::ARITHMETIC), 8);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::MEMORY_ACCESS), 0);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::LOGICAL), 0);
}

/**
 * @brief ADDI Instruction Test: Performs various ADDI sequences to produce a variety of results
 *
 * Results:
 * R1 = 5
 * R2 = -5
 * R3 = 11
 * R4 = -11
 * R5 = -2
 * R6 = 0
 *
 * - No forwarding
 * PC = 24, Cycles = 12, Stalls = 1
 * - Forwarding
 * PC = 24, Cycles = 11, Stalls = 0
 */
TEST_F(IntegrationTest, ADDISeq) {
    if (!sim_no_forward || !sim_with_forward) {
        ADD_FAILURE() << "Simulator instances not initialized properly";
        FAIL();
    }
    std::vector<uint32_t> program = {
        0x04010005,  // ADDI R1 R0 5
        0x0402fffb,  // ADDI R2 R0 -5
        0x04230006,  // ADDI R3 R1 6
        0x0444fffa,  // ADDI R4 R2 -6
        0x04450003,  // ADDI R5 R2 3
        0x0426fffb,  // ADDI R6 R1 -5
        0x44000000   // HALT
    };

    setupMockMemory(mem, program);

    while (!sim_no_forward->isProgramFinished()) {
        sim_no_forward->cycle();

        if (stats.getClockCycles() >= 1000) {
            ADD_FAILURE() << "Simulator did not halt within 1000 cycles";
            break;
        }
    }

    EXPECT_EQ(rf.read(1), 5);
    EXPECT_EQ(rf.read(2), -5);
    EXPECT_EQ(rf.read(3), 11);
    EXPECT_EQ(rf.read(4), -11);
    EXPECT_EQ(rf.read(5), -2);
    EXPECT_EQ(rf.read(6), 0);
    EXPECT_EQ(stats.getClockCycles(), 12);
    EXPECT_EQ(stats.getStalls(), 1);
    EXPECT_EQ(sim_no_forward->getPC(), 24);  // PC should be at HALT instruction
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::CONTROL_FLOW), 1);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::ARITHMETIC), 6);
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

    EXPECT_EQ(rf.read(1), 5);
    EXPECT_EQ(rf.read(2), -5);
    EXPECT_EQ(rf.read(3), 11);
    EXPECT_EQ(rf.read(4), -11);
    EXPECT_EQ(rf.read(5), -2);
    EXPECT_EQ(rf.read(6), 0);
    EXPECT_EQ(stats.getClockCycles(), 11);
    EXPECT_EQ(stats.getStalls(), 0);
    EXPECT_EQ(sim_no_forward->getPC(), 24);  // PC should be at HALT instruction
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::CONTROL_FLOW), 1);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::ARITHMETIC), 6);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::MEMORY_ACCESS), 0);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::LOGICAL), 0);
}

/**
 * @brief SUB Instruction Test: Performs various SUB sequences to produce a variety of results
 *
 * Results:
 * R1 = 5
 * R2 = 6
 * R3 = -5
 * R4 = -6
 * R5 = -1
 * R6 = 1
 * R7 = 11
 * R8 = 0
 *
 * - No forwarding
 * PC = 32, Cycles = 14, Stalls = 1
 * - Forwarding
 * PC = 32, Cycles = 13, Stalls = 0
 */
TEST_F(IntegrationTest, SUBSeq) {
    if (!sim_no_forward || !sim_with_forward) {
        ADD_FAILURE() << "Simulator instances not initialized properly";
        FAIL();
    }
    std::vector<uint32_t> program = {
        0x04010005,  // ADDI R1 R0 5
        0x04020006,  // ADDI R2 R0 6
        0x0403fffb,  // ADDI R3 R0 -5
        0x0404fffa,  // ADDI R4 R0 -6
        0x08222800,  // SUB R5 R1 R2
        0x08643000,  // SUB R6 R3 R4
        0x08243800,  // SUB R7 R1 R4
        0x08214000,  // SUB R8 R1 R1
        0x44000000   // HALT
    };

    setupMockMemory(mem, program);

    while (!sim_no_forward->isProgramFinished()) {
        sim_no_forward->cycle();

        if (stats.getClockCycles() >= 1000) {
            ADD_FAILURE() << "Simulator did not halt within 1000 cycles";
            break;
        }
    }

    EXPECT_EQ(rf.read(1), 5);
    EXPECT_EQ(rf.read(2), 6);
    EXPECT_EQ(rf.read(3), -5);
    EXPECT_EQ(rf.read(4), -6);
    EXPECT_EQ(rf.read(5), -1);
    EXPECT_EQ(rf.read(6), 1);
    EXPECT_EQ(rf.read(7), 11);
    EXPECT_EQ(rf.read(8), 0);
    EXPECT_EQ(stats.getClockCycles(), 14);
    EXPECT_EQ(stats.getStalls(), 1);
    EXPECT_EQ(sim_no_forward->getPC(), 32);  // PC should be at HALT instruction
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::CONTROL_FLOW), 1);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::ARITHMETIC), 8);
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

    EXPECT_EQ(rf.read(1), 5);
    EXPECT_EQ(rf.read(2), 6);
    EXPECT_EQ(rf.read(3), -5);
    EXPECT_EQ(rf.read(4), -6);
    EXPECT_EQ(rf.read(5), -1);
    EXPECT_EQ(rf.read(6), 1);
    EXPECT_EQ(rf.read(7), 11);
    EXPECT_EQ(rf.read(8), 0);
    EXPECT_EQ(stats.getClockCycles(), 13);
    EXPECT_EQ(stats.getStalls(), 0);
    EXPECT_EQ(sim_no_forward->getPC(), 32);  // PC should be at HALT instruction
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::CONTROL_FLOW), 1);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::ARITHMETIC), 8);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::MEMORY_ACCESS), 0);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::LOGICAL), 0);
}

/**
 * @brief SUBI Instruction Test: Performs various SUBI sequences to produce a variety of results
 *
 * Results:
 * R1 = 5
 * R2 = -5
 * R3 = -1
 * R4 = 1
 * R5 = -8
 * R6 = 0
 *
 * - No forwarding
 * PC = 24, Cycles = 12, Stalls = 1
 * - Forwarding
 * PC = 24, Cycles = 11, Stalls = 0
 */
TEST_F(IntegrationTest, SUBISeq) {
    if (!sim_no_forward || !sim_with_forward) {
        ADD_FAILURE() << "Simulator instances not initialized properly";
        FAIL();
    }
    std::vector<uint32_t> program = {
        0x04010005,  // ADDI R1 R0 5
        0x0402fffb,  // ADDI R2 R0 -5
        0x0c230006,  // SUBI R3 R1 6
        0x0c44fffa,  // SUBI R4 R2 -6
        0x0c450003,  // SUBI R5 R2 3
        0x0c260005,  // SUBI R6 R1 5
        0x44000000   // HALT
    };

    setupMockMemory(mem, program);

    while (!sim_no_forward->isProgramFinished()) {
        sim_no_forward->cycle();

        if (stats.getClockCycles() >= 1000) {
            ADD_FAILURE() << "Simulator did not halt within 1000 cycles";
            break;
        }
    }

    EXPECT_EQ(rf.read(1), 5);
    EXPECT_EQ(rf.read(2), -5);
    EXPECT_EQ(rf.read(3), -1);
    EXPECT_EQ(rf.read(4), 1);
    EXPECT_EQ(rf.read(5), -8);
    EXPECT_EQ(rf.read(6), 0);
    EXPECT_EQ(stats.getClockCycles(), 12);
    EXPECT_EQ(stats.getStalls(), 1);
    EXPECT_EQ(sim_no_forward->getPC(), 24);  // PC should be at HALT instruction
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::CONTROL_FLOW), 1);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::ARITHMETIC), 6);
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

    EXPECT_EQ(rf.read(1), 5);
    EXPECT_EQ(rf.read(2), -5);
    EXPECT_EQ(rf.read(3), -1);
    EXPECT_EQ(rf.read(4), 1);
    EXPECT_EQ(rf.read(5), -8);
    EXPECT_EQ(rf.read(6), 0);
    EXPECT_EQ(stats.getClockCycles(), 11);
    EXPECT_EQ(stats.getStalls(), 0);
    EXPECT_EQ(sim_no_forward->getPC(), 24);  // PC should be at HALT instruction
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::CONTROL_FLOW), 1);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::ARITHMETIC), 6);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::MEMORY_ACCESS), 0);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::LOGICAL), 0);
}

/**
 * @brief MUL Instruction Test: Performs various MUL sequences to produce a variety of results.
 * Both no-forwarding and forwarding tests should have the same timing.
 *
 * Results:
 * R1 = 5
 * R2 = 6
 * R3 = -5
 * R4 = -6
 * R5 = 1
 * R6 = 30
 * R7 = 30
 * R8 = -30
 * R9 = 0
 * R10 = 5
 *
 * - No forwarding and Forwarding
 * PC = 40, Cycles = 15, Stalls = 0
 */
TEST_F(IntegrationTest, MULSeq) {
    if (!sim_no_forward || !sim_with_forward) {
        ADD_FAILURE() << "Simulator instances not initialized properly";
        FAIL();
    }
    std::vector<uint32_t> program = {
        0x04010005,  // ADDI R1 R0 5
        0x04020006,  // ADDI R2 R0 6
        0x0403fffb,  // ADDI R3 R0 -5
        0x0404fffa,  // ADDI R4 R0 -6
        0x04050001,  // ADDI R5 R0 1
        0x10223000,  // MUL R6 R1 R2
        0x10643800,  // MUL R7 R3 R4
        0x10244000,  // MUL R8 R1 R4
        0x10204800,  // MUL R9 R1 R0
        0x10255000,  // MUL R10 R1 R5
        0x44000000   // HALT
    };

    setupMockMemory(mem, program);

    while (!sim_no_forward->isProgramFinished()) {
        sim_no_forward->cycle();

        if (stats.getClockCycles() >= 1000) {
            ADD_FAILURE() << "Simulator did not halt within 1000 cycles";
            break;
        }
    }

    EXPECT_EQ(rf.read(1), 5);
    EXPECT_EQ(rf.read(2), 6);
    EXPECT_EQ(rf.read(3), -5);
    EXPECT_EQ(rf.read(4), -6);
    EXPECT_EQ(rf.read(5), 1);
    EXPECT_EQ(rf.read(6), 30);
    EXPECT_EQ(rf.read(7), 30);
    EXPECT_EQ(rf.read(8), -30);
    EXPECT_EQ(rf.read(9), 0);
    EXPECT_EQ(rf.read(10), 5);
    EXPECT_EQ(stats.getClockCycles(), 15);
    EXPECT_EQ(stats.getStalls(), 0);
    EXPECT_EQ(sim_no_forward->getPC(), 40);  // PC should be at HALT instruction
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::CONTROL_FLOW), 1);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::ARITHMETIC), 10);
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

    EXPECT_EQ(rf.read(1), 5);
    EXPECT_EQ(rf.read(2), 6);
    EXPECT_EQ(rf.read(3), -5);
    EXPECT_EQ(rf.read(4), -6);
    EXPECT_EQ(rf.read(5), 1);
    EXPECT_EQ(rf.read(6), 30);
    EXPECT_EQ(rf.read(7), 30);
    EXPECT_EQ(rf.read(8), -30);
    EXPECT_EQ(rf.read(9), 0);
    EXPECT_EQ(rf.read(10), 5);
    EXPECT_EQ(stats.getClockCycles(), 15);
    EXPECT_EQ(stats.getStalls(), 0);
    EXPECT_EQ(sim_no_forward->getPC(), 40);  // PC should be at HALT instruction
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::CONTROL_FLOW), 1);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::ARITHMETIC), 10);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::MEMORY_ACCESS), 0);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::LOGICAL), 0);
}

/**
 * @brief MULI Instruction Test: Performs various MULI sequences to produce a variety of results
 * Both no-forwarding and forwarding tests should have the same timing.
 *
 * Results:
 * R1 = 5
 * R2 = -5
 * R3 = 30
 * R1 = 30
 * R2 = -15
 * R3 = 0
 * R7 = 5
 *
 * - No forwarding
 * PC = 28, Cycles = 13, Stalls = 1
 * - Forwarding
 * PC = 28, Cycles = 12, Stalls = 0
 */
TEST_F(IntegrationTest, MULISeq) {
    if (!sim_no_forward || !sim_with_forward) {
        ADD_FAILURE() << "Simulator instances not initialized properly";
        FAIL();
    }
    std::vector<uint32_t> program = {
        0x04010005,  // ADDI R1 R0 5
        0x0402fffb,  // ADDI R2 R0 -5
        0x14230006,  // MULI R3 R1 6
        0x1444fffa,  // MULI R4 R2 -6
        0x14450003,  // MULI R5 R2 3
        0x14260000,  // MULI R6 R1 0
        0x14270001,  // MULI R7 R1 1
        0x44000000   // HALT
    };

    setupMockMemory(mem, program);

    while (!sim_no_forward->isProgramFinished()) {
        sim_no_forward->cycle();

        if (stats.getClockCycles() >= 1000) {
            ADD_FAILURE() << "Simulator did not halt within 1000 cycles";
            break;
        }
    }

    EXPECT_EQ(rf.read(1), 5);
    EXPECT_EQ(rf.read(2), -5);
    EXPECT_EQ(rf.read(3), 30);
    EXPECT_EQ(rf.read(4), 30);
    EXPECT_EQ(rf.read(5), -15);
    EXPECT_EQ(rf.read(6), 0);
    EXPECT_EQ(rf.read(7), 5);
    EXPECT_EQ(stats.getClockCycles(), 13);
    EXPECT_EQ(stats.getStalls(), 1);
    EXPECT_EQ(sim_no_forward->getPC(), 28);  // PC should be at HALT instruction
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::CONTROL_FLOW), 1);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::ARITHMETIC), 7);
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

    EXPECT_EQ(rf.read(1), 5);
    EXPECT_EQ(rf.read(2), -5);
    EXPECT_EQ(rf.read(3), 30);
    EXPECT_EQ(rf.read(4), 30);
    EXPECT_EQ(rf.read(5), -15);
    EXPECT_EQ(rf.read(6), 0);
    EXPECT_EQ(rf.read(7), 5);
    EXPECT_EQ(stats.getClockCycles(), 12);
    EXPECT_EQ(stats.getStalls(), 0);
    EXPECT_EQ(sim_no_forward->getPC(), 28);  // PC should be at HALT instruction
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::CONTROL_FLOW), 1);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::ARITHMETIC), 7);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::MEMORY_ACCESS), 0);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::LOGICAL), 0);
}

/**
 * @brief Test BEQ not taken with both forwarding and no forwarding.
 * Results:
 * - No forwarding
 * Cycles = 14
 * Stalls = 4
 * - Forwarding
 * Cycles = 10
 * Stalls = 0
 *
 * R1 = 20
 * R2 = 8
 * PC = 20
 */
TEST_F(IntegrationTest, BEQNotTaken) {
    if (!sim_no_forward || !sim_with_forward) {
        ADD_FAILURE() << "Simulator instances not initialized properly";
        FAIL();
    }
    std::vector<uint32_t> program = {
        0x04010004,  // ADDI R1 R0 4
        0x04020008,  // ADDI R2 R0 8
        0x3c220002,  // BEQ R1 R2 2
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
    EXPECT_EQ(rf.read(2), 8);
    EXPECT_EQ(stats.getClockCycles(), 14);
    EXPECT_EQ(stats.getStalls(), 4);
    EXPECT_EQ(sim_no_forward->getPC(), 20);  // PC should be at HALT instruction
    // Add check for stats instruction categories
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::CONTROL_FLOW), 2);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::ARITHMETIC), 4);
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

    EXPECT_EQ(rf.read(1), 20);  // Same final result
    EXPECT_EQ(rf.read(2), 8);
    EXPECT_EQ(stats.getClockCycles(), 10);     // Expected cycle count with forwarding
    EXPECT_EQ(stats.getStalls(), 0);           // No stalls with forwarding
    EXPECT_EQ(sim_with_forward->getPC(), 20);  // PC should be at HALT instruction
    // Add check for stats instruction categories
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::CONTROL_FLOW), 2);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::ARITHMETIC), 4);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::MEMORY_ACCESS), 0);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::LOGICAL), 0);
}

/**
 * @brief Test BEQ taken with both forwarding and no forwarding.
 * Results:
 * - No forwarding
 * Cycles = 13
 * Stalls = 2
 * - Forwarding
 * Cycles = 11
 * Stalls = 0
 *
 * R1 = 14
 * R2 = 4
 * PC = 20
 */
TEST_F(IntegrationTest, BEQTaken) {
    if (!sim_no_forward || !sim_with_forward) {
        ADD_FAILURE() << "Simulator instances not initialized properly";
        FAIL();
    }
    std::vector<uint32_t> program = {
        0x04010004,  // ADDI R1 R0 4
        0x04020004,  // ADDI R2 R0 4
        0x3c220002,  // BEQ R1 R2 2
        0x04210006,  // ADDI R1 R1 6 <- Should get skipped
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

    EXPECT_EQ(rf.read(1), 14);
    EXPECT_EQ(rf.read(2), 4);
    EXPECT_EQ(stats.getClockCycles(), 13);
    EXPECT_EQ(stats.getStalls(), 2);
    EXPECT_EQ(sim_no_forward->getPC(), 20);  // PC should be at HALT instruction
    // Add check for stats instruction categories
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::CONTROL_FLOW), 2);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::ARITHMETIC), 3);
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

    EXPECT_EQ(rf.read(1), 14);  // Same final result
    EXPECT_EQ(rf.read(2), 4);
    EXPECT_EQ(stats.getClockCycles(), 11);     // Expected cycle count with forwarding
    EXPECT_EQ(stats.getStalls(), 0);           // No stalls with forwarding
    EXPECT_EQ(sim_with_forward->getPC(), 20);  // PC should be at HALT instruction
    // Add check for stats instruction categories
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::CONTROL_FLOW), 2);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::ARITHMETIC), 3);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::MEMORY_ACCESS), 0);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::LOGICAL), 0);
}

/**
 * @brief Test BZ not taken with both forwarding and no forwarding.
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
    // Add check for stats instruction categories
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::CONTROL_FLOW), 2);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::ARITHMETIC), 3);
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

    EXPECT_EQ(rf.read(1), 20);                 // Same final result
    EXPECT_EQ(stats.getClockCycles(), 9);      // Expected cycle count with forwarding
    EXPECT_EQ(stats.getStalls(), 0);           // No stalls with forwarding
    EXPECT_EQ(sim_with_forward->getPC(), 16);  // PC should be at HALT instruction
    // Add check for stats instruction categories
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::CONTROL_FLOW), 2);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::ARITHMETIC), 3);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::MEMORY_ACCESS), 0);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::LOGICAL), 0);
}

/**
 * @brief Test BZ taken with both forwarding and no forwarding.
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
 * @brief Test JR unconditional branch with both forwarding and no forwarding.
 * Results:
 * - No forwarding
 * Cycles = 13
 * Stalls = 2
 * - Forwarding
 * Cycles = 11
 * Stalls = 0
 *
 * R1 = 16
 * R2 = 10
 * PC = 20
 */
TEST_F(IntegrationTest, JRUnconditionalBranch) {
    if (!sim_no_forward || !sim_with_forward) {
        ADD_FAILURE() << "Simulator instances not initialized properly";
        FAIL();
    }
    std::vector<uint32_t> program = {
        0x00001000,  // ADD R2 R0 R0
        0x04010010,  // ADDI R1 R0 16
        0x40200000,  // JR R1
        0x0402000a,  // ADDI R2 R0 10
        0x0442000a,  // ADDI R2 R2 10
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

    EXPECT_EQ(rf.read(1), 16);
    EXPECT_EQ(rf.read(2), 10);
    EXPECT_EQ(stats.getClockCycles(), 13);
    EXPECT_EQ(stats.getStalls(), 2);
    EXPECT_EQ(sim_no_forward->getPC(), 20);  // PC should be at HALT instruction
    // Add check for stats instruction categories
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::CONTROL_FLOW), 2);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::ARITHMETIC), 3);
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

    EXPECT_EQ(rf.read(1), 16);  // Same final result
    EXPECT_EQ(rf.read(2), 10);
    EXPECT_EQ(stats.getClockCycles(), 11);     // Expected cycle count with forwarding
    EXPECT_EQ(stats.getStalls(), 0);           // No stalls with forwarding
    EXPECT_EQ(sim_with_forward->getPC(), 20);  // PC should be at HALT instruction
    // Add check for stats instruction categories
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::CONTROL_FLOW), 2);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::ARITHMETIC), 3);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::MEMORY_ACCESS), 0);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::LOGICAL), 0);
}

/**
 * @brief RAW Depedency caused by load instruction. Special case where forwarding doesn't completely
 * resolve the stall penalty. With forwarding, stall = 1, without forwarding stall = 2. The value
 * forwarded should come from mem_data in pipleinstage structure.
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

/**
 * @brief RAW dependency chaining where each instruction depends on the previous one.
 * This tests the forwarding logic in a more complex scenario.
 */

TEST_F(IntegrationTest, rawDependencyChaining) {
    if (!sim_no_forward || !sim_with_forward) {
        ADD_FAILURE() << "Simulator instances not initialized properly";
        FAIL();
    }

    std::vector<uint32_t> program{0x0401000a, 0x04220014, 0x00221800, 0x08612000,
                                  0x18832800, 0x20a43000, 0x18c13800, 0x44000000};

    setupMockMemory(mem, program);

    while (!sim_no_forward->isProgramFinished()) {
        sim_no_forward->cycle();

        if (stats.getClockCycles() >= 1000) {
            ADD_FAILURE() << "Simulator did not halt within 1000 cycles";
            break;
        }
    }

    // Check Regs
    EXPECT_EQ(rf.read(1), 10);
    EXPECT_EQ(rf.read(2), 30);
    EXPECT_EQ(rf.read(3), 40);
    EXPECT_EQ(rf.read(4), 30);
    EXPECT_EQ(rf.read(5), 62);
    EXPECT_EQ(rf.read(6), 30);
    EXPECT_EQ(rf.read(7), 30);
    // Check timing & Stats
    EXPECT_EQ(sim_no_forward->getPC(), 28);
    EXPECT_EQ(stats.getStalls(), 12);
    EXPECT_EQ(stats.getClockCycles(), 24);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::ARITHMETIC), 4);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::LOGICAL), 3);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::CONTROL_FLOW), 1);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::MEMORY_ACCESS), 0);

    // Test forwarding
    resetSimulator(sim_with_forward, rf, stats, mem, true);
    setupMockMemory(mem, program);  // Re-setup mock for new simulator instance

    while (!sim_with_forward->isProgramFinished()) {
        sim_with_forward->cycle();

        if (stats.getClockCycles() >= 1000) {
            ADD_FAILURE() << "Simulator did not halt within 1000 cycles";
            break;
        }
    }

    // Check Regs
    EXPECT_EQ(rf.read(1), 10);
    EXPECT_EQ(rf.read(2), 30);
    EXPECT_EQ(rf.read(3), 40);
    EXPECT_EQ(rf.read(4), 30);
    EXPECT_EQ(rf.read(5), 62);
    EXPECT_EQ(rf.read(6), 30);
    EXPECT_EQ(rf.read(7), 30);
    // Check timing & Stats
    EXPECT_EQ(sim_no_forward->getPC(), 28);
    EXPECT_EQ(stats.getClockCycles(), 12);
    EXPECT_EQ(stats.getStalls(), 0);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::ARITHMETIC), 4);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::LOGICAL), 3);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::CONTROL_FLOW), 1);
    EXPECT_EQ(stats.getCategoryCount(mips_lite::InstructionCategory::MEMORY_ACCESS), 0);
}

