CC := clang
CFLAGS := -std=c99 -O2 -Isrc -g -Wall

LDFLAGS := `pkg-config --cflags --static --libs allegro-static-5 allegro_image-static-5 allegro_color-static-5 allegro_main-static-5 allegro_primitives-static-5`

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
