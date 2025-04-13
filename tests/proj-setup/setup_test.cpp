#include <gtest/gtest.h>
#include <vector>
#include <algorithm>
#include <string>

// Test C++17 structured bindings
TEST(Cpp17Features, StructuredBindings) {
    std::pair<int, std::string> pair = {42, "MIPS"};
    auto [number, text] = pair;

    EXPECT_EQ(number, 42);
    EXPECT_EQ(text, "MIPS");
}

// Test C++17 standard version
TEST(Cpp17Features, StandardVersion) {
    EXPECT_GE(__cplusplus, 201703L)
        << "C++ standard should be at least C++17 (201703L), but got: "
        << __cplusplus;
}

// Test constexpr if (will compile only with C++17)
TEST(Cpp17Features, ConstexprIf) {
    bool executed = false;

    if constexpr (sizeof(void*) == 8) {
        // 64-bit system
        executed = true;
    } else {
        // 32-bit system
        executed = true;
    }

    EXPECT_TRUE(executed) << "constexpr if statement was not executed";
}

// Test standard library functionality
TEST(StandardLibrary, VectorOperations) {
    std::vector<int> numbers = {4, 8, 15, 16, 23, 42};
    int sum = 0;

    std::for_each(numbers.begin(), numbers.end(),
                 [&sum](int n) { sum += n; });

    EXPECT_EQ(sum, 108);
    EXPECT_EQ(numbers.size(), 6);
}

// Test that will output information about the environment
TEST(Environment, Information) {
    std::cout << "============================================" << std::endl;
    std::cout << "MIPS-Lite Pipeline Simulator Environment" << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << "C++ Standard: " << __cplusplus << std::endl;
    std::cout << "Pointer size: " << sizeof(void*) << " bytes" << std::endl;
    std::cout << "============================================" << std::endl;

    // This will always pass - it's just for information
    EXPECT_TRUE(true);
}
