#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <time.h>
#include "helper1.h"

#define BUF_SIZE 1024

int main(int argc, char *argv[])
{
    // Read from stdin
    uint8_t buffer[BUF_SIZE];
    size_t nbytes;

    nbytes = sizeof(buffer);
    read(STDIN_FILENO, buffer, nbytes);

    // Extract useful information
    int qr = get_qr(buffer);
    char *qname = get_qname(buffer);
    int qtype = get_qtype(buffer);
    char *ipv6_addr = get_ipv6_addr(buffer);

    write_log(qr, qname, qtype, ipv6_addr);
}
