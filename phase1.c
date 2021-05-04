#include <stdio.h>
#include <unistd.h>
int main(int argc, char *argv[])
{
    // printf("%s\n", argv[1]);

    char buf[100];
    size_t nbytes;
    ssize_t bytes_read;
    nbytes = sizeof(buf);
    bytes_read = read(STDIN_FILENO, buf, nbytes);

    printf("buf[0]: %c\n", buf[0]);
    printf("buf: %s\n", buf);
    printf("nbytes: %zu\n", nbytes);
}
