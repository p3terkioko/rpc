// concurrent_udp_async/server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <arpa/inet.h> // For inet_ntop
#include <errno.h>     // For errno

#include "rpc_protocol.h" // Headers from root via -I../
#include "calculator_ops.h" // Headers from root via -I../

#define PORT 9008 // Changed port
#define BUF_SIZE RPC_BUFFER_SIZE
#define MAX_EVENTS 10

// Simplified logging
void server_log(const char *msg) {
    printf("%s\n", msg);
}

int make_socket_non_blocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl(F_GETFL)");
        return -1;
    }
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl(F_SETFL, O_NONBLOCK)");
        return -1;
    }
    return 0;
}

int main() {
    int sockfd, epfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;
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

    if (make_socket_non_blocking(sockfd) == -1) {
        close(sockfd);
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

    epfd = epoll_create1(0);
    if (epfd == -1) {
        perror("epoll_create1 failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = sockfd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev) == -1) {
        perror("epoll_ctl ADD sockfd failed");
        close(sockfd);
        close(epfd);
        exit(EXIT_FAILURE);
    }

    printf("Async UDP RPC Server (epoll) listening on port %d...\n", PORT);

    while (1) {
        int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (n == -1) {
            if (errno == EINTR) continue;
            perror("epoll_wait failed");
            continue;
        }

        for (int i = 0; i < n; ++i) {
            if (events[i].data.fd == sockfd) {
                while(1) {
                    client_addr_len = sizeof(client_addr);
                    memset(&client_addr, 0, sizeof(client_addr));
                    memset(request_buf, 0, BUF_SIZE);

                    ssize_t bytes_received = recvfrom(sockfd, request_buf, BUF_SIZE - 1, 0,
                                                     (struct sockaddr *)&client_addr, &client_addr_len);

                    if (bytes_received < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break;
                        }
                        perror("recvfrom error");
                        break;
                    }
                    request_buf[bytes_received] = '\0';

                    char client_ip_str[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip_str, INET_ADDRSTRLEN);
                    // printf("Request from %s:%d, data: %s\n", client_ip_str, ntohs(client_addr.sin_port), request_buf);

                    strcpy(resp.server_type, "concurrent_udp_async");

                    if (unmarshal_request(request_buf, &req) != 0) {
                        fprintf(stderr, "Failed to unmarshal request from %s:%d : %s\n", client_ip_str, ntohs(client_addr.sin_port), request_buf);
                        strcpy(resp.error, "Server error: Bad request format");
                        resp.result = 0;
                    } else {
                        // printf("Op %d, op1 %.2f, op2 %.2f from %s:%d\n", req.operation, req.op1, req.op2, client_ip_str, ntohs(client_addr.sin_port));
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
                        fprintf(stderr, "Failed to marshal response for %s:%d.\n", client_ip_str, ntohs(client_addr.sin_port));
                        continue;
                    }

                    ssize_t bytes_sent = sendto(sockfd, response_buf, strlen(response_buf), 0,
                                               (struct sockaddr *)&client_addr, client_addr_len);
                    if (bytes_sent < 0) {
                        perror("sendto error");
                    } else {
                        // server_log("Sent response.");
                    }
                }
            }
        }
    }

    close(sockfd);
    close(epfd);
    return 0;
}
