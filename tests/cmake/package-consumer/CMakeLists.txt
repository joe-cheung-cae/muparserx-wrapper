cmake_minimum_required(VERSION 3.14)
project(tfp_expression_package_smoke LANGUAGES CXX)

find_package(tfpExpression CONFIG REQUIRED)

add_executable(package_smoke main.cpp)
target_link_libraries(package_smoke PRIVATE tfp::expression)
target_compile_features(package_smoke PRIVATE cxx_std_17)
