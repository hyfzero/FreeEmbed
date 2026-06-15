cmake_minimum_required(VERSION 3.16)
project(storage_abstraction_tests C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(HW_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../UserModules/hw_abstraction")

add_executable(storage_tests
    test_storage.c
    "${HW_ROOT}/bus/spi/src/spi_transport.c"
    "${HW_ROOT}/bus/i2c/src/i2c_transport.c"
    "${HW_ROOT}/storage/src/storage.c"
    "${HW_ROOT}/storage/spi_nor/src/spi_nor.c"
    "${HW_ROOT}/storage/i2c_eeprom/src/i2c_eeprom.c"
)

target_include_directories(storage_tests PRIVATE
    "${HW_ROOT}/common/include"
    "${HW_ROOT}/bus/spi/include"
    "${HW_ROOT}/bus/i2c/include"
    "${HW_ROOT}/storage/include"
    "${HW_ROOT}/storage/spi_nor/include"
    "${HW_ROOT}/storage/i2c_eeprom/include"
)

if(MSVC)
    target_compile_options(storage_tests PRIVATE /W4 /WX)
else()
    target_compile_options(storage_tests PRIVATE
        -Wall -Wextra -Werror -Wpedantic
    )
endif()

enable_testing()
add_test(NAME storage_tests COMMAND storage_tests)
