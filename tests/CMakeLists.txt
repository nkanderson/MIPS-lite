cmake_minimum_required(VERSION 3.15)

# Set up Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.13.0
)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_subdirectory(proj-setup)
add_subdirectory(libs)
