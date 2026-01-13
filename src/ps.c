#include <unistd.h>
#include <stdio.h>
#include <process.h>
#include <stddef.h>

int main() {
printf("111\n");
spawnl(1, "p2.exe", "p2.exe", NULL);
printf("222\n");

    return 1;
}

