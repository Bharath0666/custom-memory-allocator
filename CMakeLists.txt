cmake_minimum_required(VERSION 3.16)

# Project configuration
project(CustomMemoryAllocator
    VERSION 1.0.0
    DESCRIPTION "Custom Memory Allocator - C++ Implementation"
    LANGUAGES CXX
)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /WX-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/memory_allocator.cpp
    src/main.cpp
)

# Header files
set(HEADERS
    include/memory_allocator.hpp
)

# Create executable
add_executable(MemoryAllocator ${SOURCES} ${HEADERS})

# Set output directory
set_target_properties(MemoryAllocator PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Debug configuration
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(MemoryAllocator PRIVATE DEBUG_MODE)
endif()

# Print configuration info
message(STATUS "")
message(STATUS "=== Custom Memory Allocator Configuration ===")
message(STATUS "CMake version: ${CMAKE_VERSION}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "")
