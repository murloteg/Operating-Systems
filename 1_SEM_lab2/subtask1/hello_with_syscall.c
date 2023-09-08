#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>

enum FileDescriptorsConsts {
    STDOUT = 1
};

enum ErrorCodes {
    SOMETHING_WENT_WRONG = -1
};

int call_write() {
    return syscall(SYS_write, STDOUT, "Hello, World!\n", 14);
}

int main() {
    int status = call_write();
    if (status == SOMETHING_WENT_WRONG) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
