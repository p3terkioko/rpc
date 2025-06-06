CC = gcc
CFLAGS = -Wall -g -I../ -I../rpc_core
LDFLAGS =

# Common object files from the root directory
COMMON_OBJS = ../rpc_protocol.o ../calculator_ops.o

TARGET_SERVER = server

all: $(TARGET_SERVER)

# Link server.c with common RPC objects. Headers are dependencies for rebuild.
$(TARGET_SERVER): server.c $(COMMON_OBJS) ../rpc_core/rpc_protocol.h ../rpc_core/calculator_ops.h
	$(CC) $(CFLAGS) -o $(TARGET_SERVER) server.c $(COMMON_OBJS) $(LDFLAGS)

clean:
	rm -f $(TARGET_SERVER)

.PHONY: all clean
