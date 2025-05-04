cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(audiotest)

set(CMAKE_C_STANDARD 11)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# rest of your project

add_executable(audiotest
	main.c 
	i2s.c
	nco.c
	adc_stream.c
	pitch_analysis.c

)
pico_generate_pio_header(audiotest ${CMAKE_CURRENT_LIST_DIR}/i2s.pio)


target_include_directories(audiotest
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/../
)

target_link_libraries(audiotest
	pico_stdlib 
	hardware_spi
	hardware_pio
	hardware_clocks
	hardware_irq
	hardware_dma
	hardware_gpio
	hardware_adc
	pico_multicore
	pico_bootrom
	hardware_i2c)

pico_enable_stdio_usb(audiotest 1) 
pico_enable_stdio_uart(audiotest 0) 

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(audiotest)

