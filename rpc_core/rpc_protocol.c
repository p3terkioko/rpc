#include "rpc_protocol.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h> // For atof

// Helper to convert OperationType to string
const char* operation_to_string(OperationType op) {
    switch (op) {
        case OP_ADD: return "ADD";
        case OP_SUBTRACT: return "SUB";
        case OP_MULTIPLY: return "MUL";
        case OP_DIVIDE: return "DIV";
        case OP_EXIT: return "EXT"; // Corrected from EXI to EXT for consistency
        default: return "UNK"; // Unknown
    }
}

// Helper to convert string to OperationType
OperationType string_to_operation(const char* str) {
    if (strcmp(str, "ADD") == 0) return OP_ADD;
    if (strcmp(str, "SUB") == 0) return OP_SUBTRACT;
    if (strcmp(str, "MUL") == 0) return OP_MULTIPLY;
    if (strcmp(str, "DIV") == 0) return OP_DIVIDE;
    if (strcmp(str, "EXT") == 0) return OP_EXIT;
    return -1; // Invalid operation
}

// Marshal RpcRequest to buffer
// Format: OP:<OP_STR>;OP1:<VAL1>;OP2:<VAL2>;
int marshal_request(const RpcRequest* req, char* buffer, size_t buffer_size) {
    int written = snprintf(buffer, buffer_size, "OP:%s;OP1:%.10g;OP2:%.10g;",
                           operation_to_string(req->operation), req->op1, req->op2);
    if (written < 0 || (size_t)written >= buffer_size) {
        return -1; // Error or buffer too small
    }
    return 0; // Success
}

// Unmarshal buffer to RpcRequest
int unmarshal_request(const char* buffer, RpcRequest* req) {
    char op_str[10];
    // Using sscanf to parse. For robustness, more complex parsing might be needed.
    // Example: OP:ADD;OP1:10.5;OP2:5.2;
    if (sscanf(buffer, "OP:%3s;OP1:%lf;OP2:%lf;", op_str, &req->op1, &req->op2) == 3) {
        req->operation = string_to_operation(op_str);
        if (req->operation == (OperationType)-1) { // Check if string_to_operation returned an error
            return -1; // Invalid operation string
        }
        return 0; // Success
    }
    return -1; // Parsing failed
}

// Marshal RpcResponse to buffer
// Format: RES:<VAL>;ERR:<ERROR_STR>;STYPE:<SERVER_TYPE_STR>;
int marshal_response(const RpcResponse* res, char* buffer, size_t buffer_size) {
    // Replace NULL or empty error strings with a placeholder for consistent parsing
    const char* err_str = (res->error[0] == '\0') ? "NULL" : res->error;
    int written = snprintf(buffer, buffer_size, "RES:%.10g;ERR:%s;STYPE:%s;",
                           res->result, err_str, res->server_type);
    if (written < 0 || (size_t)written >= buffer_size) {
        return -1; // Error or buffer too small
    }
    return 0; // Success
}

// Unmarshal buffer to RpcResponse
int unmarshal_response(const char* buffer, RpcResponse* res) {
    // Example: RES:15.7;ERR:NULL;STYPE:concurrent_tcp_threads;
    // Need to be careful with string field that might contain spaces.
    // Using a format that reads up to ';' for string fields.
    // %[A-Za-z0-9_ ] matches alphanumeric, underscore, and space.
    // %[^;] matches everything until a semicolon.
    if (sscanf(buffer, "RES:%lf;ERR:%255[^;];STYPE:%127[^;];",
               &res->result, res->error, res->server_type) == 3) {
        if (strcmp(res->error, "NULL") == 0) {
            res->error[0] = '\0'; // Convert "NULL" placeholder back to empty string
        }
        return 0; // Success
    }
    return -1; // Parsing failed
}
