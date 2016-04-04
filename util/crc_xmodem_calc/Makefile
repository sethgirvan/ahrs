CC = gcc
CXX = g++

LDFLAGS = -g -lm -lpthread

CPPFLAGS =
CFLAGS = -c -std=c11 -Wall -Wpedantic -Wextra -I../../src -I../components/src -g
CXXFLAGS = -c -std=c++11 -Wall -Wpendantic -Wextra -g

BUILDDIR = build
SRCDIR = src

SOURCES = $(wildcard $(SRCDIR)/*.c $(SRCDIR)/*.cpp)
OBJECTS = $(addsuffix .o, $(notdir $(basename $(SOURCES))))

TARGET = crc_xmodem_calc

all: $(BUILDDIR) $(TARGET)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

crc_xmodem_calc: $(OBJECTS) ../components/Makefile ../../Makefile
	make -C ../components
	make -C ../.. PLATFORM=pc
	$(CC) $(BUILDDIR)/$(OBJECTS) ../components/build/*.o ../../build_pc/*.o -o $@ $(LDFLAGS)

%.o: $(SRCDIR)/%.c
	$(CC) $< -o $(BUILDDIR)/$@ $(CFLAGS) $(CPPFLAGS)

%.o: $(SRCDIR)/%.cpp
	$(CXX) $< -o $(BUILDDIR)/$@ $(CFLAGS) $(CPPFLAGS)

.PHONY: clean
clean:
	rm -f $(BUILDDIR)/*
	rm -f $(TARGET)
	rm -fd $(BUILDDIR)
