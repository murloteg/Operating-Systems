#include <unistd.h>

enum Consts {
    STDIN = 1
};

int main() {
    int status = write(STDIN, "Hello, World!\n", 14);
    if (status == -1) {
        return -1;
    }
    return 0;
}
