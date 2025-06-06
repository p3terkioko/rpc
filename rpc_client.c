#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h> // For IPPROTO_TCP, IPPROTO_UDP

#include "client_stubs.h" // Contains RpcCallResult and stub functions

// Define server endpoint configurations
typedef struct {
    const char* name;
    const char* ip;
    int port;
    int protocol; // IPPROTO_TCP or IPPROTO_UDP
} ServerEndpoint;

// List of known servers
// For simplicity, using localhost. Ports need to match server implementations.
ServerEndpoint known_servers[] = {
    {"Iterative TCP Server", "127.0.0.1", 9001, IPPROTO_TCP},
    {"Concurrent TCP Threads Server", "127.0.0.1", 9002, IPPROTO_TCP},
    {"Concurrent TCP Processes Server", "127.0.0.1", 9003, IPPROTO_TCP},
    {"Concurrent TCP Async Server", "127.0.0.1", 9004, IPPROTO_TCP}, // Assuming standard TCP interaction from client stub
    {"Iterative UDP Server", "127.0.0.1", 9005, IPPROTO_UDP},
    {"Concurrent UDP Threads Server", "127.0.0.1", 9006, IPPROTO_UDP},
    {"Concurrent UDP Processes Server", "127.0.0.1", 9007, IPPROTO_UDP},
    {"Concurrent UDP Async Server", "127.0.0.1", 9008, IPPROTO_UDP}  // Assuming standard UDP interaction from client stub
};
int num_known_servers = sizeof(known_servers) / sizeof(known_servers[0]);

int main() {
    int choice;
    double a, b;
    RpcCallResult rpc_res;

    while (1) {
        printf("\n===== RPC CALCULATOR CLIENT =====\n");
        printf("1. Add\n");
        printf("2. Subtract\n");
        printf("3. Multiply\n");
        printf("4. Divide\n");
        printf("5. Exit\n");
        printf("Choose operation: ");

        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n");
            while (getchar() != '\n'); // Clear input buffer
            continue;
        }
        while (getchar() != '\n'); // Clear trailing newline

        if (choice == 5) {
            printf("Exiting client. Goodbye!\n");
            break;
        }

        if (choice < 1 || choice > 4) {
            printf("Invalid choice. Please try again.\n");
            continue;
        }

        printf("Enter two numbers: ");
        if (scanf("%lf %lf", &a, &b) != 2) {
            printf("Invalid input for numbers. Please enter two numbers.\n");
            while (getchar() != '\n'); // Clear input buffer
            continue;
        }
        while (getchar() != '\n'); // Clear trailing newline

        int operation_performed = 0;
        for (int i = 0; i < num_known_servers; ++i) {
            printf("\nAttempting operation with %s (%s:%d)...\n",
                   known_servers[i].name, known_servers[i].ip, known_servers[i].port);

            switch (choice) {
                case 1:
                    rpc_res = rpc_add(a, b, known_servers[i].ip, known_servers[i].port, known_servers[i].protocol);
                    break;
                case 2:
                    rpc_res = rpc_subtract(a, b, known_servers[i].ip, known_servers[i].port, known_servers[i].protocol);
                    break;
                case 3:
                    rpc_res = rpc_multiply(a, b, known_servers[i].ip, known_servers[i].port, known_servers[i].protocol);
                    break;
                case 4:
                    rpc_res = rpc_divide(a, b, known_servers[i].ip, known_servers[i].port, known_servers[i].protocol);
                    break;
                default: // Should not happen due to earlier check
                    fprintf(stderr, "Internal error: Invalid choice in switch.\n");
                    continue;
            }

            if (rpc_res.call_success) {
                if (rpc_res.error[0] == '\0') { // Server processed without error
                    printf("SUCCESS! Result from %s: %.2lf\n", rpc_res.server_type_handled, rpc_res.result);
                    operation_performed = 1;
                } else { // Server returned an error (e.g., division by zero)
                    printf("Server %s reported an error: %s\n", rpc_res.server_type_handled, rpc_res.error);
                    // This is still a "successful" RPC call in that we got a response,
                    // but the operation itself failed. We can consider this as handled.
                    operation_performed = 1;
                }
                break; // Exit loop once an operation is successfully processed or server returns a specific error
            } else {
                printf("Failed to connect or get response from %s: %s\n", known_servers[i].name, rpc_res.error);
                // Try next server
            }
        }

        if (!operation_performed) {
            printf("Operation failed. All known servers tried or none could complete the request.\n");
        }
    }

    return 0;
}
