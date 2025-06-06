#include "client_stubs.h"
#include "rpc_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h> // For errno

#define TIMEOUT_SECONDS 5

// Generic function to perform an RPC call
static RpcCallResult perform_rpc_call(OperationType op_type, double a, double b, const char* server_ip, int server_port, int protocol) {
    RpcCallResult call_res = {0};
    call_res.call_success = 0; // Assume failure initially
    strcpy(call_res.error, "RPC call failed"); // Default error

    RpcRequest req;
    req.operation = op_type;
    req.op1 = a;
    req.op2 = b;

    char request_buffer[RPC_BUFFER_SIZE];
    if (marshal_request(&req, request_buffer, sizeof(request_buffer)) != 0) {
        strcpy(call_res.error, "Failed to marshal request");
        return call_res;
    }

    int sock = -1;
    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        strcpy(call_res.error, "Invalid server IP address");
        return call_res;
    }

    if (protocol == IPPROTO_TCP) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
    } else if (protocol == IPPROTO_UDP) {
        sock = socket(AF_INET, SOCK_DGRAM, 0);
    } else {
        strcpy(call_res.error, "Invalid protocol specified");
        return call_res;
    }

    if (sock < 0) {
        snprintf(call_res.error, sizeof(call_res.error), "Socket creation failed: %s", strerror(errno));
        return call_res;
    }

    // Set timeout for socket operations
    struct timeval tv;
    tv.tv_sec = TIMEOUT_SECONDS;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);


    char response_buffer[RPC_BUFFER_SIZE];
    ssize_t bytes_sent, bytes_received;

    if (protocol == IPPROTO_TCP) {
        if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            snprintf(call_res.error, sizeof(call_res.error), "TCP Connect failed to %s:%d: %s", server_ip, server_port, strerror(errno));
            close(sock);
            return call_res;
        }
        bytes_sent = send(sock, request_buffer, strlen(request_buffer), 0);
        if (bytes_sent < 0) {
            snprintf(call_res.error, sizeof(call_res.error), "TCP Send failed: %s", strerror(errno));
            close(sock);
            return call_res;
        }
        bytes_received = recv(sock, response_buffer, sizeof(response_buffer) - 1, 0);
        if (bytes_received <= 0) {
            if (bytes_received == 0) strcpy(call_res.error, "TCP Recv failed: Server closed connection");
            else snprintf(call_res.error, sizeof(call_res.error), "TCP Recv failed: %s", strerror(errno));
            close(sock);
            return call_res;
        }
    } else { // UDP
        bytes_sent = sendto(sock, request_buffer, strlen(request_buffer), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
        if (bytes_sent < 0) {
            snprintf(call_res.error, sizeof(call_res.error), "UDP Sendto failed: %s", strerror(errno));
            close(sock);
            return call_res;
        }
        socklen_t addr_len = sizeof(server_addr); // For recvfrom
        bytes_received = recvfrom(sock, response_buffer, sizeof(response_buffer) - 1, 0, NULL, NULL); // Not checking source for UDP response here for simplicity
        if (bytes_received <= 0) {
            snprintf(call_res.error, sizeof(call_res.error), "UDP Recvfrom failed: %s", strerror(errno));
            close(sock);
            return call_res;
        }
    }

    response_buffer[bytes_received] = '\0';

    RpcResponse resp;
    if (unmarshal_response(response_buffer, &resp) != 0) {
        strcpy(call_res.error, "Failed to unmarshal response");
        // Optionally, copy raw buffer for debugging: snprintf(call_res.error, sizeof(call_res.error), "Unmarshal failed. Raw: %s", response_buffer);
        close(sock);
        return call_res;
    }

    call_res.result = resp.result;
    strcpy(call_res.error, resp.error); // Copy error from server, if any
    strcpy(call_res.server_type_handled, resp.server_type);
    call_res.call_success = (resp.error[0] == '\0'); // Success if server reported no error

    close(sock);
    return call_res;
}

RpcCallResult rpc_add(double a, double b, const char* server_ip, int server_port, int protocol) {
    return perform_rpc_call(OP_ADD, a, b, server_ip, server_port, protocol);
}

RpcCallResult rpc_subtract(double a, double b, const char* server_ip, int server_port, int protocol) {
    return perform_rpc_call(OP_SUBTRACT, a, b, server_ip, server_port, protocol);
}

RpcCallResult rpc_multiply(double a, double b, const char* server_ip, int server_port, int protocol) {
    return perform_rpc_call(OP_MULTIPLY, a, b, server_ip, server_port, protocol);
}

RpcCallResult rpc_divide(double a, double b, const char* server_ip, int server_port, int protocol) {
    return perform_rpc_call(OP_DIVIDE, a, b, server_ip, server_port, protocol);
}
