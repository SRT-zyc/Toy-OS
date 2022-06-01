#ifndef PUT_H
#define PUT_H

#define UART16550A_DR (volatile unsigned char *)0x10000000

#include "stddef.h"
int printk(const char *, ...);

void puti(int num);
int puts(const char *s);
void putullHex(unsigned long long x);
#endif
