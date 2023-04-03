#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>

enum ErrorCodes {
    SOMETHING_WENT_WRONG = -1
};

ssize_t call_write(int fileDescriptor, const void* buffer, size_t countOfBytes) {
    int syscallStatus = 0;
    asm volatile (
            "syscall"
            : "=a" (syscallStatus)
            : "a" (SYS_write), "D" (fileDescriptor), "S" (buffer), "d" (countOfBytes)
            );

    if (syscallStatus < 0) {
        errno = -syscallStatus;
        return SOMETHING_WENT_WRONG;
    }
    return syscallStatus;
}

int main() {
    size_t countOfBytes = 14;
    char* message = "Hello, world!\n";

    ssize_t syscallWriteStatus = call_write(STDOUT_FILENO, message, countOfBytes);
    if (syscallWriteStatus == SOMETHING_WENT_WRONG) {
        perror("Error");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
