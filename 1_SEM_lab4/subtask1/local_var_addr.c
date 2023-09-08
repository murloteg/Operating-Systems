#include <stdio.h>
#include <stdlib.h>

char* get_local_var_address(void) {
    char char_var = 'A';
    return &char_var;
}

int main(int argc, char** argv) {
    char* local_var_address = get_local_var_address();
    fprintf(stdout, "Local var address: %p\n", local_var_address);
    return EXIT_SUCCESS;
}
