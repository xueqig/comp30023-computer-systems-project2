#include <stdio.h>
#include <unistd.h>
#include <string.h>
int main(int argc, char *argv[])
{
    // printf("%s\n", argv[1]);

    char buf[100];
    size_t nbytes;
    ssize_t bytes_read;

    nbytes = sizeof(buf);
    bytes_read = read(STDIN_FILENO, buf, nbytes);

    printf("buf: %c\n", buf[2]);
    printf("bytes_read: %zd\n", bytes_read);
}
