cmake_minimum_required(VERSION 3.16)
project(gbs LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_subdirectory(${CMAKE_SOURCE_DIR}/external/RF24 external/RF24)

find_package(Qt5 REQUIRED COMPONENTS Widgets)

set(CLI_SOURCES
    ${CMAKE_SOURCE_DIR}/src/gbs.cpp
    ${CMAKE_SOURCE_DIR}/src/radio.cpp
    ${CMAKE_SOURCE_DIR}/src/main.cpp
)

set(GUI_SOURCES
    ${CMAKE_SOURCE_DIR}/src/gbs.cpp
    ${CMAKE_SOURCE_DIR}/src/radio.cpp
    ${CMAKE_SOURCE_DIR}/src/gui_main.cpp
)

add_executable(gbs_program ${CLI_SOURCES})
add_executable(gbs_gui ${GUI_SOURCES})

target_include_directories(gbs_program PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/external/RF24
)
target_include_directories(gbs_gui PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/external/RF24
)

target_link_libraries(gbs_program PRIVATE rf24)
target_link_libraries(gbs_gui PRIVATE rf24 Qt5::Widgets)

add_custom_command(TARGET gbs_program POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_SOURCE_DIR}/compile_commands.json)

add_custom_target(run COMMAND gbs_program DEPENDS gbs_program
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_custom_target(run_gui COMMAND gbs_gui DEPENDS gbs_gui
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
