#include <unistd.h>

enum Consts {
    STDIN = 1
};

int main() {
    write(STDIN, "Hello, World!\n", 14);
    return 0;
}
