cmake_minimum_required(VERSION 4.1)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

enable_language(C ASM)

add_library(common-io-basic INTERFACE)

add_subdirectory(target)

target_sources(common-io-basic PUBLIC utils/linked_list.c)
target_include_directories(common-io-basic INTERFACE include utils)
target_link_libraries(common-io-basic INTERFACE BOARD_TARGET)