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

    nbytes = sizeof(buf);
    read(STDIN_FILENO, buf, nbytes);

    // extract request or response
    int qr;
    if ((int)(buf[4] >> 4 & 0x0F) == 0)
    {
        qr = 0;
    }
    else
    {
        qr = 1;
    }
    printf("qr: %d\n", qr);

    // extract domain name
    char *qname;
    int msg_idx = 14;
    int qname_idx = 0;

    int tot_len = 0;
    qname = (char *)malloc(tot_len);

    while ((int)buf[msg_idx] != 0)
    {
        int sec_len = (int)buf[msg_idx];
        tot_len += (sec_len + 1);
        qname = (char *)realloc(qname, tot_len);
        int i;
        for (i = 0; i < sec_len; i++)
        {
            qname[qname_idx++] = buf[msg_idx + i + 1];
        }
        qname[qname_idx++] = '.';
        msg_idx += (sec_len + 1);
    }
    qname[qname_idx - 1] = '\0';
    printf("qname: %s\n", qname);

    // extract qtype
    msg_idx += 2;
    int qtype = buf[msg_idx];
    printf("qtype: %d\n", qtype);

    // extract ipv6 address part
    msg_idx += 15;
    char addr[16];
    int i;
    for (i = 0; i < 16; i++)
    {
        addr[i] = buf[msg_idx++];
    }

    char ipv6_addr[40];
    inet_ntop(AF_INET6, addr, ipv6_addr, sizeof(ipv6_addr));
    printf("ipv6_addr: %s\n", ipv6_addr);

    // get current time
    time_t t;
    struct tm *tmp;
    char cur_time[50];

    time(&t);
    tmp = localtime(&t);
    strftime(cur_time, sizeof(cur_time), "%FT%T%z", tmp);

    printf("cur_time : %s\n", cur_time);

    // write to log file
    FILE *log_file;
    log_file = fopen("dns_svr.log", "a");

    if (log_file == NULL)
    {
        printf("Error!");
        exit(1);
    }

    if (qr == 0)
    {
        fprintf(log_file, "%s requested %s\n", cur_time, qname);
        fflush(log_file);
        if (qtype != 28)
        {
            fprintf(log_file, "%s unimplemented request\n", cur_time);
            fflush(log_file);
        }
    }
    else
    {
        fprintf(log_file, "%s %s is at %s\n", cur_time, qname, ipv6_addr);
        fflush(log_file);
    }
}
