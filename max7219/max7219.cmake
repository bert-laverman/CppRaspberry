message(STATUS "Adding files for the SPI connected MAX7219 numeric display driver")

if(NOT HAVE_SPI)
message(FATAL_ERROR "A SPI interface is required for the MAX7219 driver")
endif()

set(CPP_RASPBERRY_INCLUDES ${CPP_RASPBERRY_INCLUDES}
    ${CMAKE_CURRENT_LIST_DIR}/include)


set(CPP_RASPBERRY_LIBS ${CPP_RASPBERRY_LIBS} pigpiod_if2)
