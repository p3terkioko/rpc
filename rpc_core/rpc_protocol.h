#ifndef RPC_PROTOCOL_H
#define RPC_PROTOCOL_H

#include <stddef.h> // For size_t

// Define operation types
typedef enum {
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_EXIT
} OperationType;

// Structure for RPC requests
typedef struct {
    OperationType operation;
    double op1;
    double op2;
} RpcRequest;

// Structure for RPC responses
typedef struct {
    double result;
    char error[256];
    char server_type[128];
} RpcResponse;

#define RPC_BUFFER_SIZE 1024

// Function prototypes for marshalling/unmarshalling
int marshal_request(const RpcRequest* req, char* buffer, size_t buffer_size);
int unmarshal_request(const char* buffer, RpcRequest* req);
int marshal_response(const RpcResponse* res, char* buffer, size_t buffer_size);
int unmarshal_response(const char* buffer, RpcResponse* res);

#endif // RPC_PROTOCOL_H
