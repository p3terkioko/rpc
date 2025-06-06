// concurrent_tcp_threads/server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h> // For logging if kept

#include "rpc_protocol.h" // Paths for root dir (using -I../../)
#include "calculator_ops.h" // Paths for root dir (using -I../../)

#define PORT 9002 // Changed port
#define BUF_SIZE RPC_BUFFER_SIZE
// #define LOG_FILE "server.log" // Logging can be simplified or kept

// int session_counter = 0; // Keep if client_id is used in logs
// pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; // Keep if session_counter is used

// If logging is kept, ensure current_timestamp and log_message are available or simplified.
// For this refactor, focus is on RPC. Extensive logging can be secondary.

typedef struct {
    int client_sock;
    // int client_id; // Optional, can be simplified
    struct sockaddr_in client_addr;
} ClientData;

void *handle_client(void *arg) {
    ClientData *data = (ClientData *)arg;
    char buffer[BUF_SIZE];
    RpcRequest req;
    RpcResponse resp;
    CalcResult calc_res;

    // Optional: logging client connection
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(data->client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    printf("Thread %lu: Connection from %s:%d\n", pthread_self(), client_ip, ntohs(data->client_addr.sin_port));

    strcpy(resp.server_type, "concurrent_tcp_threads");

    // Loop to handle multiple requests on the same connection if client supports it
    // (current rpc_client makes a new connection per call)
    while(1) {
        memset(buffer, 0, BUF_SIZE);
        ssize_t bytes_received = recv(data->client_sock, buffer, BUF_SIZE - 1, 0);

        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                printf("Thread %lu: Client %s:%d disconnected.\n", pthread_self(), client_ip, ntohs(data->client_addr.sin_port));
            } else {
                perror("Recv error");
            }
            break; // Exit loop, thread will terminate
        }
        buffer[bytes_received] = '\0';

        if (unmarshal_request(buffer, &req) != 0) {
            fprintf(stderr, "Thread %lu: Failed to unmarshal request: %s\n", pthread_self(), buffer);
            strcpy(resp.error, "Server error: Bad request format");
            resp.result = 0;
        } else {
            if (req.operation == OP_EXIT) {
                printf("Thread %lu: Client %s:%d requested exit.\n", pthread_self(), client_ip, ntohs(data->client_addr.sin_port));
                // Optionally send a confirmation, then break.
                break;
            }
            printf("Thread %lu: Op %d, op1 %.2f, op2 %.2f from %s:%d\n", pthread_self(), req.operation, req.op1, req.op2, client_ip, ntohs(data->client_addr.sin_port));

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
            fprintf(stderr, "Thread %lu: Failed to marshal response for %s:%d.\n", pthread_self(), client_ip, ntohs(data->client_addr.sin_port));
            // If marshalling fails, we can't send a useful error to client here
        } else {
            ssize_t bytes_sent = send(data->client_sock, buffer, strlen(buffer), 0);
            if (bytes_sent < 0) {
                perror("Send error");
            } else {
                printf("Thread %lu: Sent response to %s:%d: %s\n", pthread_self(), client_ip, ntohs(data->client_addr.sin_port), buffer);
            }
        }
    }

    close(data->client_sock);
    printf("Thread %lu: Client connection %s:%d closed.\n", pthread_self(), client_ip, ntohs(data->client_addr.sin_port));
    free(data);
    pthread_exit(NULL);
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    // pthread_mutex_init(&lock, NULL); // If session_counter and lock are used

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt SO_REUSEADDR failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, 10) < 0) { // Original listen backlog was 10
        perror("Listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Concurrent TCP Threads RPC Server listening on port %d...\n", PORT);
    // log_message("Server started and listening..."); // If logging kept

    while (1) {
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sock < 0) {
            perror("Accept failed"); // Log or print, then continue
            continue;
        }

        ClientData *data = malloc(sizeof(ClientData));
        if (!data) {
            perror("Failed to allocate memory for client data");
            close(client_sock);
            continue;
        }
        data->client_sock = client_sock;
        data->client_addr = client_addr; // Store client address if needed for logging

        // If using session_counter for client_id
        // pthread_mutex_lock(&lock);
        // data->client_id = ++session_counter;
        // pthread_mutex_unlock(&lock);
        // printf("Accepted connection, client ID %d\n", data->client_id);


        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, data) != 0) {
            perror("Failed to create thread");
            free(data); // Free data if thread creation failed
            close(client_sock); // Close client socket
        } else {
            pthread_detach(tid); // Detach thread so resources are freed on exit
        }
    }

    close(server_sock);
    // pthread_mutex_destroy(&lock); // If lock is used
    return 0;
}
