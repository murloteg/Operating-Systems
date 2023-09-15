#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>

enum util_consts {
    STRING_LENGTH = 14
};

enum file_descriptors {
    STDOUT = 1
};

enum statuses {
    SOMETHING_WENT_WRONG = -1
};

int call_write() {
    return syscall(SYS_write, STDOUT, "Hello, World!\n", STRING_LENGTH);
}

int main(int argc, char** argv) {
    int status = call_write();
    if (status == SOMETHING_WENT_WRONG) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
