#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum statuses {
    OK = 0,
    MEMORY_ALLOCATION_ERROR = -1
};

enum utils {
    BUFFER_SIZE = 128
};

enum statuses execute_program() {
    char* buffer = (char*) calloc(BUFFER_SIZE, sizeof(char));
    if (buffer == NULL) {
        perror("Error during calloc");
        return MEMORY_ALLOCATION_ERROR;
    }

    char* message = "Hello, world!\n";
    buffer = strcpy(buffer, message);

    fprintf(stdout, "%s", buffer);
    free(buffer);
    fprintf(stdout, "%s", buffer);

    char* another_buffer = (char*) calloc(BUFFER_SIZE, sizeof(char));
    if (another_buffer == NULL) {
        perror("Error during calloc");
        return MEMORY_ALLOCATION_ERROR;
    }

    another_buffer = strcpy(another_buffer, message);
    fprintf(stdout, "%s", another_buffer);

    size_t buffer_length = strlen(another_buffer);
    another_buffer += buffer_length / 2;
    free(another_buffer);
    fprintf(stdout, "%s", another_buffer);
    return OK; /* but it's absolutely not ok... */
}

int main(int argc, char** argv) {
    enum statuses executing_status = execute_program();
    if (executing_status != OK) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
