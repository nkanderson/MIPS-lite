# Create test executable
add_executable(test_stats test_stats.cpp)

# Set C++ standard for the test
target_compile_features(test_stats PRIVATE cxx_std_17)

target_include_directories(test_stats PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Link against Google Test
target_link_libraries(test_stats PRIVATE mips_lite_lib gtest gtest_main)

# Register with CTest using Google Test's discovery
include(GoogleTest)
gtest_discover_tests(test_stats
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  PROPERTIES LABELS "libs"
)
