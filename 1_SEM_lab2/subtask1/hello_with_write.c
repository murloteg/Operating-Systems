#include <stdlib.h>
#include <unistd.h>

enum util_consts {
    STRING_LENGTH = 14
};

enum file_descriptors_consts {
    STDOUT = 1
};

enum statuses {
    SOMETHING_WENT_WRONG = -1
};

int main(int argc, char** argv) {
    ssize_t status = write(STDOUT, "Hello, World!\n", STRING_LENGTH);
    if (status == SOMETHING_WENT_WRONG) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
