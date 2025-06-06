# Top-level Makefile
CC = gcc
CFLAGS = -Wall -g -Irpc_core # Added -Irpc_core
LDFLAGS =

# Object file names (to be created in the root directory)
COMMON_RPC_OBJS = calculator_ops.o rpc_protocol.o
CLIENT_STUB_OBJS = client_stubs.o

RPC_CLIENT_SRC = rpc_client.c # Source is in root
RPC_CLIENT_OBJ = $(RPC_CLIENT_SRC:.c=.o) # rpc_client.o
RPC_CLIENT_EXE = rpc_client

SERVER_DIRS =     iterative_tcp     concurrent_tcp_threads     concurrent_tcp_processes     concurrent_tcp_async     iterative_udp     concurrent_udp_threads     concurrent_udp_processes     concurrent_udp_async

all: $(RPC_CLIENT_EXE) servers

# Explicit rules for common RPC objects to ensure output in root
calculator_ops.o: rpc_core/calculator_ops.c rpc_core/calculator_ops.h
	$(CC) $(CFLAGS) -c rpc_core/calculator_ops.c -o calculator_ops.o

rpc_protocol.o: rpc_core/rpc_protocol.c rpc_core/rpc_protocol.h
	$(CC) $(CFLAGS) -c rpc_core/rpc_protocol.c -o rpc_protocol.o

# Explicit rule for client stub object to ensure output in root
client_stubs.o: rpc_core/client_stubs.c rpc_core/client_stubs.h rpc_core/rpc_protocol.h
	$(CC) $(CFLAGS) -c rpc_core/client_stubs.c -o client_stubs.o

# Rule to build rpc_client.o (source is in root, includes files from rpc_core/)
rpc_client.o: rpc_client.c rpc_core/client_stubs.h # rpc_client.c includes rpc_core/client_stubs.h
	$(CC) $(CFLAGS) -c rpc_client.c -o rpc_client.o

# Rule to build the RPC client executable
$(RPC_CLIENT_EXE): $(RPC_CLIENT_OBJ) $(CLIENT_STUB_OBJS) $(COMMON_RPC_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Target to build all servers
servers: $(COMMON_RPC_OBJS) # Ensure common objects are built before server sub-makes
	@for dir in $(SERVER_DIRS); do 	    echo "Building server in $$dir..."; 	    $(MAKE) -C $$dir || exit 1; 	done

clean:
	rm -f $(RPC_CLIENT_EXE) $(RPC_CLIENT_OBJ) $(CLIENT_STUB_OBJS) $(COMMON_RPC_OBJS)
	@for dir in $(SERVER_DIRS); do 	    echo "Cleaning in $$dir..."; 	    $(MAKE) -C $$dir clean; 	done
	@echo "Top-level clean complete."

.PHONY: all servers clean $(SERVER_DIRS)
