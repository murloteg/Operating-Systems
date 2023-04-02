#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>

enum ErrorCodes {
    SOMETHING_WENT_WRONG = -1
};

ssize_t call_write(int fileDescriptor, const void* buffer, size_t count) {
    int syscallStatus = 0;
    asm volatile (
            "movl %[syscallNumber], %%eax\n"    /* "zero"-operand */
            "movl %[fileDescriptor], %%edi\n"   /* "first"-operand */
            "movq %[buffer], %%rsi\n"           /* "second"-operand */
            "movq %[count], %%rdx\n"            /* "third"-operand */
            "syscall\n"
            "movl %%eax, %[syscallStatus]"
            : [syscallStatus] "=r" (syscallStatus)  /* output operands */
            : [syscallNumber] "i" (SYS_write), [fileDescriptor] "r" (fileDescriptor),    /* input operands */
                                        [buffer] "r" (buffer), [count] "r" (count)
            : "eax"     /* clobbered-list */
    );

    if (syscallStatus < 0) {
        errno = -syscallStatus;
        return SOMETHING_WENT_WRONG;
    }
}

int main() {
    char* message = "Hello, world!\n";
    size_t count = 14;
    ssize_t isSuccessfulCalling = call_write(STDOUT_FILENO, message, count);

    if (isSuccessfulCalling == SOMETHING_WENT_WRONG) {
        perror("Write error");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
