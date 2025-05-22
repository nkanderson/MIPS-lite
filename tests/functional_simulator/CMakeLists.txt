# Create test executable for FunctionalSimulator tests
set(TEST_NAME  functional_simulator_test)
add_executable(${TEST_NAME} functional_simulator_tests.cpp)
# add_executable(${TEST_NAME} functional_simulator_tests.cpp functional_simulator_integration_tests.cpp)
target_compile_definitions(functional_simulator_test PRIVATE UNIT_TEST)

# Set C++ standard for the test (redundant but kept for clarity)
target_compile_features(${TEST_NAME} PRIVATE cxx_std_17)

# Link against Google Test and the main project library
target_link_libraries(${TEST_NAME}
    PRIVATE
    gtest
    gmock
    gtest_main
    mips_lite_lib
)

# Register with CTest using Google Test's discovery
include(GoogleTest)
gtest_discover_tests(${TEST_NAME})
