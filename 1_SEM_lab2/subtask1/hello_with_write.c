#include <stdlib.h>
#include <unistd.h>

enum FileDescriptorsConsts {
    STDOUT = 1
};

enum ErrorCodes {
    SOMETHING_WENT_WRONG = -1
};

int main() {
    ssize_t status = write(STDOUT, "Hello, World!\n", 14);
    if (status == SOMETHING_WENT_WRONG) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
