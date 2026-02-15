add_subdirectory(arch)

if(NOT BOARD)
    message(FATAL_ERROR "BOARD not defined!")
endif ()

if(BOARD_DIR)
    add_subdirectory(${BOARD_DIR} ${BOARD_DIR}/build)
    message(STATUS "external board defined : ${BOARD_DIR}")
else()
    set(BOARD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/board/${BOARD})
    if(IS_DIRECTORY ${BOARD_DIR})
        add_subdirectory(${BOARD_DIR})
    endif ()
endif ()

if(DEVICE MATCHES "STM32")
    add_subdirectory(st)
endif ()