# Top-level Makefile
CC = gcc
CFLAGS = -Wall -g
LDFLAGS =

# Source files for common objects and client stubs
COMMON_RPC_SRCS = calculator_ops.c rpc_protocol.c
COMMON_RPC_OBJS = $(COMMON_RPC_SRCS:.c=.o)

CLIENT_STUB_SRCS = client_stubs.c
CLIENT_STUB_OBJS = $(CLIENT_STUB_SRCS:.c=.o)

RPC_CLIENT_SRC = rpc_client.c
RPC_CLIENT_OBJ = $(RPC_CLIENT_SRC:.c=.o)
RPC_CLIENT_EXE = rpc_client

# List of server directories
SERVER_DIRS =     iterative_tcp     concurrent_tcp_threads     concurrent_tcp_processes     concurrent_tcp_async     iterative_udp     concurrent_udp_threads     concurrent_udp_processes     concurrent_udp_async

# Default target
all: $(RPC_CLIENT_EXE) servers

# Rule to build common RPC objects
$(COMMON_RPC_OBJS): %.o: %.c rpc_protocol.h calculator_ops.h
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to build client stub objects
$(CLIENT_STUB_OBJS): %.o: %.c client_stubs.h rpc_protocol.h
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to build rpc_client object
$(RPC_CLIENT_OBJ): %.o: %.c client_stubs.h rpc_protocol.h calculator_ops.h
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to build the RPC client executable
$(RPC_CLIENT_EXE): $(RPC_CLIENT_OBJ) $(CLIENT_STUB_OBJS) $(COMMON_RPC_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Target to build all servers
servers: $(COMMON_RPC_OBJS) # Ensure common objects are built before server sub-makes
	@for dir in $(SERVER_DIRS); do 	    echo "Building server in $$dir..."; 	    $(MAKE) -C $$dir || exit 1; 	done

# Individual server targets (optional, as 'servers' target handles all)
# Example:
# iterative_tcp_server:
#	$(MAKE) -C iterative_tcp

# Clean target
clean:
	rm -f $(RPC_CLIENT_EXE) $(RPC_CLIENT_OBJ) $(CLIENT_STUB_OBJS) $(COMMON_RPC_OBJS)
	@for dir in $(SERVER_DIRS); do 	    echo "Cleaning in $$dir..."; 	    $(MAKE) -C $$dir clean; 	done
	@echo "Top-level clean complete."

.PHONY: all servers clean $(SERVER_DIRS)
