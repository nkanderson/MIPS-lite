### Root CMakeLists.txt for MIPS-Lite project
cmake_minimum_required(VERSION 3.15)

# Always define toolchain file so we have same setup
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/cmake/toolchain.cmake")

project(MIPS-Lite VERSION 0.1.0 LANGUAGES CXX)

# Cmakes built in test runner
include(CTest)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Ensure C++17 is available
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include directories will go here as you add them
include_directories(include)

# Setup GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.13.0
)
FetchContent_MakeAvailable(googletest)

set(SOURCE_FILES
    src/functional_simulator.cpp
    src/mips_mem_parser.cpp
    src/mips_instruction.cpp
    src/stats.cpp
)

# Create the library that will be used by tests and main executable
add_library(mips_lite_lib ${SOURCE_FILES})

# Create main binary target (will add actual source files later)
add_executable(mips_simulator src/main.cpp)

# Set debug flag for debug builds
target_compile_definitions(mips_simulator PRIVATE $<$<CONFIG:Debug>:DEBUG_MODE=1>)

# Link the library to the main executable
target_link_libraries(mips_simulator PRIVATE mips_lite_lib)

# Enable testing
enable_testing()
add_subdirectory(tests/proj_setup)
add_subdirectory(tests/mem_parser)
add_subdirectory(tests/libs)
add_subdirectory(tests/mips_instruction)
add_subdirectory(tests/reg_type)
add_subdirectory(tests/functional_simulator)
