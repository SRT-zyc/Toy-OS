#include "syscall.h"
#include "string.h"

static inline long read(int fd, void* buf, int nbuf)
{
	long ret;
	asm volatile (	"li a7, %4\n"
					"mv a0, %3\n"
					"mv a1, %2\n"
					"mv a2, %1\n"
					"ecall\n"
					"mv %0, a0\n"
					: "+r" (ret)
					: "r" (nbuf), "r" (buf), "r" (fd),
					"i" (SYS_READ)
					);
	return ret;
}

char *gets(char *buf, int max) {
    int i, cc;
    char c;

    for (i = 0; i + 1 < max;) {
        cc = read(0, &c, 1);
        if (cc == 0)
            continue;
        if (cc < 1)
            break;
        buf[i++] = c;
        if (c == '\n' || c == '\r')
            break;
    }
    buf[i - 1] = '\0';
    return buf;
}

void *memset(void *dst, int c, unsigned int n) {
    char *cdst = (char *)dst;
    int i;
    for (i = 0; i < n; i++) {
        cdst[i] = c;
    }
    return dst;
}

int strcmp(const char *p, const char *q) {
    while (*p && *p == *q)
        p++, q++;
    return (unsigned char)*p - (unsigned char)*q;
}