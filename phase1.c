#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <time.h>
#include "helper1.h"

int main(int argc, char *argv[])
{
    // Read from stdin
    uint8_t buffer[200];
    size_t nbytes;

    nbytes = sizeof(buffer);
    read(STDIN_FILENO, buffer, nbytes);

    // Extract useful information
    int qr = get_qr(buffer);
    printf("qr: %d\n", qr);

    char *qname = get_qname(buffer);
    printf("qname: %s\n", qname);

    int qtype = get_qtype(buffer);
    printf("qtype: %d\n", qtype);

    char *ipv6_addr = get_ipv6_addr(buffer);
    printf("ipv6_addr: %s\n", ipv6_addr);

    write_log(qr, qname, qtype, ipv6_addr);
}
