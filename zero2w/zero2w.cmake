message(STATUS "Building for the Raspberry Pi Zero 2W")
add_compile_definitions(TARGET_ZERO2W)

set(CPP_RASPBERRY_INCLUDES ${CPP_RASPBERRY_INCLUDES}
    ${CMAKE_CURRENT_LIST_DIR}/include)

set(CPP_RASPBERRY_SOURCES ${CPP_RASPBERRY_SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/src/zero2w.cpp)

# Add in interface specific stuff for the Pico

if(HAVE_I2C)
    include(${CMAKE_CURRENT_LIST_DIR}/i2c/i2c.cmake)
endif(HAVE_I2C)

if(HAVE_SPI)
    include(${CMAKE_CURRENT_LIST_DIR}/spi/spi.cmake)
endif()
