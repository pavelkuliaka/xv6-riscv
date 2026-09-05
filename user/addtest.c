#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Usage: addtest <num_1> <num_2>\n");
        exit(1);
    }

    int a = atoi(argv[1]);
    int b = atoi(argv[2]);

    int result = add(a, b);

    printf("Result: %d\n", result);

    exit(1);
}
