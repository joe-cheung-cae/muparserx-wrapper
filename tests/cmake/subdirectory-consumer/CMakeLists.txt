cmake_minimum_required(VERSION 3.14)
project(tfp_expression_subdirectory_smoke LANGUAGES CXX)

find_package(muparserx REQUIRED CONFIG)
find_package(nlohmann_json REQUIRED CONFIG)

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../.." "${CMAKE_CURRENT_BINARY_DIR}/muparserx-wrapper")

add_executable(subdirectory_smoke main.cpp)
target_link_libraries(subdirectory_smoke PRIVATE tfp::expression)
target_compile_features(subdirectory_smoke PRIVATE cxx_std_17)
