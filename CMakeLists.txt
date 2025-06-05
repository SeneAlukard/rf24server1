cmake_minimum_required(VERSION 3.16)
project(gbs LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(external/RF24 external/RF24)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/src/*.cpp
)

add_executable(gbs_program ${SRC_FILES})

target_include_directories(gbs_program PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/external/RF24
)

target_link_libraries(gbs_program PRIVATE rf24)

add_custom_command(TARGET gbs_program POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_SOURCE_DIR}/compile_commands.json)

add_custom_target(run COMMAND gbs_program DEPENDS gbs_program
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
