// concurrent_tcp_async/server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h> // For fcntl
// #include <time.h> // Keep if log_with_timestamp is used extensively

#include "rpc_protocol.h" // Headers from root (using -I../)
#include "calculator_ops.h" // Headers from root (using -I../)

#define PORT 9004 // Changed port
#define MAX_EVENTS 64 // Increased max events slightly
#define BUF_SIZE RPC_BUFFER_SIZE

void set_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        // In a real server, might need more robust error handling than just returning
        // For example, close sockfd and signal an error condition
        close(sockfd); // Example cleanup
        // exit(EXIT_FAILURE); // Or propagate error
        return;
    }
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL O_NONBLOCK");
        close(sockfd); // Example cleanup
        // exit(EXIT_FAILURE); // Or propagate error
        return;
    }
}

// Simplified logging
void log_msg(const char *msg) {
    printf("%s\n", msg);
}


int main() {
    int server_fd, client_fd, epoll_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    struct epoll_event event, events[MAX_EVENTS];

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

    set_nonblocking(server_fd);
    // Check if server_fd was closed by set_nonblocking on error
    if (fcntl(server_fd, F_GETFD) == -1 && errno == EBADF) {
         log_msg("Server socket closed due to fcntl error in set_nonblocking. Exiting.");
         exit(EXIT_FAILURE);
    }


    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, SOMAXCONN) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    char log_buf[100];
    snprintf(log_buf, sizeof(log_buf), "Async TCP RPC Server listening on port %d...", PORT);
    log_msg(log_buf);

    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1 failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    event.data.fd = server_fd;
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
        perror("epoll_ctl ADD server_fd failed");
        close(server_fd);
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }

    while (1) {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (n == -1) {
            if (errno == EINTR) continue;
            perror("epoll_wait failed");
            continue;
        }

        for (int i = 0; i < n; i++) {
            if (events[i].data.fd == server_fd) {
                while(1) {
                    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
                    if (client_fd < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break;
                        } else {
                            perror("accept failed");
                            break;
                        }
                    }
                    set_nonblocking(client_fd);
                    if (fcntl(client_fd, F_GETFD) == -1 && errno == EBADF) { // Check if closed by set_nonblocking
                        log_msg("Client socket closed due to fcntl error in set_nonblocking. Skipping add to epoll.");
                        continue; // Don't add this fd to epoll
                    }

                    event.data.fd = client_fd;
                    event.events = EPOLLIN | EPOLLET;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
                        perror("epoll_ctl ADD client_fd failed");
                        close(client_fd);
                    } else {
                        // log_msg("New client connected.");
                    }
                }
            } else {
                int current_client_fd = events[i].data.fd;
                char request_buf[BUF_SIZE];
                char response_buf[BUF_SIZE];
                RpcRequest req;
                RpcResponse resp;
                CalcResult calc_res;

                strcpy(resp.server_type, "concurrent_tcp_async");

                ssize_t bytes_received = recv(current_client_fd, request_buf, BUF_SIZE - 1, 0);

                if (bytes_received <= 0) {
                    if (bytes_received == 0) {
                        // log_msg("Client disconnected.");
                    } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
                        // perror("recv error");
                    }
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current_client_fd, NULL); // Ignore error for DEL
                    close(current_client_fd);
                } else {
                    request_buf[bytes_received] = '\0';

                    if (unmarshal_request(request_buf, &req) != 0) {
                        fprintf(stderr, "Failed to unmarshal request: %s\n", request_buf);
                        strcpy(resp.error, "Server error: Bad request format");
                        resp.result = 0;
                    } else {
                        if (req.operation == OP_EXIT) {
                            // log_msg("Client requested exit. Closing connection.");
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current_client_fd, NULL);
                            close(current_client_fd);
                            continue;
                        }

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
                        fprintf(stderr, "Failed to marshal response.\n");
                    } else {
                        ssize_t bytes_sent = send(current_client_fd, response_buf, strlen(response_buf), 0);
                        if (bytes_sent < 0) {
                            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                                // perror("send error");
                            }
                            // For full ET, if send returns EAGAIN/EWOULDBLOCK,
                            // need to arm EPOLLOUT and send later.
                            // Current client disconnects, so this might not be hit often.
                        } else {
                             // log_msg("Sent response to client.");
                        }
                    }
                    // Client disconnects after one RPC, so we can close here.
                    // If persistent connections were expected for multiple RPCs,
                    // we would not DEL and close here unless OP_EXIT.
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current_client_fd, NULL);
                    close(current_client_fd);
                }
            }
        }
    }

    close(server_fd);
    close(epoll_fd);
    return 0;
}
