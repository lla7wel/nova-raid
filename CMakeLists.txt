cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico_w CACHE STRING "Target board (pico_w for the kit's Pico W; pico also works)")

include(pico_sdk_import.cmake)

project(nova_raid C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(nova_raid
    src/main.c
    src/hal/hal_pico.c
    src/hal/st7796.c
    src/gfx/gfx.c
    src/gfx/sprites.c
    src/game/game.c
    src/game/entities.c
    src/game/waves.c
    src/game/boss.c
    src/game/starfield.c
    src/game/hud.c
    src/game/audio_seq.c
    src/game/ledfx.c
    src/game/hiscore.c
    src/game/diag.c
)

pico_generate_pio_header(nova_raid ${CMAKE_CURRENT_LIST_DIR}/src/hal/ws2812.pio)

target_include_directories(nova_raid PRIVATE src)

target_link_libraries(nova_raid
    pico_stdlib
    pico_multicore
    pico_rand
    hardware_spi
    hardware_dma
    hardware_pwm
    hardware_adc
    hardware_pio
    hardware_flash
    hardware_sync
)

# USB CDC serial for the diagnostics log; UART pins stay free for the user.
pico_enable_stdio_usb(nova_raid 1)
pico_enable_stdio_uart(nova_raid 0)

pico_add_extra_outputs(nova_raid)
