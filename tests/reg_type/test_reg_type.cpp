#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "mips_instruction.h"
#include "reg_type.h"

// Simple test struct for basic functionality testing
struct TestData {
    int value;
    std::string name;

    TestData(int v = 0, const std::string& n = "") : value(v), name(n) {}

    bool operator==(const TestData& other) const {
        return value == other.value && name == other.name;
    }
};

// Test fixture for reg type tests
class RegTypeTest : public ::testing::Test {
   protected:
    void SetUp() override {
        // Setup code if needed
    }

    void TearDown() override {
        // Cleanup code if needed
    }
};

TEST_F(RegTypeTest, BasicOperations) {
    // Test with integer
    reg<int> int_reg(42);

    EXPECT_EQ(int_reg.current(), 42);
    EXPECT_FALSE(int_reg.next().has_value());
    EXPECT_TRUE(int_reg.isValid());
    EXPECT_TRUE(int_reg.isEnabled());

    // Test setNext and clock
    int_reg.setNext(100);
    EXPECT_EQ(int_reg.current(), 42);  // Still old value
    EXPECT_TRUE(int_reg.next().has_value());
    EXPECT_EQ(int_reg.next().value(), 100);

    int_reg.clock();
    EXPECT_EQ(int_reg.current(), 100);         // New value after clock
    EXPECT_FALSE(int_reg.next().has_value());  // Next is cleared
}

// Pipeline flow operatorv
TEST_F(RegTypeTest, PipelineFlowOperator) {
    reg<TestData> stage1(TestData{10, "data1"});
    reg<TestData> stage2(TestData{});  // Provide initial value
    reg<TestData> stage3(TestData{});  // Provide initial value

    // Test single transfer
    stage1 >> stage2;
    stage2.clock();

    EXPECT_EQ(stage2.current().value, 10);
    EXPECT_EQ(stage2.current().name, "data1");

    // Test chained transfer
    stage1.setNext(TestData{20, "data2"});
    stage1.clock();

    stage1 >> stage2 >> stage3;
    stage2.clock();
    stage3.clock();

    EXPECT_EQ(stage3.current().value, 10);  // Original data moved through
    EXPECT_EQ(stage3.current().name, "data1");
}

// Enable/disable functionality
TEST_F(RegTypeTest, EnableDisable) {
    reg<int> test_reg(50);

    // Disable register
    test_reg.setEnable(false);
    EXPECT_FALSE(test_reg.isEnabled());

    // Try to set next value while disabled
    test_reg.setNext(75);
    EXPECT_FALSE(test_reg.next().has_value());  // Should not set

    // Re-enable and try again
    test_reg.setEnable(true);
    test_reg.setNext(75);
    EXPECT_TRUE(test_reg.next().has_value());
    EXPECT_EQ(test_reg.next().value(), 75);
}

// Clear functionality
TEST_F(RegTypeTest, ClearRegister) {
    reg<std::string> str_reg("Hello");

    EXPECT_TRUE(str_reg.isValid());
    EXPECT_EQ(str_reg.current(), "Hello");

    str_reg.clear();
    EXPECT_FALSE(str_reg.isValid());

    // Accessing current() should throw
    EXPECT_THROW(str_reg.current(), std::runtime_error);

    // Next value should also be cleared
    EXPECT_FALSE(str_reg.next().has_value());

    // Test value_or for safe access
    EXPECT_EQ(str_reg.value_or("default"), "default");
}

// Update functionality
TEST_F(RegTypeTest, UpdateFunction) {
    reg<int> counter(5);

    // Test with lambda
    counter.update([](int val) { return val * 2; });
    counter.clock();
    EXPECT_EQ(counter.current(), 10);
}

// Move semantics to test that the reg class can handle move operations
// in this example move will cause large vector to transfer ownership
// leaving the original empty
TEST_F(RegTypeTest, MoveSemantics) {
    reg<std::vector<int>> vec_reg(std::vector<int>{});  // Provide initial empty vector

    // Test move construction
    std::vector<int> large_vec(1000, 42);
    vec_reg.setNext(std::move(large_vec));

    EXPECT_TRUE(large_vec.empty());  // Should be moved from here not copied
    vec_reg.clock();
    EXPECT_EQ(vec_reg.current().size(), 1000);
}

// Integration test with Instruction class
TEST_F(RegTypeTest, InstructionPipeline) {
    // Create a simple pipeline with Instruction type
    uint32_t dummy_instruction = 0x01284020;  // Dummy instruction

    reg<Instruction> fetch_reg{Instruction(dummy_instruction)};
    reg<Instruction> decode_reg;
    // Test pipeline flow
    fetch_reg >> decode_reg;
    decode_reg.clock();

    EXPECT_EQ(decode_reg.current().getInstruction(), dummy_instruction);
    EXPECT_EQ(decode_reg.current().getOpcode(), fetch_reg.current().getOpcode());

    // Test with HALT instruction
    uint32_t halt_instruction = 0x44000000;  //  HALT
    fetch_reg.setNext(Instruction(halt_instruction));
    fetch_reg.clock();

    EXPECT_TRUE(fetch_reg.current().isHaltInstruction());
}
