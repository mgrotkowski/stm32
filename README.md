This repo contains a simple communication protocol with ST7355s LCD display, written for STM32F411 Nucleo board.
Notable features:
- written from scratch using only CMSIS
- DMA + SPI with interrupts for command/data transfer to LCD
- simplified LZW decoding for GIF with hardcoded header locations/no header parsing
- TODO: display GIFs forwards/backwards based on rotary encoder input - using a counter and interrupts
