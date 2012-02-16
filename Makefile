# Makefile for temper1

CC=gcc
CFLAGS=-c -Wall
LDFLAGS=-lusb
SOURCES=temper1.c usbhelper.c
DEPS=usbhelper.h
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=temper1

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

