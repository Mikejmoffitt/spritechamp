CC := gcc
CFLAGS := -std=c99 -O3 -Isrc -g -Wall

LDFLAGS :=
SOURCES := $(wildcard src/*.c)
OBJECTS := $(SOURCES:.c=.o)
TARGET := spritechamp

.PHONY: all clean

all: $(TARGET)

clean:
	$(RM) $(OBJECTS) $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@ $(LDFLAGS)

.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@
