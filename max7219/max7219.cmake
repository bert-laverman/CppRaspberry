message(STATUS "Adding files for the SPI connected MAX7219 numeric display driver")

if(NOT HAVE_SPI)
message(FATAL_ERROR "A SPI interface is required for the MAX7219 driver")
endif()

set(CPP_RASPBERRY_INCLUDES ${CPP_RASPBERRY_INCLUDES}
    ${CMAKE_CURRENT_LIST_DIR}/include)

set (CPP_RASPBERRY_SOURCES ${CPP_RASPBERRY_SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/src/devices/local-max7219.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/devices/remote-max7219.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/util/max7219-state.cpp)
