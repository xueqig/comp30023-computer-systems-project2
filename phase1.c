#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <time.h>
int main(int argc, char *argv[])
{

    // read .raw data
    char buf[200];
    size_t nbytes;
    ssize_t bytes_read;

    nbytes = sizeof(buf);
    bytes_read = read(STDIN_FILENO, buf, nbytes);

    // extract request or response
    int qr = (int)(buf[4] >> 1);
    printf("qr: %d\n", qr);

    // extract domain name
    char *qname;
    int i = 14;
    int j = 0;
    int k;
    int tot_len = 0;
    qname = (char *)malloc(tot_len);

    while ((int)buf[i] != 0)
    {
        int sec_len = (int)buf[i];
        tot_len += (sec_len + 1);
        qname = (char *)realloc(qname, tot_len);

        for (k = 0; k < sec_len; k++)
        {
            qname[j++] = buf[i + k + 1];
        }
        qname[j++] = '.';
        i += (sec_len + 1);
    }
    qname[j - 1] = '\0';
    printf("qname: %s\n", qname);

    // extract type
    char type[2];
    j = 0;
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
    char addr[16];
    j = 0;
    for (i = 43; i < 43 + 16; i++)
    {
        addr[j++] = buf[i];
    }

    char ipv6_addr[40];
    inet_ntop(AF_INET6, addr, ipv6_addr, sizeof(ipv6_addr));
    printf("ipv6_addr: %s\n", ipv6_addr);

    // get time
    time_t t;
    struct tm *tmp;
    char cur_time[50];

    time(&t);
    tmp = localtime(&t);
    strftime(cur_time, sizeof(cur_time), "%FT%T%z", tmp);

    printf("Formatted time : %s\n", cur_time);

    // write to log file
    FILE *log_file;
    log_file = fopen("dns_svr.log", "a");

    if (log_file == NULL)
    {
        printf("Error!");
        exit(1);
    }

    if (strcmp(argv[1], "query") == 0)
    {
        fprintf(log_file, "%s requested 1.comp30023\n", cur_time);
        fflush(log_file);
    }

    if (type[0] == '\x00' && type[1] == '\x1c')
    {
    }
    else
    {
        fprintf(log_file, "%s unimplemented request\n", cur_time);
        fflush(log_file);
    }
}
