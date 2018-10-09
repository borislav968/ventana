#DEVICE = atmega8
#DEVICE_PROG = m8
DEVICE = atmega16
DEVICE_PROG = atmega16
CLOCK = 1000000
PORT = /dev/ttyUSB0
BAUD = 19200
PROGRAMMER = -c avr910 -P $(PORT) -b $(BAUD)
PROJECT = ventana
OBJECTS = $(patsubst %.c, $(BUILD)/%.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)
BUILD = build
SIM = sim
COMPILE = avr-gcc -gdwarf-2 -g3 -Wall -Os -DF_CPU=$(CLOCK) -mmcu=$(DEVICE)
OBJCOPY = avr-objcopy

AVRDUDE = avrdude $(PROGRAMMER) -p $(DEVICE) 
FUSES = -U hfuse:w:0x64:m -U lfuse:w:0xdd:m

.PHONY: all clean

all: clean $(BUILD)/$(PROJECT).elf
	cp -f $(BUILD)/$(PROJECT).elf $(SIM)
	cp -f *.c $(SIM)
	
hex: $(BUILD)/$(PROJECT).hex

$(BUILD)/%.o: %.c $(HEADERS)
	$(COMPILE) -c $< -o $@

$(BUILD)/$(PROJECT).elf: $(OBJECTS)
	$(COMPILE) -o $(BUILD)/$(PROJECT).elf $(OBJECTS)
	
$(BUILD)/$(PROJECT).hex: $(BUILD)/$(PROJECT).elf
	$(OBJCOPY) -j .text -j .data -O ihex $(BUILD)/$(PROJECT).elf $(BUILD)/$(PROJECT).hex

flash:	$(BUILD)/$(PROJECT).hex
	$(AVRDUDE) -U flash:w:$(BUILD)/$(PROJECT).hex:i
	
fuse:
	$(AVRDUDE) $(FUSES)
	
$(shell [ -d $(BUILD) ] || mkdir $(BUILD))
$(shell [ -d $(SIM) ] || mkdir $(SIM))

clean:
	rm -f $(BUILD)/*
	rm -f $(SIM)/*.c
	rm -f $(SIM)/$(PROJECT).elf
	
