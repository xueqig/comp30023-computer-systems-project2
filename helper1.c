#include "helper1.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

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
    // extract domain name
    char *qname;
    int msg_idx = 14;
    int qname_idx = 0;

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
    int msg_idx = 14;

    // Skip header and qname of the message
    while ((int)buffer[msg_idx] != 0)
    {
        msg_idx++;
    }
    // extract qtype
    msg_idx += 2;
    int qtype = buffer[msg_idx];

    return qtype;
}
