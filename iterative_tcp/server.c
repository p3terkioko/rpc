// iterative_tcp/server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h> // For socket functions

// Adjust relative paths as necessary if headers are at root
#include "rpc_protocol.h"
#include "calculator_ops.h"

#define PORT 9001 // Changed port
#define BUF_SIZE RPC_BUFFER_SIZE // Use defined RPC buffer size

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    char buffer[BUF_SIZE];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt SO_REUSEADDR failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Iterative TCP RPC Server listening on port %d...\n", PORT);

    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (new_socket < 0) {
            perror("Accept failed");
            continue; // Continue to accept other connections
        }

        printf("Client connected to Iterative TCP RPC Server.\n");

        // For iterative server, handle one client fully then loop for next.
        // If client sends multiple requests on same connection, this loop handles them.
        // The current client stub creates a new connection for each RPC call.
        // So this inner loop might run only once per accept if client disconnects after one RPC.
        while (1) {
            memset(buffer, 0, BUF_SIZE);
            int bytes_received = read(new_socket, buffer, BUF_SIZE - 1);

            if (bytes_received <= 0) {
                if (bytes_received == 0) printf("Client disconnected.\n");
                else perror("Read error");
                break; // Break inner loop, close client socket, wait for new connection
            }
            buffer[bytes_received] = '\0'; // Null-terminate the received data

            RpcRequest req;
            RpcResponse resp;
            CalcResult calc_res;

            // Set server type for the response
            strcpy(resp.server_type, "iterative_tcp");

            if (unmarshal_request(buffer, &req) != 0) {
                fprintf(stderr, "Failed to unmarshal request: %s\n", buffer);
                // Send back an error response if possible
                strcpy(resp.error, "Server error: Bad request format");
                resp.result = 0; // Or some NaN/error indicator
            } else {
                // Process OP_EXIT from client if needed, though client manages connection
                if (req.operation == OP_EXIT) {
                     printf("Client requested exit. Closing connection.\n");
                     // Optionally send a confirmation back, but not strictly required by current RPC design
                     break;
                }

                printf("Received operation %d, op1=%.2f, op2=%.2f\n", req.operation, req.op1, req.op2);

                switch (req.operation) {
                    case OP_ADD:      calc_res = add(req.op1, req.op2); break;
                    case OP_SUBTRACT: calc_res = subtract(req.op1, req.op2); break;
                    case OP_MULTIPLY: calc_res = multiply(req.op1, req.op2); break;
                    case OP_DIVIDE:   calc_res = divide(req.op1, req.op2); break;
                    default:
                        snprintf(calc_res.error, sizeof(calc_res.error), "Invalid operation requested: %d", req.operation);
                        calc_res.value = 0; // Or NaN
                        break;
                }
                resp.result = calc_res.value;
                strcpy(resp.error, calc_res.error);
            }

            memset(buffer, 0, BUF_SIZE);
            if (marshal_response(&resp, buffer, BUF_SIZE) != 0) {
                fprintf(stderr, "Failed to marshal response.\n");
                // Cannot send error to client if marshalling fails
            } else {
                ssize_t bytes_sent = write(new_socket, buffer, strlen(buffer));
                if (bytes_sent < 0) {
                    perror("Write error");
                } else {
                    printf("Sent response: %s\n", buffer);
                }
            }
            // If the client disconnects after one request, the next read() will detect it.
        }
        close(new_socket);
        printf("Client connection closed.\n");
    }

    close(server_fd);
    return 0;
}
