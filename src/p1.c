#include <unistd.h>
#include <stdio.h>

int main() {
    char *args[] = { "./p2.exe", "hello", NULL };
    printf("11111\n");
    execv("./p2.exe", args);

    perror("execv failed");
    printf("222222\n");
    return 1;
}
