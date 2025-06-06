// concurrent_tcp_processes/server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h> // For waitpid or signal
#include <signal.h>   // For signal
#include <errno.h>    // For errno and EINTR

#include "rpc_protocol.h" // Will be found via CFLAGS -I../
#include "calculator_ops.h" // Will be found via CFLAGS -I../

#define PORT 9003 // Changed port
#define BUF_SIZE RPC_BUFFER_SIZE

void handle_client_connection(int client_sock, struct sockaddr_in client_addr) {
    char buffer[BUF_SIZE];
    RpcRequest req;
    RpcResponse resp;
    CalcResult calc_res;

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    printf("Child process %d: Handling client %s:%d\n", getpid(), client_ip, ntohs(client_addr.sin_port));

    strcpy(resp.server_type, "concurrent_tcp_processes");

    // Loop to handle multiple requests on the same connection
    while(1) {
        memset(buffer, 0, BUF_SIZE);
        ssize_t bytes_received = recv(client_sock, buffer, BUF_SIZE - 1, 0);

        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                printf("Child process %d: Client %s:%d disconnected.\n", getpid(), client_ip, ntohs(client_addr.sin_port));
            } else {
                perror("Recv error in child");
            }
            break;
        }
        buffer[bytes_received] = '\0';

        if (unmarshal_request(buffer, &req) != 0) {
            fprintf(stderr, "Child process %d: Failed to unmarshal request from %s:%d: %s\n", getpid(), client_ip, ntohs(client_addr.sin_port), buffer);
            strcpy(resp.error, "Server error: Bad request format");
            resp.result = 0;
        } else {
            if (req.operation == OP_EXIT) {
                printf("Child process %d: Client %s:%d requested exit.\n", getpid(), client_ip, ntohs(client_addr.sin_port));
                break;
            }
            printf("Child process %d: Op %d, op1 %.2f, op2 %.2f from %s:%d\n", getpid(), req.operation, req.op1, req.op2, client_ip, ntohs(client_addr.sin_port));

            switch (req.operation) {
                case OP_ADD:      calc_res = add(req.op1, req.op2); break;
                case OP_SUBTRACT: calc_res = subtract(req.op1, req.op2); break;
                case OP_MULTIPLY: calc_res = multiply(req.op1, req.op2); break;
                case OP_DIVIDE:   calc_res = divide(req.op1, req.op2); break;
                default:
                    snprintf(calc_res.error, sizeof(calc_res.error), "Invalid operation: %d", req.operation);
                    calc_res.value = 0;
                    break;
            }
            resp.result = calc_res.value;
            strcpy(resp.error, calc_res.error);
        }

        memset(buffer, 0, BUF_SIZE);
        if (marshal_response(&resp, buffer, BUF_SIZE) != 0) {
            fprintf(stderr, "Child process %d: Failed to marshal response for %s:%d.\n", getpid(), client_ip, ntohs(client_addr.sin_port));
        } else {
            ssize_t bytes_sent = send(client_sock, buffer, strlen(buffer), 0);
            if (bytes_sent < 0) {
                perror("Send error in child");
            } else {
                printf("Child process %d: Sent response to %s:%d: %s\n", getpid(), client_ip, ntohs(client_addr.sin_port), buffer);
            }
        }
    }

    close(client_sock);
    printf("Child process %d: Client connection %s:%d closed, exiting.\n", getpid(), client_ip, ntohs(client_addr.sin_port));
    exit(EXIT_SUCCESS); // Child process exits after handling client
}

// Basic SIGCHLD handler to prevent zombie processes
void sigchld_handler(int sig) {
    (void)sig; // Unused parameter
    // Reap all terminated children without blocking
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main() {
    int server_fd, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    pid_t pid;

    // Setup SIGCHLD handler
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa)); // Initialize sa structure
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, 0) == -1) {
        perror("sigaction failed for SIGCHLD");
        exit(EXIT_FAILURE);
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt SO_REUSEADDR failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) { // Using a common backlog value
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Concurrent TCP Processes RPC Server listening on port %d...\n", PORT);

    while (1) {
        client_sock = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_sock < 0) {
            // Check if accept was interrupted by SIGCHLD, and continue if so
            if (errno == EINTR) {
                continue;
            }
            perror("Accept failed");
            continue;
        }

        pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            close(client_sock);
            continue;
        }

        if (pid == 0) { // Child process
            close(server_fd);
            handle_client_connection(client_sock, client_addr);
            // handle_client_connection calls exit()
        } else { // Parent process
            close(client_sock);
        }
    }

    close(server_fd);
    return 0;
}
