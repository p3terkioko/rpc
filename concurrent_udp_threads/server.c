// concurrent_udp_threads/server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> // For inet_ntop (optional for logging)
#include <pthread.h>

#include "rpc_protocol.h" // Headers from root via -I../
#include "calculator_ops.h" // Headers from root via -I../

#define PORT 9006 // Changed port
#define BUF_SIZE RPC_BUFFER_SIZE
#define MAX_REQUEST_DATA_SIZE BUF_SIZE

typedef struct {
    int server_sockfd;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
    char request_data[MAX_REQUEST_DATA_SIZE];
    ssize_t data_len;
} ThreadData;

void *handle_request_thread(void *arg) {
    ThreadData *td = (ThreadData *)arg;
    char response_buf[BUF_SIZE];
    RpcRequest req;
    RpcResponse resp;
    CalcResult calc_res;

    char client_ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &td->client_addr.sin_addr, client_ip_str, INET_ADDRSTRLEN);
    // printf("Thread %lu: Handling request from %s:%d. Data len: %zd\n", pthread_self(), client_ip_str, ntohs(td->client_addr.sin_port), td->data_len);

    // Copy request data to a local buffer and null-terminate it for sscanf-based unmarshalling
    char current_request_buffer[MAX_REQUEST_DATA_SIZE + 1];
    memcpy(current_request_buffer, td->request_data, td->data_len);
    current_request_buffer[td->data_len] = '\0';
    // printf("Thread %lu: Request buffer: \"%s\"\n", pthread_self(), current_request_buffer);


    strcpy(resp.server_type, "concurrent_udp_threads");

    if (unmarshal_request(current_request_buffer, &req) != 0) {
        fprintf(stderr, "Thread %lu: Failed to unmarshal request from %s:%d: %s\n", pthread_self(), client_ip_str, ntohs(td->client_addr.sin_port), current_request_buffer);
        strcpy(resp.error, "Server error: Bad request format");
        resp.result = 0;
    } else {
        // printf("Thread %lu: Op %d, op1 %.2f, op2 %.2f from %s:%d\n", pthread_self(), req.operation, req.op1, req.op2, client_ip_str, ntohs(td->client_addr.sin_port));
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

    memset(response_buf, 0, BUF_SIZE);
    if (marshal_response(&resp, response_buf, BUF_SIZE) != 0) {
        fprintf(stderr, "Thread %lu: Failed to marshal response for %s:%d.\n", pthread_self(), client_ip_str, ntohs(td->client_addr.sin_port));
    } else {
        ssize_t bytes_sent = sendto(td->server_sockfd, response_buf, strlen(response_buf), 0,
                                    (struct sockaddr *)&td->client_addr, td->client_addr_len);
        if (bytes_sent < 0) {
            perror("sendto error in thread");
        } else {
            // printf("Thread %lu: Sent response: %s to %s:%d\n", pthread_self(), response_buf, client_ip_str, ntohs(td->client_addr.sin_port));
        }
    }

    free(td);
    pthread_exit(NULL);
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt SO_REUSEADDR failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Concurrent UDP Threads RPC Server listening on port %d...\n", PORT);

    while (1) {
        ThreadData *td = malloc(sizeof(ThreadData));
        if (!td) {
            perror("Failed to allocate memory for ThreadData");
            continue;
        }

        td->server_sockfd = sockfd;
        td->client_addr_len = sizeof(td->client_addr);
        memset(&td->client_addr, 0, sizeof(td->client_addr)); // Clear client_addr

        // Receive into td->request_data, leaving space for null terminator if needed by sscanf in unmarshal
        // MAX_REQUEST_DATA_SIZE is BUF_SIZE, ensure unmarshal_request is robust or data is null-terminated
        ssize_t bytes_received = recvfrom(sockfd, td->request_data, MAX_REQUEST_DATA_SIZE, 0,
                                          (struct sockaddr *)&td->client_addr, &td->client_addr_len);

        if (bytes_received < 0) {
            perror("recvfrom error in main loop");
            free(td);
            continue;
        }
        td->data_len = bytes_received;
        // Null termination is handled in the thread by copying to a local buffer of size MAX_REQUEST_DATA_SIZE + 1

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_request_thread, td) != 0) {
            perror("Failed to create thread");
            free(td);
        } else {
            pthread_detach(tid);
        }
    }

    close(sockfd);
    return 0;
}
