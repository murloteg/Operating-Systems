#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>

int main() {
    char* message = "Hello, world!\n";
    unsigned long countOfBytes = 14;
    int syscallStatus = 0;

    asm volatile (
            "movl %[syscallNumber], %%eax\n"    /* "zero"-operand */
            "movl %[fileDescriptor], %%edi\n"   /* "first"-operand */
            "movq %[buffer], %%rsi\n"           /* "second"-operand */
            "movq %[count], %%rdx\n"            /* "third"-operand */
            "syscall\n"
            "movl %%eax, %[syscallStatus]"
            : [syscallStatus] "=r" (syscallStatus)      /* output operands */
            : [syscallNumber] "i" (SYS_write), [fileDescriptor] "i" (STDOUT_FILENO),
                        [buffer] "r" (message), [count] "r" (countOfBytes)     /* input operands */
            : "eax"     /* clobbered-list */
    );

    if (syscallStatus < 0) {
        errno = -syscallStatus;
        perror("Error!");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
