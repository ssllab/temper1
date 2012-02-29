# Makefile for temper1

CC=gcc
CFLAGS=-c -Wall
LDFLAGS=-lusb
SOURCES=temper1.c usbhelper.c sysfshelper.c strreplace.c
DEPS=usbhelper.h sysfshelper.h strreplace.h
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=temper1

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS) 

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

