# Create test executable for FunctionalSimulator tests
set(TEST_NAME  functional_simulator_test)
add_executable(${TEST_NAME} functional_simulator_tests.cpp)
# add_executable(${TEST_NAME} functional_simulator_tests.cpp functional_simulator_integration_tests.cpp)

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

### Create tests for decode stage
set(ID_TEST_NAME decode_stage_tests)
add_executable(${ID_TEST_NAME} decode_stage_tests.cpp)
target_link_libraries(${ID_TEST_NAME}
    PRIVATE
    gtest
    gmock
    gtest_main
    mips_lite_lib
)

### Create tests for execute stage
set(EXE_TEST_NAME exe_stage_tests)
add_executable(${EXE_TEST_NAME} exe_stage_tests.cpp)
target_link_libraries(${EXE_TEST_NAME}
    PRIVATE
    gtest
    gmock
    gtest_main
    mips_lite_lib
)

# Register with CTest using Google Test's discovery
include(GoogleTest)
gtest_discover_tests(${TEST_NAME})
gtest_discover_tests(${ID_TEST_NAME})
gtest_discover_tests(${EXE_TEST_NAME})
