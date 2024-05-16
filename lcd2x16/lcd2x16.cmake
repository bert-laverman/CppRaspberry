message(STATUS "Adding files for the I2C connected 2x16 LCD display")

if(NOT HAVE_I2C)
message(FATAL_ERROR "An I2C interface is required for the LCD2x16 device")
endif()

set(CPP_RASPBERRY_INCLUDES ${CPP_RASPBERRY_INCLUDES}
    ${CMAKE_CURRENT_LIST_DIR}/include)

set(CPP_RASPBERRY_SOURCES ${CPP_RASPBERRY_SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/src/devices/lcd2x16.cpp)