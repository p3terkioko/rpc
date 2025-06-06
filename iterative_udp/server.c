// iterative_udp/server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> // For inet_ntop (optional for logging)

#include "rpc_protocol.h" // Headers from root via -I../
#include "calculator_ops.h" // Headers from root via -I../

#define PORT 9005 // Changed port
#define BUF_SIZE RPC_BUFFER_SIZE

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char request_buf[BUF_SIZE];
    char response_buf[BUF_SIZE];
    RpcRequest req;
    RpcResponse resp;
    CalcResult calc_res;

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

    printf("Iterative UDP RPC Server listening on port %d...\n", PORT);

    while (1) {
        memset(request_buf, 0, BUF_SIZE);
        memset(response_buf, 0, BUF_SIZE);
        memset(&client_addr, 0, sizeof(client_addr)); // Clear client_addr before recvfrom
        client_addr_len = sizeof(client_addr); // Reset client_addr_len

        ssize_t bytes_received = recvfrom(sockfd, request_buf, BUF_SIZE - 1, 0,
                                          (struct sockaddr *)&client_addr, &client_addr_len);
        if (bytes_received < 0) {
            perror("recvfrom error");
            continue;
        }
        request_buf[bytes_received] = '\0';

        char client_ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip_str, INET_ADDRSTRLEN);
        printf("Received %zd bytes from %s:%d. Request: %s\n", bytes_received, client_ip_str, ntohs(client_addr.sin_port), request_buf);


        strcpy(resp.server_type, "iterative_udp");

        if (unmarshal_request(request_buf, &req) != 0) {
            fprintf(stderr, "Failed to unmarshal request from %s:%d: %s\n", client_ip_str, ntohs(client_addr.sin_port), request_buf);
            strcpy(resp.error, "Server error: Bad request format");
            resp.result = 0;
        } else {
            printf("Op %d, op1 %.2f, op2 %.2f from %s:%d\n", req.operation, req.op1, req.op2, client_ip_str, ntohs(client_addr.sin_port));
            switch (req.operation) {
                case OP_ADD:      calc_res = add(req.op1, req.op2); break;
                case OP_SUBTRACT: calc_res = subtract(req.op1, req.op2); break;
                case OP_MULTIPLY: calc_res = multiply(req.op1, req.op2); break;
                case OP_DIVIDE:   calc_res = divide(req.op1, req.op2); break;
                default:
                    snprintf(calc_res.error, sizeof(calc_res.error), "Invalid or unsupported operation: %d", req.operation);
                    calc_res.value = 0;
                    break;
            }
            resp.result = calc_res.value;
            strcpy(resp.error, calc_res.error);
        }

        if (marshal_response(&resp, response_buf, BUF_SIZE) != 0) {
            fprintf(stderr, "Failed to marshal response for %s:%d.\n", client_ip_str, ntohs(client_addr.sin_port));
            continue;
        }

        ssize_t bytes_sent = sendto(sockfd, response_buf, strlen(response_buf), 0,
                                    (struct sockaddr *)&client_addr, client_addr_len);
        if (bytes_sent < 0) {
            perror("sendto error");
        } else {
            printf("Sent response: %s to %s:%d\n", response_buf, client_ip_str, ntohs(client_addr.sin_port));
        }
    }

    close(sockfd);
    return 0;
}
