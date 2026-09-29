cmake_minimum_required(VERSION 3.16)
project(LinuxSentinel VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Strict warnings for systems code quality
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

include_directories(${PROJECT_SOURCE_DIR}/include)

# ----------------------------------------------------
# Target 1: Linux Sentinel Supervisor Daemon
# ----------------------------------------------------
set(SENTINEL_SOURCES
    src/main.cpp
    src/HealthCollector.cpp
    src/AnomalyDetector.cpp
    src/Logger.cpp
)

add_executable(sentinel ${SENTINEL_SOURCES})

# ----------------------------------------------------
# Target 2: Synthetic Fault Injection Application
# ----------------------------------------------------
add_executable(fault_app test_service/fault_app.cpp)

# Output binaries directly in project root or build/
set_target_properties(sentinel PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set_target_properties(fault_app PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
