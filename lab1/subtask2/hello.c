#include <stdio.h>
#include "static-hello.h"

int main() {
    printf("Hello, World!\n");
    hello_from_static_lib();
    return 0;
}
