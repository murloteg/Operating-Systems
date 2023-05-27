#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

enum util_consts {
    REQUIRED_NUMBER_OF_ARGS = 2,
    DEFAULT = 0,
    MAX_BUFFER_SIZE = 128,
    DECIMAL_SYSTEM = 10
};

typedef enum statuses {
    OK = 0,
    SOMETHING_WENT_WRONG = -1
} status_t;

int main(int argc, char** argv) {
    if (argc != REQUIRED_NUMBER_OF_ARGS) {
        fprintf(stderr, "Usage: ./client \"message <...>\"\n");
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_socket_addr;
    int server_socket_fd = socket(AF_INET, SOCK_DGRAM, DEFAULT);
    if (server_socket_fd == SOMETHING_WENT_WRONG) {
        perror("Error during socket()");
        return EXIT_FAILURE;
    }

    memset(&server_socket_addr, 0, sizeof(server_socket_addr));

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

    char buffer[MAX_BUFFER_SIZE];
    memset(buffer, '\0', MAX_BUFFER_SIZE);
    strcpy(buffer, argv[1]);

    ssize_t sent_bytes_number = sendto(server_socket_fd, buffer, strlen(buffer), DEFAULT,
                                       (const struct sockaddr*) &server_socket_addr, (socklen_t) sizeof(server_socket_addr));
    if (sent_bytes_number < 0) {
        perror("Error during sendto()");
        close(server_socket_fd);
        return EXIT_FAILURE;
    }

    socklen_t address_length = sizeof(server_socket_addr);
    ssize_t received_bytes_number = recvfrom(server_socket_fd, buffer, MAX_BUFFER_SIZE - 1, DEFAULT,
                                             (struct sockaddr*) &server_socket_addr, &address_length);
    if (received_bytes_number < 0) {
        perror("Error during recvfrom()");
        close(server_socket_fd);
        return EXIT_FAILURE;
    }

    fprintf(stdout, "[CLIENT] Received: %s\n", buffer);
    status_t closing_status = close(server_socket_fd);
    if (closing_status != OK) {
        perror("Error during close()");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
