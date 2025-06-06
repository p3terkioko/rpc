# Distributed RPC Calculator System

## Overview

This project implements a distributed calculator system using Remote Procedure Calls (RPC). Clients can perform basic arithmetic operations (addition, subtraction, multiplication, division), and these operations are handled by one of several available server types running in the background. The system demonstrates location and access transparency, where the client does not need to know the specific server that will handle its request. For demonstration purposes, the client reports which type of server processed each calculation.

The core RPC logic, calculator operations, and client stubs are located in the `rpc_core/` directory. The main RPC client application is `rpc_client` in the root directory.

Eight different server implementations are provided in their respective subdirectories, showcasing various communication protocols (TCP/UDP) and concurrency models (iterative, threads, processes, async with epoll):
- `iterative_tcp/`
- `concurrent_tcp_threads/`
- `concurrent_tcp_processes/`
- `concurrent_tcp_async/` (uses epoll)
- `iterative_udp/`
- `concurrent_udp_threads/`
- `concurrent_udp_processes/`
- `concurrent_udp_async/` (uses epoll)

## Compilation

### Prerequisites
- A C compiler (e.g., GCC)
- The `make` utility
- Standard C libraries and the `pthread` library (for threaded servers)

### Building the System
To build the entire system (all servers and the RPC client):
1.  Navigate to the root directory of the repository.
2.  Run `make clean` to remove any previous build artifacts (optional, but recommended for a fresh build).
3.  Run `make all` (or simply `make`).

This command will:
- Compile common RPC components and client stubs from `rpc_core/` into object files located in the root directory.
- Compile the main client application `rpc_client.c` and link it to create the `rpc_client` executable in the root directory.
- Compile each of the 8 server types. The executable for each server (named `server`) will be located within its respective subdirectory (e.g., `iterative_tcp/server`).

## Running the System

### 1. Start the Servers
You can run any combination of the 8 server types simultaneously. Each server listens on a unique, predefined port (9001-9008).

- Open a separate terminal window for each server you wish to run.
- In each terminal, navigate to the specific server's directory.
- Execute the server. Examples:

  ```bash
  cd iterative_tcp
  ./server 
  # Server will print: Iterative TCP RPC Server listening on port 9001...
  ```
  ```bash
  # In another terminal:
  cd concurrent_udp_threads
  ./server
  # Server will print: Concurrent UDP Threads RPC Server listening on port 9006...
  ```
  (Repeat for other desired servers in their respective directories.)

The servers available and their ports are:
- `iterative_tcp`: Port 9001
- `concurrent_tcp_threads`: Port 9002
- `concurrent_tcp_processes`: Port 9003
- `concurrent_tcp_async`: Port 9004
- `iterative_udp`: Port 9005
- `concurrent_udp_threads`: Port 9006
- `concurrent_udp_processes`: Port 9007
- `concurrent_udp_async`: Port 9008

### 2. Run the RPC Client
- Open a new terminal window.
- Navigate to the root directory of the repository (where the `rpc_client` executable is located).
- Run the client application:

  ```bash
  ./rpc_client
  ```

- The client will display a menu:
  ```
  ===== RPC CALCULATOR CLIENT =====
  1. Add
  2. Subtract
  3. Multiply
  4. Divide
  5. Exit
  Choose operation: 
  ```
- Enter your choice of operation and then the two numbers.
- The client will attempt to connect to servers from its predefined list (localhost, ports 9001-9008). It will print which server configuration it is currently trying.
- Upon a successful RPC call, the client will display the result and the `server_type` string of the server that handled the request (e.g., "SUCCESS! Result from iterative_tcp: 15.00").
- If a particular server is unavailable, the client will try the next one in its list.
- If an operation results in a server-side error (e.g., division by zero), the client will display the error message received from the server.
