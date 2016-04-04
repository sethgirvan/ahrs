CC = gcc
CXX = g++

LDFLAGS = -g

CPPFLAGS =
CFLAGS = -c -std=c11 -Wall -Wpedantic -Wextra -I../../src -g

BUILDDIR = build
SRCDIR = src

SOURCES = $(wildcard $(SRCDIR)/*.c $(SRCDIR)/*.cpp)
OBJECTS = $(addsuffix .o, $(notdir $(basename $(SOURCES))))

all: $(BUILDDIR) $(OBJECTS)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

%.o: $(SRCDIR)/%.c
	$(CC) $< -o $(BUILDDIR)/$@ $(CFLAGS) $(CPPFLAGS)

%.o: $(SRCDIR)/%.cpp
	$(CXX) $< -o $(BUILDDIR)/$@ $(CFLAGS) $(CPPFLAGS)

.PHONY: clean
clean:
	rm -f $(BUILDDIR)/*
	rm -fd $(BUILDDIR)
