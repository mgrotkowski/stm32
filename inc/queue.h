#ifndef QUEUE_H
#define QUEUE_H
#include <stm32f411xe.h>
#include <gpio.h>
#include <irq.h>
#include "globals.h"

void global_queue_insert( uint32_t data_location, 
                          uint32_t num_items,
                          enum CommandFlag command
                          );


#endif
