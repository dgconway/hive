cmake_minimum_required(VERSION 3.20)
project(BugsGameBackend VERSION 1.0 LANGUAGES CXX)

# C++20 Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find OpenMP
find_package(OpenMP REQUIRED)

# Fetch Dependencies
include(FetchContent)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_Declare(
    cpp-httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.18.3
)
FetchContent_MakeAvailable(nlohmann_json cpp-httplib)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/models.cpp
    src/hex_math.cpp
    src/game_logic.cpp
    src/game_interface.cpp
    src/evaluator.cpp
    src/minimax.cpp
    src/tunable_evaluator.cpp
    src/self_play.cpp
    src/weight_optimizer.cpp
    src/main.cpp
)

set(REPRO_SOURCES
    src/models.cpp
    src/hex_math.cpp
    src/game_logic.cpp
    src/game_interface.cpp
    src/evaluator.cpp
    src/minimax.cpp
    src/tunable_evaluator.cpp
    src/self_play.cpp
    src/weight_optimizer.cpp
    src/repro_crash.cpp
)

# Executable
add_executable(bugs_server ${SOURCES})
add_executable(repro_crash ${REPRO_SOURCES})

# Link Libraries
# Note: nlohmann_json targets are namespaced as nlohmann_json::nlohmann_json
target_link_libraries(bugs_server PRIVATE OpenMP::OpenMP_CXX nlohmann_json::nlohmann_json httplib::httplib)
target_link_libraries(repro_crash PRIVATE OpenMP::OpenMP_CXX nlohmann_json::nlohmann_json)

# Platform-specific settings
if(WIN32)
    target_link_libraries(bugs_server PRIVATE ws2_32)
    target_link_libraries(repro_crash PRIVATE ws2_32)
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(bugs_server PRIVATE /W4)
    target_compile_options(repro_crash PRIVATE /W4)
else()
    target_compile_options(bugs_server PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(repro_crash PRIVATE -Wall -Wextra -Wpedantic)
endif()


# Optional: Build tests
option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
