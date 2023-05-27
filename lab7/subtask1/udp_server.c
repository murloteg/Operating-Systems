#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

enum util_consts {
    DEFAULT = 0,
    MAX_BUFFER_SIZE = 128,
    DECIMAL_SYSTEM = 10
};

typedef enum statuses {
    OK = 0,
    SOMETHING_WENT_WRONG = -1
} status_t;

int main(int argc, char** argv) {
    struct sockaddr_in server_socket_addr;
    struct sockaddr_in client_socket_addr;
    int server_socket_fd = socket(AF_INET, SOCK_DGRAM, DEFAULT);
    if (server_socket_fd == SOMETHING_WENT_WRONG) {
        perror("Error during socket()");
        return EXIT_FAILURE;
    }

    memset(&server_socket_addr, 0, sizeof(server_socket_addr));
    memset(&client_socket_addr, 0, sizeof(client_socket_addr));

    char* port_as_string = getenv("PORT");
    if (port_as_string == NULL) {
        fprintf(stderr, "Error: environment variable \"PORT\" isn't set\n");
        close(server_socket_fd);
        return EXIT_FAILURE;
    }
    short port = (short) strtol(port_as_string, NULL, DECIMAL_SYSTEM);

    server_socket_addr.sin_family = AF_INET;
    server_socket_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_socket_addr.sin_port = htons(port);

    status_t status = bind(server_socket_fd, (const struct sockaddr*) &server_socket_addr, (socklen_t) sizeof(server_socket_addr));
    if (status != OK) {
        perror("Error during bind()");
        close(server_socket_fd);
        return EXIT_FAILURE;
    }

    char buffer[MAX_BUFFER_SIZE];
    memset(buffer, '\0', MAX_BUFFER_SIZE);

    while (true) {
        socklen_t address_length = sizeof(client_socket_addr);
        ssize_t received_bytes_number = recvfrom(server_socket_fd, buffer, MAX_BUFFER_SIZE - 1, DEFAULT,
                                                 (struct sockaddr*) &client_socket_addr, &address_length);
        if (received_bytes_number < 0) {
            perror("Error during recvfrom()");
            close(server_socket_fd);
            return EXIT_FAILURE;
        }
        fprintf(stdout, "[SERVER] Received: %s\n", buffer);

        size_t buffer_length = strlen(buffer);
        ssize_t sent_bytes_number = sendto(server_socket_fd, buffer, buffer_length, DEFAULT,
                                           (struct sockaddr*) &client_socket_addr, (socklen_t) sizeof(client_socket_addr));
        if (sent_bytes_number < 0) {
            perror("Error during sendto()");
            close(server_socket_fd);
            return EXIT_FAILURE;
        }
    }

    status_t closing_status = close(server_socket_fd);
    if (closing_status != OK) {
        perror("Error during close()");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
