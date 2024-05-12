message(STATUS "Adding generic spi files")

add_compile_definitions(HAVE_SPI)

set(CPP_RASPBERRY_INCLUDES ${CPP_RASPBERRY_INCLUDES}
    ${CMAKE_CURRENT_LIST_DIR}/include)
