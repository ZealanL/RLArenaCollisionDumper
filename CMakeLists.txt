cmake_minimum_required (VERSION 3.8)

project("RLArenaCollisionDumper")

# Add all headers and code files
file(GLOB_RECURSE CODE_FILES "${PROJECT_SOURCE_DIR}/src/*.cpp" "${PROJECT_SOURCE_DIR}/src/*.h")

add_executable(RLArenaCollisionDumper ${CODE_FILES})

set_target_properties(RLArenaCollisionDumper PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(RLArenaCollisionDumper PROPERTIES CXX_STANDARD 20)