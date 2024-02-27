SRC_DIR = src
SRC = $(wildcard $(SRC_DIR)/**/*.c)
EXCLUDED_FILE = $(SRC_DIR)/lcd/lcd_clear.c $(SRC_DIR)/lcd/lcd.c $(SRC_DIR)/lcd/lcd_spi.c $(SRC_DIR)/images/cat_spin.c
#$(SRC_DIR)/lcd/lcd_dma.c 
OBJ = $(SRC:.c=.o)
OBJ := $(filter-out $(patsubst %.c,%.o,$(EXCLUDED_FILE)),$(OBJ))
CC = arm-eabi-gcc
OBJCOPY = arm-eabi-objcopy
FLAGS = -mthumb -mcpu=cortex-m4
CPPFLAGS = -DSTM32F411xE
CFLAGS = $(FLAGS) -Wall -g -g3 \
-O2 -ffunction-sections -fdata-sections \
-I/home/mgrot/builds/arm/stm32/inc \
-I/home/mgrot/builds/arm/stm32/CMSIS/Include \
-I/home/mgrot/builds/arm/stm32/CMSIS/Device/ST/STM32F4xx/Include \
-I/home/mgrot/Moje/Coding/STM32/inc
LDFLAGS = $(FLAGS) -Wl,--gc-sections -nostartfiles \
-L/home/mgrot/builds/arm/stm32/lds -Tstm32f411re.lds
vpath %.c /home/mgrot/builds/arm/stm32/src src /home/mgrot/builds/arm/stm32/inc
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
