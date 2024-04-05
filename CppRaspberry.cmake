# CppRaspberry library

# We require a target app name to be defined
if(NOT APP_NAME)
    message(FATAL_ERROR "APP_NAME is not defined")
endif(NOT APP_NAME)

set(CPP_RASPBERRY_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/include)
set(CPP_RASPBERRY_SOURCES ${CMAKE_CURRENT_LIST_DIR}/src/raspberry_pi.cpp)
set(CPP_RASPBERRY_LIBS "")

if(TARGET_PICO)

    message(STATUS "Building for the Raspberry Pi Pico")
    add_compile_definitions(TARGET_PICO)

    set(CPP_RASPBERRY_SOURCES ${CPP_RASPBERRY_SOURCES} ${CMAKE_CURRENT_LIST_DIR}/src/pico.cpp)


    if(HAVE_I2C)
        message(STATUS "HAVE_I2C is enabled")
        add_compile_definitions(HAVE_I2C)

        set(CPP_RASPBERRY_SOURCES ${CPP_RASPBERRY_SOURCES} ${CMAKE_CURRENT_LIST_DIR}/src/interfaces/pico_i2c.cpp ${CMAKE_CURRENT_LIST_DIR}/src/devices/lcd2x16.cpp)
        set(CPP_RASPBERRY_LIBS ${CPP_RASPBERRY_LIBS} hardware_i2c)
    endif(HAVE_I2C)

    if(HAVE_SPI)
        message(STATUS "HAVE_SPI is enabled")
        add_compile_definitions(HAVE_SPI)

        set(CPP_RASPBERRY_SOURCES ${CPP_RASPBERRY_SOURCES} ${CMAKE_CURRENT_LIST_DIR}/src/interfaces/pico_spi.cpp)
        set(CPP_RASPBERRY_LIBS ${CPP_RASPBERRY_LIBS} hardware_spi)
    endif(HAVE_SPI)

    if(HAVE_PWM)
        message(STATUS "PICO_PWM is enabled")
        add_compile_definitions(HAVE_PWM)

        set(CPP_RASPBERRY_LIBS ${CPP_RASPBERRY_LIBS} hardware_pwm)
    endif(HAVE_PWM)

elseif(TARGET_ZERO2W)

    message(STATUS "Building for the Raspberry Pi Zero 2W")
    add_compile_definitions(TARGET_ZERO2W)

    set(CPP_RASPBERRY_SOURCES ${CPP_RASPBERRY_SOURCES} ${CMAKE_CURRENT_LIST_DIR}/src/zero2w.cpp)

    if(HAVE_I2C)
        message(STATUS "ZERO2W_I2C is enabled")
        add_compile_definitions(HAVE_I2C)

        set(CPP_RASPBERRY_SOURCES ${CPP_RASPBERRY_SOURCES} ${CMAKE_CURRENT_LIST_DIR}/src/interfaces/zero2w_i2c.cpp)
        set(CPP_RASPBERRY_LIBS ${CPP_RASPBERRY_LIBS} i2c)
    endif(HAVE_I2C)

endif(TARGET_PICO)

macro(add_cpp_raspberry_app APP_NAME)
    message(STATUS "Adding CppRaspberry library to ${APP_NAME}")

    target_include_directories(${APP_NAME} PUBLIC ${CPP_RASPBERRY_INCLUDES})
    target_sources(${APP_NAME} PUBLIC ${CPP_RASPBERRY_SOURCES})
    target_link_libraries(${APP_NAME} ${CPP_RASPBERRY_LIBS})
endmacro(add_cpp_raspberry_app)