add_library(logical_instruction_set
    ${CMAKE_SOURCE_DIR}/src/logical_instruction_set.cpp
)

add_executable(logical_instruction_test
    logical_instruction_test.cpp
)

target_link_libraries(logical_instruction_test
    PRIVATE
    gtest
    gtest_main
    mips_lite_lib
    logical_instruction_set
    
)

include(GoogleTest)
gtest_discover_tests(logical_instruction_test)
