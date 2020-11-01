# set the project name and version
project(Editor VERSION 1.0)
print_variables()

set(INC_DIR
${CMAKE_SOURCE_DIR}/
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
add_executable(${PROJECT_NAME})

#set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

target_sources(${PROJECT_NAME} PUBLIC ${SRC_FILES})
target_link_libraries(${PROJECT_NAME} FusionReactor)
target_link_libraries(${PROJECT_NAME} FusionReactor_DX12)

#Copy libs folder to binary
file(GLOB filesToCopy
    ${PROJECT_SOURCE_DIR}/libs/*.dll
    ${CMAKE_SOURCE_DIR}/FusionReactor/libs/*.dll
    ${CMAKE_SOURCE_DIR}/FusionReactor_DX12/libs/*.dll
)

function(copyFiles targ fileList dest)
    foreach(fileNameFull ${fileList})
        get_filename_component(file ${fileNameFull} NAME)
        add_custom_command(        
            TARGET ${targ}
            POST_BUILD
            COMMAND cmake -E make_directory ${dest}
            COMMAND cmake -E copy_if_different ${fileNameFull} ${dest}
            #MAIN_DEPENDENCY ${fileNameFull}
            BYPRODUCTS " "
            #BYPRODUCTS "${dest} ${dest}/${file}"
            COMMENT "COPYING ${file} into ${dest}\t"
        )
    endforeach()
endfunction(copyFiles)
#Copy Dll Files
#copyFiles(${PROJECT_NAME} "${filesToCopy}" "${PROJECT_BINARY_DIR}/$<CONFIG>")
copyFiles(${PROJECT_NAME} "${filesToCopy}" "${CMAKE_SOURCE_DIR}/bin")
