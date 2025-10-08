find_package(GTest 1.10 REQUIRED NO_MODULE)


add_executable(projectM_EvalLib_Test
        InstructionListTest.cpp
        InstructionListTest.hpp
        PrecedenceTest.cpp
        PrecedenceTest.hpp
        Stubs.cpp
        TreeFunctionsTest.cpp
        )


target_link_libraries(projectM_EvalLib_Test
        PRIVATE
        projectM::Eval
        GTest::gtest_main
        )

target_include_directories(projectM_EvalLib_Test
        PRIVATE
        ${CMAKE_SOURCE_DIR}/SimpleCompiler
        )

target_compile_definitions(projectM_EvalLib_Test
        PRIVATE
        PROJECTM_TEST_DATA_DIR="${CMAKE_CURRENT_LIST_DIR}/data"
        )

add_test(NAME projectM_EvalLib_Test COMMAND projectM_EvalLib_Test)
