if(${CMAKE_C_COMPILER_ID} STREQUAL GNU)
    set(CMAKE_C_COMPILER_NAME gcc)
elseif (${CMAKE_C_COMPILER_ID} STREQUAL ARMCC)
    set(CMAKE_C_COMPILER_NAME arm)
elseif (${CMAKE_C_COMPILER_ID} STREQUAL IAR)
    set(CMAKE_C_COMPILER_NAME iar)
else ()
    message(FATAL_ERROR "compiler not selected (GNU/ARMCC/IAR)")
endif ()

if(DEVICE MATCHES "F4")
    add_subdirectory(f4)
elseif(DEVICE MATCHES "F1")
    add_subdirectory(f1)
else ()
    message(FATAL_ERROR "unknown target" ${DEVICE})
endif ()

message(STATUS "target selected : " ${DEVICE})

file(GLOB VENDOR_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

add_library(VENDOR_TARGET INTERFACE)
target_sources(VENDOR_TARGET PUBLIC
    ${VENDOR_SOURCES}
)
target_include_directories(VENDOR_TARGET INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(VENDOR_TARGET INTERFACE HAL_TARGET)
