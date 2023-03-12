#include <unistd.h>
#include <sys/syscall.h>

enum Consts {
    STDIN = 1
};

int call_write() {
    return syscall(SYS_write, STDIN, "Hello, World!\n", 14);
}

int main() {
    int status = call_write();
    if (status == -1) {
        return -1;
    }
    return 0;
}
