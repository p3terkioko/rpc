// Content for rpc_client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h> // For IPPROTO_TCP, IPPROTO_UDP

#include "rpc_core/client_stubs.h" // Contains RpcCallResult and stub functions

// Define server endpoint configurations
typedef struct {
    const char* name;
    const char* ip;
    int port;
    int protocol; // IPPROTO_TCP or IPPROTO_UDP
} ServerEndpoint;

// List of known servers
ServerEndpoint known_servers[] = {
    {"Iterative TCP Server", "127.0.0.1", 9001, IPPROTO_TCP},
    {"Concurrent TCP Threads Server", "127.0.0.1", 9002, IPPROTO_TCP},
    {"Concurrent TCP Processes Server", "127.0.0.1", 9003, IPPROTO_TCP},
    {"Concurrent TCP Async Server", "127.0.0.1", 9004, IPPROTO_TCP},
    {"Iterative UDP Server", "127.0.0.1", 9005, IPPROTO_UDP},
    {"Concurrent UDP Threads Server", "127.0.0.1", 9006, IPPROTO_UDP},
    {"Concurrent UDP Processes Server", "127.0.0.1", 9007, IPPROTO_UDP},
    {"Concurrent UDP Async Server", "127.0.0.1", 9008, IPPROTO_UDP}
};
int num_known_servers = sizeof(known_servers) / sizeof(known_servers[0]);

// Static variable to keep track of the next server index for round-robin
static int next_server_index = 0;

int main() {
    int choice;
    double a, b;
    RpcCallResult rpc_res;

    while (1) {
        printf("\n===== RPC CALCULATOR CLIENT (Round-Robin) =====\n");
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

        int operation_handled = 0;

        for (int j = 0; j < num_known_servers; ++j) {
            int current_server_idx_to_try = (next_server_index + j) % num_known_servers;
            ServerEndpoint current_server = known_servers[current_server_idx_to_try];

            printf("\nAttempting operation with %s (%s:%d)...\n",
                   current_server.name, current_server.ip, current_server.port);

            switch (choice) {
                case 1:
                    rpc_res = rpc_add(a, b, current_server.ip, current_server.port, current_server.protocol);
                    break;
                case 2:
                    rpc_res = rpc_subtract(a, b, current_server.ip, current_server.port, current_server.protocol);
                    break;
                case 3:
                    rpc_res = rpc_multiply(a, b, current_server.ip, current_server.port, current_server.protocol);
                    break;
                case 4:
                    rpc_res = rpc_divide(a, b, current_server.ip, current_server.port, current_server.protocol);
                    break;
            }

            if (rpc_res.call_success) {
                if (rpc_res.error[0] == '\0') {
                    printf("SUCCESS! Result from %s: %.2lf\n", rpc_res.server_type_handled, rpc_res.result);
                } else {
                    printf("Server %s reported an error: %s\n", rpc_res.server_type_handled, rpc_res.error);
                }
                operation_handled = 1;
                next_server_index = (current_server_idx_to_try + 1) % num_known_servers;
                break;
            } else {
                printf("Failed to connect or get response from %s: %s\n", current_server.name, rpc_res.error);
            }
        }

        if (!operation_handled) {
            printf("Operation failed. All known servers tried or none could complete the request according to round-robin attempt.\n");
            next_server_index = (next_server_index + 1) % num_known_servers;
        }
    }

    return 0;
}
