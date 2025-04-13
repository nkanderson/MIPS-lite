# Simple toolchain file for MIPS Lite Pipeline Simulator
# TODO: What flags are needed for the MIPS simulator?
# I've included Claude generate generic ones, and also additional ones
# that are commented out for now.

# Specify the C++ compiler (adjust as needed for your team)
set(CMAKE_CXX_COMPILER g++)
# You could also use: clang++, MSVC (cl.exe), etc.

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)          # Use C++17 standard
set(CMAKE_CXX_STANDARD_REQUIRED ON) # Require C++17 support (fail if not available)
set(CMAKE_CXX_EXTENSIONS OFF)       # Use standard C++17 instead of GNU extensions

# Basic compiler flags
set(CMAKE_CXX_FLAGS_INIT "-Wall -Wextra")
# -Wall:  Enable all common warning messages
# -Wextra: Enable extra warning messages beyond -Wall

set(CMAKE_CXX_FLAGS_DEBUG_INIT "-g")
# -g: Include debugging information in the executable

set(CMAKE_CXX_FLAGS_RELEASE_INIT "-O2")
# -O2: Optimize for speed, but not at the expense of binary size

# Additional useful flags (commented out by default)
# set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} -Wpedantic")
# -Wpedantic: Issue warnings for non-standard C++ features

# set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} -Werror")
# -Werror: Treat warnings as errors (build will fail if warnings are generated)

# set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} -Wshadow")
# -Wshadow: Warn when a variable declaration shadows another variable

# set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} -Wconversion")
# -Wconversion: Warn about implicit conversions that may change a value

# set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} -Wcast-align")
# -Wcast-align: Warn when a pointer cast increases alignment requirements

# set(CMAKE_CXX_FLAGS_DEBUG_INIT "${CMAKE_CXX_FLAGS_DEBUG_INIT} -fno-omit-frame-pointer")
# -fno-omit-frame-pointer: Improved debugging at very little cost to performance

# set(CMAKE_CXX_FLAGS_RELEASE_INIT "${CMAKE_CXX_FLAGS_RELEASE_INIT} -O3")
# -O3: Maximum optimization for speed, potentially larger binary size than -O2

# set(CMAKE_CXX_FLAGS_RELEASE_INIT "${CMAKE_CXX_FLAGS_RELEASE_INIT} -DNDEBUG")
# -DNDEBUG: Disable assert() macros in release builds for better performance

# set(CMAKE_CXX_FLAGS_RELEASE_INIT "${CMAKE_CXX_FLAGS_RELEASE_INIT} -flto")
# -flto: Link-time optimization which can improve performance

# set(CMAKE_CXX_FLAGS_MINSIZEREL_INIT "-Os -DNDEBUG")
# -Os: Optimize for size rather than speed

# set(CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT "-O2 -g -DNDEBUG")
# Combined optimizations (-O2) with debug info (-g)

# Platform detection for minimal platform-specific settings
if(WIN32)
  # Windows-specific settings (if needed)

elseif(APPLE)
  # macOS-specific settings (if needed)

elseif(UNIX)
  # Linux-specific settings (if needed)

endif()
