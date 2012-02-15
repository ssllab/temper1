# Makefile for temperature reader
CC=gcc
CFLAGS=-Wall -lusb

all: temper1

temper1: temper1.c usbhelper.c
	$(CC) $(CFLAGS) $^ -o $@ 

clean:
	rm temper1

install:
	echo TODO!
