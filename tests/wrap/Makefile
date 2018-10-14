SRCS_DIR:=src
BUILD_DIR:=bin
CURRENT_DIR := $(shell pwd)


CC=gcc
CFLAGS=-Wall -g -m64 -Doff64_t=_off64_t -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE
LDFLAGS = -m64 
CCSOURCES=$(wildcard $(SRCS_DIR)/*.c)


all:  link


link:

	$(CC) $(CFLAGS) -c $(SRCS_DIR)/wrap-link.c -o $(BUILD_DIR)/wrap-link.o
	$(CC) $(CFLAGS) -c $(SRCS_DIR)/test.c -o $(BUILD_DIR)/test-link.o
	$(CC) $(LDFLAGS) -Wl,-wrap=fopen -Wl,-wrap=fclose -Wl,-wrap=fread -Wl,-wrap=fwrite \
					 -Wl,-wrap=fseek -Wl,-wrap=fseeko \
					 -Wl,-wrap=ftell -Wl,-wrap=ftello \
					 $(BUILD_DIR)/test-link.o $(BUILD_DIR)/wrap-link.o -o $(BUILD_DIR)/test-link-bin

clean:
	rm -rf bin/*
