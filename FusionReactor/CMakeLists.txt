# set the project name and version
project(FusionReactor VERSION 1.0)

#set include directories
set(INC_DIR
${PROJECT_SOURCE_DIR}/src/
)

#include all source files
file(GLOB_RECURSE SRC_FILES 
${PROJECT_SOURCE_DIR}/src/*.h
${PROJECT_SOURCE_DIR}/src/*.hpp
${PROJECT_SOURCE_DIR}/src/*.c
${PROJECT_SOURCE_DIR}/src/*.cpp
)

macro(GroupSources curdir)
   file(GLOB children RELATIVE ${PROJECT_SOURCE_DIR}/${curdir}
${PROJECT_SOURCE_DIR}/${curdir}/*)
   foreach(child ${children})
          if(IS_DIRECTORY ${PROJECT_SOURCE_DIR}/${curdir}/${child})
                  GroupSources(${curdir}/${child})
          else()
                  string(REPLACE "/" "\\" groupname ${curdir})
                # I would like to call the src root folder in a different name, only in visual studio (not mandatory requirement)
                  # string(REPLACE "src" "Common" groupname ${groupname})
            source_group(${groupname} FILES
${PROJECT_SOURCE_DIR}/${curdir}/${child})
          endif()
   endforeach()
endmacro()

# Group files within the "src" folder in the correct folder strcuture
GroupSources(src)

include_directories(${INC_DIR})
add_library(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE ${SRC_FILES})
#set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${EXECUTABLE_OUTPUT_PATH}")
