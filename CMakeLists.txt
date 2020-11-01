cmake_minimum_required(VERSION 3.10)
include(scripts.cmake)
# set the project name and version
project(Root VERSION 1.0)

##print_variables()

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
#set_property(TARGET Editor PROPERTY RUNTIME_OUTPUT_DIRECTORY "${EXECUTABLE_OUTPUT_PATH}")

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
#set(VS_DEBUGGER_WORKING_DIRECTORY "${EXECUTABLE_OUTPUT_PATH}")

add_subdirectory(FusionReactor)
add_subdirectory(FusionReactor_DX12)
add_subdirectory(Editor)
