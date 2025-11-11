cmake_minimum_required(VERSION 3.16)
project(MarginCallReport)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(MarginCallReport SHARED
        src/MarginCallPluginInterface.cpp
)

target_include_directories(MarginCallReport PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/api
        ${CMAKE_SOURCE_DIR}/external
)
