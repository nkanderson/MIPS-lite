cmake_minimum_required(VERSION 3.15)

# Set up Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.13.0
)
FetchContent_MakeAvailable(googletest)

# Create test executable
add_executable(setup_test setup_test.cpp)

# Set C++ standard for the test
target_compile_features(setup_test PRIVATE cxx_std_17)

# Link against Google Test
target_link_libraries(setup_test PRIVATE gtest gtest_main)

# Register with CTest using Google Test's discovery
include(GoogleTest)
gtest_discover_tests(setup_test)
