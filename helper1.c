#include "helper1.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <time.h>

#define QN_START 14
#define AAAA_ID 28
#define IPV6_LEN 40

int get_qr(uint8_t buffer[])
{
    // extract request or response
    int qr;
    if ((int)(buffer[4] >> 4 & 0x0F) == 0)
    {
        qr = 0;
    }
    else
    {
        qr = 1;
    }
    return qr;
}

char *get_qname(uint8_t buffer[])
{
    // Extract domain name
    char *qname;
    int qname_idx = 0;

    // Skip length field and header
    int msg_idx = QN_START;

    int tot_len = 0;
    qname = (char *)malloc(tot_len);

    while ((int)buffer[msg_idx] != 0)
    {
        int sec_len = (int)buffer[msg_idx];
        tot_len += (sec_len + 1);
        qname = (char *)realloc(qname, tot_len);
        int i;
        for (i = 0; i < sec_len; i++)
        {
            qname[qname_idx++] = buffer[msg_idx + i + 1];
        }
        qname[qname_idx++] = '.';
        msg_idx += (sec_len + 1);
    }
    qname[qname_idx - 1] = '\0';

    return qname;
}

int get_qtype(uint8_t buffer[])
{
    // Skip length field and header
    int msg_idx = QN_START;

    // Skip qname of the message
    while ((int)buffer[msg_idx] != 0)
    {
        msg_idx++;
    }
    // Extract qtype
    msg_idx += 2;
    int qtype = buffer[msg_idx];

    return qtype;
}

char *get_ipv6_addr(uint8_t buffer[])
{
    int msg_idx = QN_START;

    // Skip header and qname of the message
    while ((int)buffer[msg_idx] != 0)
    {
        msg_idx++;
    }

    // Check if response contains answer
    int ans_start = msg_idx += 5;
    if ((int)buffer[ans_start + 1] != 12)
    {
        return NULL;
    }

    // Skip qtype and other irrelevant information
    msg_idx += 17;

    char addr[16];
    int i;
    for (i = 0; i < 16; i++)
    {
        addr[i] = buffer[msg_idx++];
    }

    static char ipv6_addr[IPV6_LEN];
    inet_ntop(AF_INET6, addr, ipv6_addr, IPV6_LEN);

    return ipv6_addr;
}

char *get_cur_time()
{
    time_t t;
    struct tm *tmp;
    static char cur_time[50];

    time(&t);
    tmp = localtime(&t);
    strftime(cur_time, sizeof(cur_time), "%FT%T%z", tmp);

    return cur_time;
}

void write_log(int qr, char *qname, int qtype, char *ipv6_addr)
{
    char *cur_time = get_cur_time();

    FILE *log_file;
    log_file = fopen("dns_svr.log", "a");

    if (log_file == NULL)
    {
        printf("Error!");
        exit(1);
    }

    // Log request
    if (qr == 0)
    {
        fprintf(log_file, "%s requested %s\n", cur_time, qname);
        fflush(log_file);
        if (qtype != AAAA_ID)
        {
            fprintf(log_file, "%s unimplemented request\n", cur_time);
            fflush(log_file);
        }
    }
    // Log response
    else
    {
        if (ipv6_addr)
        {
            fprintf(log_file, "%s %s is at %s\n", cur_time, qname, ipv6_addr);
            fflush(log_file);
        }
    }
}
