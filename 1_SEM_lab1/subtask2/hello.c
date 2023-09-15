#include <stdio.h>
#include <stdlib.h>
#include "static-hello.h"

int main(int argc, char** argv) {
    printf("Hello, World!\n");
    hello_from_static_lib();
    return EXIT_SUCCESS;
}
