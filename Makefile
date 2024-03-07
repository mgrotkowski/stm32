SRC_DIR = src
SRC = $(wildcard $(SRC_DIR)/**/*.c)
PWD = $(realpath .)
EXCLUDED_FILE = $(SRC_DIR)/lcd/lcd_clear.c $(SRC_DIR)/lcd/lcd.c $(SRC_DIR)/lcd/lcd_spi.c $(SRC_DIR)/images/cat_spin.c
INC_DIR = /opt
OBJ = $(SRC:.c=.o)
OBJ := $(filter-out $(patsubst %.c,%.o,$(EXCLUDED_FILE)),$(OBJ))
CC = arm-eabi-gcc
OBJCOPY = arm-eabi-objcopy
FLAGS = -mthumb -mcpu=cortex-m4
CPPFLAGS = -DSTM32F411xE
CFLAGS = $(FLAGS) -Wall -g -g3 \
-O2 -ffunction-sections -fdata-sections \
-I$(INC_DIR)/arm/stm32/inc \
-I$(INC_DIR)/arm/stm32/CMSIS/Include \
-I$(INC_DIR)/arm/stm32/CMSIS/Device/ST/STM32F4xx/Include \
-I$(PWD)/inc
LDFLAGS = $(FLAGS) -Wl,--gc-sections -nostartfiles \
-L$(INC_DIR)/arm/stm32/lds -Tstm32f411re.lds
vpath %.c $(INC_DIR)/arm/stm32/src src $(INC_DIR)/arm/stm32/inc
OBJECTS = main.o startup_stm32.o $(OBJ) delay.o gpio.o fonts.o
TARGET = main
.SECONDARY: $(TARGET).elf $(OBJECTS)
all: $(TARGET).bin
%.elf : $(OBJECTS)
		$(CC) $(LDFLAGS) $^ -o $@
%.bin : %.elf
		$(OBJCOPY) $< $@ -O binary
clean :
		rm -f *.bin *.elf *.hex *.d $(OBJECTS) *.bak *~
print-info:
	$(info OBJ : $(OBJ))
