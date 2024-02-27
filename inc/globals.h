#ifndef GLOBALS_H
#define GLOBALS_H

#include <stm32f411xe.h>
#include <gpio.h>
#include "dma.h"


#define QUEUE_SIZE 1024 
#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 128
#define NUM_FRAMES 30

typedef struct DMApacket
{
    uint8_t packets[4];
    uint16_t num_items;
    enum CommandFlag command;
    
} DMApacket;


typedef enum EncoderInput 
{
    ENCODER_FORWARD,
    ENCODER_REVERSE,
    ENCODER_NO_INPUT

} EncoderInput;


extern volatile EncoderInput direction;

extern uint32_t dma_queue_head;
extern uint32_t dma_queue_insert_ptr;
extern uint32_t dma_queue_items;
extern DMApacket global_dma_queue[QUEUE_SIZE];
extern DMApacket first_packet;
extern enum CommandFlag last_command;
extern uint8_t imageBytes_lock;
extern uint16_t prev_items;

extern uint8_t imageBytes[2*SCREEN_WIDTH*SCREEN_HEIGHT];
extern const uint8_t gifBytes[];

#endif
