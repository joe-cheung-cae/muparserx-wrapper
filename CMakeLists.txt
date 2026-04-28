cmake_minimum_required(VERSION 3.14)
project(muparserx_wrapper CXX)

include(FetchContent)
include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(TFP_EXPRESSION_BUILD_TESTS "enable building tfp expression runtime tests" ON)
option(TFP_EXPRESSION_BUILD_EXAMPLES "enable building tfp expression runtime examples" ON)

set(BUILD_EXAMPLES OFF CACHE BOOL "disable muparserx examples" FORCE)
add_subdirectory(muparserx)

find_package(nlohmann_json 3.11.0 QUIET)
if(NOT nlohmann_json_FOUND)
    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

set(TFP_EXPRESSION_SOURCES
    src/utility/expression/constant_resolver.cpp
    src/utility/expression/expression_compiler.cpp
    src/utility/expression/expression_config_loader.cpp
    src/utility/expression/expression_error.cpp
    src/utility/expression/expression_handle.cpp
    src/utility/expression/expression_runtime.cpp
    src/utility/expression/muparserx_backend.cpp
    src/utility/expression/muparserx_table_callback.cpp
    src/utility/expression/runtime_expression.cpp
    src/utility/expression/table_function.cpp
)

add_library(tfp_expression ${TFP_EXPRESSION_SOURCES})
target_compile_features(tfp_expression PUBLIC cxx_std_17)
target_include_directories(tfp_expression
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/utility/expression
        ${CMAKE_CURRENT_SOURCE_DIR}/muparserx/parser
)
target_link_libraries(tfp_expression
    PRIVATE
        muparserx
        nlohmann_json::nlohmann_json
)

if(TFP_EXPRESSION_BUILD_TESTS)
    enable_testing()
    set(TFP_EXPRESSION_TESTS
        test_table_function
        test_json_loader
        test_constant_resolver
        test_expression_runtime
        test_expression_evaluate_loop
    )

    foreach(test_name ${TFP_EXPRESSION_TESTS})
        add_executable(${test_name} tests/utility/expression/${test_name}.cpp)
        target_include_directories(${test_name} PRIVATE tests/utility/expression)
        target_link_libraries(${test_name} PRIVATE tfp_expression)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
endif()

if(TFP_EXPRESSION_BUILD_EXAMPLES)
    add_executable(expression_runtime_example examples/utility/expression_runtime_example.cpp)
    target_link_libraries(expression_runtime_example PRIVATE tfp_expression)
endif()
