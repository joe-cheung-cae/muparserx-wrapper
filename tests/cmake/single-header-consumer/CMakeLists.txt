cmake_minimum_required(VERSION 3.14)
project(single_header_consumer LANGUAGES CXX)

find_package(muparserx REQUIRED CONFIG)
find_package(nlohmann_json 3.11.0 REQUIRED CONFIG)

add_executable(single_header_consumer
    main.cpp
    secondary.cpp
)

target_compile_features(single_header_consumer PRIVATE cxx_std_17)
target_include_directories(single_header_consumer PRIVATE
    ../../../single_include
    ${muparserx_INCLUDE_DIRS}
)
target_link_libraries(single_header_consumer PRIVATE
    ${muparserx_LIBRARIES}
    nlohmann_json::nlohmann_json
)
