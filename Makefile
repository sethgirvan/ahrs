PLATFORM = pc

ifeq ($(PLATFORM), avr)
	CC = avr-gcc
	CXX = avr-g++
else
	CC = gcc
	CXX = g++
endif

MCU = atmega2560
F_CPU = 16000000UL
CFLAGS_avr = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -DAVR

EXTERN_INCLUDES = ../io/src $(EXTERN_INCLUDES_$(PLATFORM))
CFLAGS = -c -std=c11 -Wall -Wpedantic -Wextra -I$(SRCDIR) -g $(CFLAGS_$(PLATFORM)) $(addprefix -I, $(EXTERN_INCLUDES))
CPPFLAGS = -DIEEE754 $(CPPFLAGS_$(PLATFORM))

BUILDDIR = build_$(PLATFORM)
SRCDIR = src

SOURCES = $(wildcard $(SRCDIR)/*.c $(SRCDIR)/*.cpp $(SRCDIR)/$(PLATFORM)/*.c $(SRCDIR)/$(PLATFORM)/*.cpp)
OBJECTS = $(addprefix $(BUILDDIR)/, $(addsuffix .o, $(notdir $(basename $(SOURCES)))))


.PHONY: all
all: $(BUILDDIR) $(OBJECTS)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $< -o $@ $(CFLAGS) $(CPPFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $< -o $@ $(CXXFLAGS) $(CPPFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/$(PLATFORM)/%.c
	$(CC) $< -o $@ $(CFLAGS) $(CPPFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/$(PLATFORM)/%.cpp
	$(CXX) $< -o $@ $(CXXFLAGS) $(CPPFLAGS)

.PHONY: clean
clean:
	rm -f build_avr/*
	rm -f build_pc/*
	rm -df build_avr
	rm -df build_pc
