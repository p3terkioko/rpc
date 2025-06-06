CC = gcc
CFLAGS = -Wall -g -I../  # Add include path for headers in root
LDFLAGS =

# Path to common object files (expected to be in root directory)
COMMON_OBJS = ../rpc_protocol.o ../calculator_ops.o

TARGET_SERVER = server

all: $(TARGET_SERVER)

$(TARGET_SERVER): server.c $(COMMON_OBJS) ../rpc_protocol.h ../calculator_ops.h
	$(CC) $(CFLAGS) -o $(TARGET_SERVER) server.c $(COMMON_OBJS) $(LDFLAGS)

clean:
	rm -f $(TARGET_SERVER)
	# Common .o files are not cleaned by this makefile

.PHONY: all clean
