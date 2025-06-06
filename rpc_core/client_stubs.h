#ifndef CLIENT_STUBS_H
#define CLIENT_STUBS_H

#include "rpc_protocol.h" // For RpcRequest, RpcResponse
#include <netinet/in.h> // For IPPROTO_TCP, IPPROTO_UDP (though actual value might be from elsewhere)

// Define a structure to hold the outcome of an RPC call, including server type
typedef struct {
    double result;
    char error[256];
    char server_type_handled[128];
    int call_success; // 1 for success, 0 for failure (network error, marshalling error, etc.)
} RpcCallResult;

// If IPPROTO_TCP and IPPROTO_UDP are not directly available,
// we might need to define our own constants or include a more specific header.
// For now, assume they are available or will be resolved during compilation.
// Typically, these are standard.

RpcCallResult rpc_add(double a, double b, const char* server_ip, int server_port, int protocol);
RpcCallResult rpc_subtract(double a, double b, const char* server_ip, int server_port, int protocol);
RpcCallResult rpc_multiply(double a, double b, const char* server_ip, int server_port, int protocol);
RpcCallResult rpc_divide(double a, double b, const char* server_ip, int server_port, int protocol);

#endif // CLIENT_STUBS_H
