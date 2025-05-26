# Function to create a test executable with common settings
function(create_simulator_test TEST_NAME SOURCE_FILE)
    add_executable(${TEST_NAME} ${SOURCE_FILE})
    
    # Common compile definitions and features
    target_compile_definitions(${TEST_NAME} PRIVATE UNIT_TEST)
    target_compile_features(${TEST_NAME} PRIVATE cxx_std_17)
    
    # Common libraries
    target_link_libraries(${TEST_NAME}
        PRIVATE
        gtest
        gmock
        gtest_main
        mips_lite_lib
    )
    
    # Register with CTest
    include(GoogleTest)
    gtest_discover_tests(${TEST_NAME})
endfunction()

# Create all test executables using the function
create_simulator_test(functional_simulator_test functional_simulator_tests.cpp)
create_simulator_test(decode_stage_tests decode_stage_tests.cpp)
create_simulator_test(exe_stage_tests exe_stage_tests.cpp)
create_simulator_test(fetch_stage_tests fetch_stage_tests.cpp)

# Add the integration tests later...
# create_simulator_test(functional_simulator_integration_test functional_simulator_integration_tests.cpp)