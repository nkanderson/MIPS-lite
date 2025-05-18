# Create test executable for utilities test
add_executable(utilities_test utilities_test.cpp)
# Set the include directories for the test executable
target_include_directories(utilities_test
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)


# Set C++ standard for the test (redundant but kept for clarity)
target_compile_features(utilities_test PRIVATE cxx_std_17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Link against Google Test and the main project library
target_link_libraries(utilities_test
    PRIVATE
    gtest
    gtest_main
    mips_lite_lib
)


# Register with CTest using Google Test's discovery
include(GoogleTest)
gtest_discover_tests(utilities_test)
