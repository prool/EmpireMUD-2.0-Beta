#include <stdio.h>

int main(int argc, char **argv) {
FILE *fp;
fp=fopen("p2.log", "a");
fprintf(fp, "p2 start, arg=%s\n", argv[1]);
fclose(fp);
    printf("prog2 started, arg=%s\n", argv[1]);
    return 0;
}
