message(STATUS "Adding Pico specific spi files")

set(CPP_RASPBERRY_INCLUDES ${CPP_RASPBERRY_INCLUDES}
    ${CMAKE_CURRENT_LIST_DIR}/include)

set(CPP_RASPBERRY_SOURCES ${CPP_RASPBERRY_SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/src/interfaces/pico_spi.cpp)

set(CPP_RASPBERRY_LIBS ${CPP_RASPBERRY_LIBS} hardware_spi)
    