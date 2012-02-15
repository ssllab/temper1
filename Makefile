# Makefile for temperature reader
CC=gcc
CFLAGS=-Wall -DDEBUG

all: temper1

temper1: temper1.c usbhelper.c
	$(CC) $(CFLAGS) $^ /usr/lib/libusb-1.0.a -lpthread -lrt -o $@ 

clean:
	rm temper1

install:
	echo TODO!
