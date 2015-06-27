#ifndef _UTIL_H
#define _UTIL_H
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

void bin2hex(char *s, const unsigned char *p, const size_t len);
uint64_t swap_uint64(const uint64_t val);
uint32_t swap_uint32(const uint32_t val);
void byte_swap(unsigned char* data, int len);
bool hex2bin(unsigned char *dest, const char *hexstr, size_t len);

#endif /* util.h */