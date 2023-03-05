#include <stdio.h>
#include "dynamic-hello.h"

int main() {
    printf("Hello, World!\n");
    hello_from_dynamic_lib();
    return 0;
}
