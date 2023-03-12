#include <unistd.h>
#include <sys/syscall.h>

enum Consts {
    STDIN = 1
};

void call_my_write() {
    syscall(SYS_write, STDIN, "Hello, World!\n", 14);
}

int main() {
    call_my_write();
    return 0;
}
