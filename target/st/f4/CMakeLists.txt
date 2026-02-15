if(NOT HAL_SYSTEM_SOURCE)
    message(STATUS "system source defined in ${DEVICE}")
set(HAL_SYSTEM_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/cmsis-device-f4/Source/Templates/system_stm32f4xx.c)
endif ()
if(NOT HAL_STARTUP_SOURCE)
    message(STATUS "startup defined in ${DEVICE}")
    string(TOLOWER ${DEVICE} _DEVICE)
    set(HAL_STARTUP_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/cmsis-device-f4/Source/Templates/${CMAKE_C_COMPILER_NAME}/startup_${_DEVICE}.s)
endif ()

if(NOT TARGET_LINKER_FILE)
    message(FATAL_ERROR "linker file not defined")
endif ()

set(HAL_DEFINE_SYMBOLS ${HAL_DEFINE_SYMBOLS}
        USE_HAL_DRIVER
        STM32F4
        ${DEVICE}
        $<$<CONFIG:Debug>:DEBUG>
)

file(GLOB HAL_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx-hal-driver/Src/*.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx-hal-driver/Src/Legacy/*.c"
)
list(FILTER HAL_SOURCES EXCLUDE REGEX "_template\\.c$")

set(HAL_INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/cmsis-device-f4/Include
        ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx-hal-driver/Inc
        ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx-hal-driver/Inc/Legacy
)

add_library(HAL_TARGET INTERFACE)

target_sources(HAL_TARGET PUBLIC
        ${HAL_SOURCES}
        ${HAL_SYSTEM_SOURCE}
        ${HAL_STARTUP_SOURCE}
)

target_compile_definitions(HAL_TARGET INTERFACE ${HAL_DEFINE_SYMBOLS})
target_include_directories(HAL_TARGET INTERFACE ${HAL_INCLUDES})
target_link_libraries(HAL_TARGET INTERFACE
        ${TOOLCHAIN_LINK_LIBRARIES}
        CORE_LIBRARIES
)

target_link_options(HAL_TARGET INTERFACE
        -T ${TARGET_LINKER_FILE}
)
