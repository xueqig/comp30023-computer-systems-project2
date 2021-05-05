#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
int main(int argc, char *argv[])
{
    // get query or response
    printf("%s\n", argv[1]);

    int i, j;

    // read .raw data
    char buf[200];
    size_t nbytes;
    ssize_t bytes_read;

    nbytes = sizeof(buf);
    bytes_read = read(STDIN_FILENO, buf, nbytes);

    // extract number
    int number = strtol(&buf[15], NULL, 16);
    printf("number: %d\n", number);

    // extract size
    int size = strtol(&buf[16], NULL, 16);
    printf("size: %d\n", size);

    // extract data
    // j = 0;
    // char data[10];
    // for (i = 17; i < 17 + 9; i++)
    // {
    //     data[j++] = buf[i];
    // }

    // extract type
    j = 0;
    char type[2];
    for (i = 27; i < 27 + 4; i++)
    {
        type[j++] = buf[i];
    }

    if (type[0] == '\x00' && type[1] == '\x1c')
    {
        printf("AAAA\n");
    }
    else
    {
        printf("Not AAAA\n");
    }

    // extract ipv6 address part
    j = 0;
    char addr[16];
    for (i = 43; i < 43 + 16; i++)
    {
        addr[j++] = buf[i];
    }

    char ipv6_addr[40];
    inet_ntop(AF_INET6, addr, ipv6_addr, sizeof(ipv6_addr));
    printf("ipv6_addr: %s\n", ipv6_addr);
}
