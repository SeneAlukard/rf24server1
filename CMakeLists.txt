cmake_minimum_required(VERSION 3.16)
project(gbs LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_subdirectory(${CMAKE_SOURCE_DIR}/external/RF24 external/RF24)

option(BUILD_GUI "Build GUI application" OFF)
if(BUILD_GUI)
    find_package(Qt5 REQUIRED COMPONENTS Widgets)
endif()

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
if(BUILD_GUI)
    add_executable(gbs_gui ${GUI_SOURCES})
endif()

target_include_directories(gbs_program PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/external/RF24
)
if(BUILD_GUI)
    target_include_directories(gbs_gui PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/external/RF24
    )
endif()

target_link_libraries(gbs_program PRIVATE rf24)
if(BUILD_GUI)
    target_link_libraries(gbs_gui PRIVATE rf24 Qt5::Widgets)
endif()

# Simple test to receive telemetry (MPU) data without GUI
add_executable(mpu_receive_test
    ${CMAKE_SOURCE_DIR}/tests/mpu_receive_test.cpp
    ${CMAKE_SOURCE_DIR}/src/radio.cpp)

target_include_directories(mpu_receive_test PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/external/RF24)

target_link_libraries(mpu_receive_test PRIVATE rf24)

add_custom_command(TARGET gbs_program POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_SOURCE_DIR}/compile_commands.json)

add_custom_target(run COMMAND gbs_program DEPENDS gbs_program
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

if(BUILD_GUI)
    add_custom_target(run_gui COMMAND gbs_gui DEPENDS gbs_gui
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endif()

# Simple server to read gyro data from SimplePacket drones
add_executable(gyro_server
    ${CMAKE_SOURCE_DIR}/src/gyro_server.cpp
    ${CMAKE_SOURCE_DIR}/src/radio.cpp)

target_include_directories(gyro_server PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/external/RF24)

target_link_libraries(gyro_server PRIVATE rf24)
