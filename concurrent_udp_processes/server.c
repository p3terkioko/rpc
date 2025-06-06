// concurrent_udp_processes/server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> // For inet_ntop (optional for logging)
#include <sys/wait.h>  // For waitpid
#include <signal.h>    // For signal/sigaction
#include <errno.h>     // For errno, EINTR

#include "rpc_protocol.h" // Headers from root via -I../
#include "calculator_ops.h" // Headers from root via -I../

#define PORT 9007 // Changed port
#define BUF_SIZE RPC_BUFFER_SIZE

// Basic SIGCHLD handler to prevent zombie processes
void sigchld_handler(int sig) {
    (void)sig; // Unused parameter
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

void process_client_request(int server_sockfd, const char* request_buf, ssize_t data_len, struct sockaddr_in client_addr, socklen_t client_addr_len) {
    char response_buf[BUF_SIZE];
    RpcRequest req;
    RpcResponse resp;
    CalcResult calc_res;

    char current_request_copy[BUF_SIZE + 1];
    memcpy(current_request_copy, request_buf, data_len);
    current_request_copy[data_len] = '\0';

    char client_ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip_str, INET_ADDRSTRLEN);
    // printf("Child PID %d: Handling request from %s:%d. Data: %s\n", getpid(), client_ip_str, ntohs(client_addr.sin_port), current_request_copy);


    strcpy(resp.server_type, "concurrent_udp_processes");

    if (unmarshal_request(current_request_copy, &req) != 0) {
        fprintf(stderr, "Child PID %d: Failed to unmarshal request from %s:%d: %s\n", getpid(), client_ip_str, ntohs(client_addr.sin_port), current_request_copy);
        strcpy(resp.error, "Server error: Bad request format");
        resp.result = 0;
    } else {
        // printf("Child PID %d: Op %d, op1 %.2f, op2 %.2f from %s:%d\n", getpid(), req.operation, req.op1, req.op2, client_ip_str, ntohs(client_addr.sin_port));
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
        fprintf(stderr, "Child PID %d: Failed to marshal response for %s:%d.\n", getpid(), client_ip_str, ntohs(client_addr.sin_port));
    } else {
        ssize_t bytes_sent = sendto(server_sockfd, response_buf, strlen(response_buf), 0,
                                    (struct sockaddr *)&client_addr, client_addr_len);
        if (bytes_sent < 0) {
            perror("sendto error in child process");
        } else {
            // printf("Child PID %d: Sent response: %s to %s:%d\n", getpid(), response_buf, client_ip_str, ntohs(client_addr.sin_port));
        }
    }
    exit(EXIT_SUCCESS);
}


int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;
    char request_buf[BUF_SIZE];

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, 0) == -1) {
        perror("sigaction failed for SIGCHLD");
        exit(EXIT_FAILURE);
    }

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

    printf("Concurrent UDP Processes RPC Server listening on port %d...\n", PORT);

    while (1) {
        client_addr_len = sizeof(client_addr);
        memset(&client_addr, 0, sizeof(client_addr));
        memset(request_buf, 0, BUF_SIZE);

        // Read into request_buf, ensuring space for null termination if needed by child processing
        // The child process_client_request now creates a local copy and null terminates.
        ssize_t bytes_received = recvfrom(sockfd, request_buf, BUF_SIZE, 0,
                                          (struct sockaddr *)&client_addr, &client_addr_len);

        if (bytes_received < 0) {
            if (errno == EINTR) continue;
            perror("recvfrom error in main loop");
            continue;
        }

        pid_t pid = fork();

        if (pid < 0) {
            perror("Fork failed");
            continue;
        }

        if (pid == 0) { // Child process
            // Note: Child does not need to close sockfd as it's a datagram socket and not connection-oriented.
            // It uses the inherited sockfd to send the reply.
            process_client_request(sockfd, request_buf, bytes_received, client_addr, client_addr_len);
            // process_client_request calls exit()
        } else { // Parent process
            // Parent continues to listen for new requests
        }
    }

    close(sockfd);
    return 0;
}
