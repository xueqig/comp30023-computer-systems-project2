#ifndef HELPER1_H
#define HELPER1_H

#include <stdint.h>

int get_qr(uint8_t buffer[]);
char *get_qname(uint8_t buffer[]);
int get_qtype(uint8_t buffer[]);

#endif
