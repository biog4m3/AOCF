cmake_minimum_required(VERSION 3.18)

project(AOCF)


string(TOLOWER ${CMAKE_SYSTEM_NAME} PLATFORM)
if (PLATFORM STREQUAL "windows")
    set(PLATFORM "win64")
elseif(PLATFORM STREQUAL "linux")
    set(PLATFORM "x11")
endif()


add_library(AOCF SHARED 
src/aocf_platform_${PLATFORM}.cpp
src/aocf_gl.cpp
)
target_include_directories(AOCF PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(AOCF PRIVATE AOCF_EXPORTS)
target_link_libraries(AOCF PRIVATE opengl32)
