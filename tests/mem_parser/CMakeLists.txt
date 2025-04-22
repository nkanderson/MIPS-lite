# Create test executable for memory parser
add_executable(memory_parser_test memory_parser_tests.cpp)

# Set C++ standard for the test (redundant but kept for clarity)
target_compile_features(memory_parser_test PRIVATE cxx_std_17)

# Link against Google Test and the main project library
target_link_libraries(memory_parser_test
    PRIVATE
    gtest
    gtest_main
    mips_lite_lib
)

# Register with CTest using Google Test's discovery
include(GoogleTest)
gtest_discover_tests(memory_parser_test)
