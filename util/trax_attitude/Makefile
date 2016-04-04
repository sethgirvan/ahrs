CC = gcc
CXX = g++

LDFLAGS = -g -lm -lpthread
EXTERN_OBJECTS = ../../build_pc/*.o ../../../io/build_pc/*.o

EXTERN_INCLUDES = ../../src ../../../io/src
CPPFLAGS =
CFLAGS = -c -std=c11 -Wall -Wpedantic -Wextra $(addprefix -I, $(EXTERN_INCLUDES)) -g

DEPS = ahrs io

BUILDDIR = build
SRCDIR = src

SOURCES = $(wildcard $(SRCDIR)/*.c $(SRCDIR)/*.cpp)
OBJECTS = $(addprefix $(BUILDDIR)/, $(addsuffix .o, $(notdir $(basename $(SOURCES)))))

TARGET = trax_attitude

.PHONY: all
all: $(BUILDDIR) $(TARGET)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(TARGET): $(OBJECTS) $(DEPS)
	$(CC) $(OBJECTS) $(EXTERN_OBJECTS) -o $@ $(LDFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $< -o $@ $(CFLAGS) $(CPPFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $< -o $@ $(CFLAGS) $(CPPFLAGS)

.PHONY: $(DEPS)
ahrs:
	make -C ../.. PLATFORM=pc
io:
	make -C ../../../io

.PHONY: clean
clean:
	rm -f $(BUILDDIR)/*
	rm -f $(TARGET)
	rm -fd $(BUILDDIR)
