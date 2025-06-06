CC = gcc
CFLAGS = -Wall -g -I../ # Include path for root headers -I../rpc_core
LDFLAGS =

# Path to common object files (expected to be in root directory)
COMMON_OBJS = ../rpc_protocol.o ../calculator_ops.o

TARGET_SERVER = server

all: $(TARGET_SERVER)

$(TARGET_SERVER): server.c $(COMMON_OBJS) ../rpc_core/rpc_protocol.h ../rpc_core/calculator_ops.h
	$(CC) $(CFLAGS) -o $(TARGET_SERVER) server.c $(COMMON_OBJS) $(LDFLAGS)

clean:
	rm -f $(TARGET_SERVER)

.PHONY: all clean
