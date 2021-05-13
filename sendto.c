#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main()
{

    int bytes_sent;
    int bytes_received;
    char data_sent[256];
    char data_received[256];
    struct sockaddr_in to;
    struct sockaddr from;
    int addrlen;
    int s;

    memset(&to, 0, sizeof(to));
    to.sin_family = AF_INET;
    to.sin_addr.s_addr = inet_addr("129.5.24.1");
    to.sin_port = htons(1024);

    bytes_sent = sendto(s, data_sent, sizeof(data_sent), 0,
                        (struct sockaddr *)&to, sizeof(to));

    addrlen = sizeof(from); /* must be initialized */
    bytes_received = recvfrom(s, data_received, sizeof(data_received), 0, &from, &addrlen);

    return 0;
}