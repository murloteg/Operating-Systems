#include <stdio.h>
#include <stdlib.h>

typedef enum statuses {
    OK = 0,
    SOMETHING_WENT_WRONG = -1
} status_t;

status_t execute_program() {
    // TODO
    return OK;
}

int main(int argc, char** argv) {
    status_t execution_status = execute_program();
    if (execution_status != OK) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
