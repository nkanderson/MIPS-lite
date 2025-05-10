# Create test executable for memory parser
set(TEST_NAME  reg_type_tests)
add_executable(${TEST_NAME} test_reg_type.cpp)

# Set C++ standard for the test (redundant but kept for clarity)
target_compile_features(${TEST_NAME} PRIVATE cxx_std_17)

# Link against Google Test and the main project library
target_link_libraries(${TEST_NAME}
    PRIVATE
    gtest
    gtest_main
    mips_lite_lib
)

# Register with CTest using Google Test's discovery
include(GoogleTest)
gtest_discover_tests(${TEST_NAME})
