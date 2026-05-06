cmake_minimum_required(VERSION 3.14)
project(muparserx_wrapper VERSION 0.0.4 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(TFP_EXPRESSION_IS_TOP_LEVEL OFF)
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(TFP_EXPRESSION_IS_TOP_LEVEL ON)
endif()

option(TFP_EXPRESSION_BUILD_TESTS "enable building tfp expression runtime tests" ${TFP_EXPRESSION_IS_TOP_LEVEL})
option(TFP_EXPRESSION_BUILD_EXAMPLES "enable building tfp expression runtime examples" ${TFP_EXPRESSION_IS_TOP_LEVEL})
option(TFP_EXPRESSION_BUILD_STATIC "build tfp_expression as a static library" OFF)
option(TFP_EXPRESSION_STATIC_GNU_RUNTIME "statically link libstdc++ and libgcc for this library on GNU/MinGW toolchains" ON)

find_package(muparserx REQUIRED CONFIG)
find_package(nlohmann_json 3.11.0 REQUIRED CONFIG)

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

if(TFP_EXPRESSION_BUILD_STATIC)
    add_library(tfp_expression STATIC ${TFP_EXPRESSION_SOURCES})
else()
    add_library(tfp_expression SHARED ${TFP_EXPRESSION_SOURCES})
endif()

add_library(tfp::expression ALIAS tfp_expression)

set_target_properties(tfp_expression PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    EXPORT_NAME expression
)

target_compile_features(tfp_expression PUBLIC cxx_std_17)
target_include_directories(tfp_expression
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/utility/expression
        ${muparserx_INCLUDE_DIRS}
)
target_link_libraries(tfp_expression
    PUBLIC
        ${muparserx_LIBRARIES}
    PRIVATE
        nlohmann_json::nlohmann_json
)

if(TFP_EXPRESSION_STATIC_GNU_RUNTIME AND
   (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "MinGW"))
    target_link_options(tfp_expression PRIVATE -static-libstdc++ -static-libgcc)
endif()

install(TARGETS tfp_expression
    EXPORT tfpExpressionTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT tfpExpressionTargets
    FILE tfpExpressionTargets.cmake
    NAMESPACE tfp::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/tfpExpression
)

configure_package_config_file(
    cmake/tfpExpressionConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/tfpExpressionConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/tfpExpression
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/tfpExpressionConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/tfpExpressionConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/tfpExpressionConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/tfpExpression
)

if(TFP_EXPRESSION_BUILD_TESTS)
    include(CTest)
    enable_testing()
    set(TFP_EXPRESSION_TESTS
        test_table_function
        test_json_loader
        test_constant_resolver
        test_expression_runtime
        test_expression_evaluate_loop
        test_expression_handle
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
