#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>

enum statuses {
    SOMETHING_WENT_WRONG = -1
};

ssize_t call_write(int file_descriptor, const void* buffer, size_t count_of_bytes) {
    int syscall_status = 0;
    asm volatile (
            "syscall"
            : "=a" (syscall_status)
            : "a" (SYS_write), "D" (file_descriptor), "S" (buffer), "d" (count_of_bytes)
            );

    if (syscall_status < 0) {
        errno = -syscall_status;
        return SOMETHING_WENT_WRONG;
    }
    return syscall_status;
}

int main(int argc, char** argv) {
    size_t count_of_bytes = 14;
    char* message = "Hello, world!\n";

    ssize_t syscall_write_status = call_write(STDOUT_FILENO, message, count_of_bytes);
    if (syscall_write_status == SOMETHING_WENT_WRONG) {
        perror("Error during call_write()");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
