CC = avr-gcc
CXX = avr-g++
AR = avr-ar
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
AVRSIZE = avr-size
AVRDUDE = avrdude

MCU = atmega2560
F_CPU = 16000000UL
AVRDUDE_OPTIONS = -p $(MCU) -c wiring -P /dev/ttyUSB0 -b 115200 -D

avr_CFLAGS = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -DAVR
avr_LDFLAGS = -mmcu=$(MCU) -Wl,-u,vfprintf -lprintf_flt -lm -Wl,-u,vfscanf -lscanf_flt -lm

LDFLAGS = -g -Wl,--gc-sections $(avr_LDFLAGS)
EXTERN_OBJECTS = ../../build_avr/*.o ../../../io/build_avr/*.o

EXTERN_INCLUDES = ../../src ../../../io/src
CPPFLAGS = -DIEEE754 -DNDEBUG -DAVR
CFLAGS = -c -std=c11 -Wall -Wpedantic -Wextra $(addprefix -I, $(EXTERN_INCLUDES)) $(avr_CFLAGS)
CXXFLAGS = -c -std=c++11 -Wall -Wextra -g $(addprefix -I, $(EXTERN_INCLUDES))

DEPS = ahrs io

BUILDDIR = build
SRCDIR = src

SOURCES = $(wildcard $(SRCDIR)/*.c $(SRCDIR)/*.cpp)
OBJECTS = $(addprefix $(BUILDDIR)/, $(addsuffix .o, $(notdir $(basename $(SOURCES)))))

TARGET = avr_relay

.PHONY: all
all: $(BUILDDIR) $(TARGET).ihex

$(BUILDDIR):
	mkdir -p $(BUILDDIR)


$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $< -o $@ $(CFLAGS) $(CPPFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $< -o $@ $(CXXFLAGS) $(CPPFLAGS)


$(TARGET).elf: $(OBJECTS) $(DEPS)
	$(CC) $(OBJECTS) $(EXTERN_OBJECTS) -o $@ $(LDFLAGS)

$(TARGET).ihex: $(TARGET).elf
	@echo "---------- Creating hex file: $@"
	$(OBJCOPY) -O ihex -R .eeprom $< $@

$(TARGET).eeprom: $(TARGET).elf
	$(OBJCOPY) -j .eeprom --change-sectionlma .eeprom=0 -O ihex $< $@

.PHONY: flash
flash: $(TARGET).ihex
	$(AVRDUDE) $(AVRDUDE_OPTIONS) -Uflash:w:$<:i

.PHONY: $(DEPS)
ahrs:
	make -C ../.. PLATFORM=avr
io:
	make -C ../../../io

.PHONY: clean
clean:
	rm -f $(BUILDDIR)/*
	rm -f $(TARGET).elf
	rm -f $(TARGET).ihex
	rm -fd $(BUILDDIR)
