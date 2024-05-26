# CppRaspberry library

# We require a target app name to be defined
if(NOT APP_NAME)
    message(FATAL_ERROR "APP_NAME is not defined")
endif(NOT APP_NAME)

set(CPP_RASPBERRY_INCLUDES
    ${CMAKE_CURRENT_LIST_DIR}/include)
set(CPP_RASPBERRY_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/raspberry-pi.cpp)
set(CPP_RASPBERRY_LIBS "")

# Add in interface specific stuff

if(HAVE_I2C)
    include(${CMAKE_CURRENT_LIST_DIR}/i2c/i2c.cmake)
endif(HAVE_I2C)

if(HAVE_SPI)
    include(${CMAKE_CURRENT_LIST_DIR}/spi/spi.cmake)
endif()

# The target platform is set with a "TARGET_XYZ" variable.
if(TARGET_PICO)
    include(${CMAKE_CURRENT_LIST_DIR}/pico/pico.cmake)
elseif(TARGET_ZERO2W)
    include(${CMAKE_CURRENT_LIST_DIR}/zero2w/zero2w.cmake)
endif(TARGET_PICO)

# And also add device specific stuff

if(HAVE_LCD2X16)
    include(${CMAKE_CURRENT_LIST_DIR}/lcd2x16/lcd2x16.cmake)
endif()

if(HAVE_MAX7219)
    include(${CMAKE_CURRENT_LIST_DIR}/max7219/max7219.cmake)
endif()

macro(add_cpp_raspberry_app APP_NAME)
    message(STATUS "Adding CppRaspberry library to ${APP_NAME}")

    target_include_directories(${APP_NAME} PUBLIC ${CPP_RASPBERRY_INCLUDES})
    target_sources(${APP_NAME} PUBLIC ${CPP_RASPBERRY_SOURCES})
    target_link_libraries(${APP_NAME} ${CPP_RASPBERRY_LIBS})
endmacro(add_cpp_raspberry_app)