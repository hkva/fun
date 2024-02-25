cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(Fun)

#
# Generator settings
#

# Build to bin/
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin)

# Export compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

#
# Shared options
#
add_library(common INTERFACE)

if(WIN32)
    target_compile_definitions(common INTERFACE _CRT_SECURE_NO_WARNINGS)
endif()

if(MSVC)
    target_compile_options(common INTERFACE /W4)
else()
    target_compile_options(common INTERFACE
        -Wall -Wextra
        -Wno-unused-parameter
    )
endif()

target_compile_features(common INTERFACE c_std_99)

# MD5 hash
add_executable(md5 "${CMAKE_CURRENT_LIST_DIR}/md5.c")
target_link_libraries(md5 PRIVATE common)

# WinAPI demos
if(WIN32)
    add_executable(winapi-tray WIN32
        "${CMAKE_CURRENT_LIST_DIR}/winapi-tray.c"
        "${CMAKE_CURRENT_LIST_DIR}/winapi-tray.rc"
    )
    target_link_libraries(winapi-tray PRIVATE common)
endif()