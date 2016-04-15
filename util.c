#include "util.h"

/* Change uint64 endianess */
uint64_t swap_uint64(uint64_t val) {
    val = ((val << 8) & 0xFF00FF00FF00FF00ULL ) | ((val >> 8) & 0x00FF00FF00FF00FFULL );
    val = ((val << 16) & 0xFFFF0000FFFF0000ULL ) | ((val >> 16) & 0x0000FFFF0000FFFFULL );
    return (val << 32) | (val >> 32);
}

/* Change uint32 endianess */
uint32_t swap_uint32(uint32_t val) {
	return ((((val) << 24) & 0xff000000u) | (((val) << 8) & 0x00ff0000u) | (((val) >> 8) & 0x0000ff00u) | (((val) >> 24) & 0x000000ffu));
}

uint32_t le32dec(const void *pp) {
	const uint8_t *p = (uint8_t const *)pp;
	return ((uint32_t)(p[0]) + ((uint32_t)(p[1]) << 8) +
	    ((uint32_t)(p[2]) << 16) + ((uint32_t)(p[3]) << 24));
}

uint32_t be32dec(const void *pp)
{
	const uint8_t *p = (uint8_t const *)pp;
	return ((uint32_t)(p[3]) + ((uint32_t)(p[2]) << 8) +
	    ((uint32_t)(p[1]) << 16) + ((uint32_t)(p[0]) << 24));
}

void le32enc(void *pp, uint32_t x) {
	uint8_t *p = (uint8_t *)pp;
	p[0] = x & 0xff;
	p[1] = (x >> 8) & 0xff;
	p[2] = (x >> 16) & 0xff;
	p[3] = (x >> 24) & 0xff;
}

void be32enc(void *pp, uint32_t x) {
	uint8_t *p = (uint8_t *)pp;
	p[3] = x & 0xff;
	p[2] = (x >> 8) & 0xff;
	p[1] = (x >> 16) & 0xff;
	p[0] = (x >> 24) & 0xff;
}


/* Endian swap a string */
void byte_swap(unsigned char* data, int len) {
        int c;
        unsigned char tmp[len];
       
        c=0;
        while(c<len)
        {
                tmp[c] = data[len-(c+1)];
                c++;
        }
       
        c=0;
        while(c<len)
        {
                data[c] = tmp[c];
                c++;
        }
}

/* Converts hex string to binary */
bool hex2bin(unsigned char *dest, const char *hexstr, size_t len) {
	char hex_byte[3];
	char *ep;

	hex_byte[2] = '\0';

	while (*hexstr && len) {
		if (!hexstr[1]) {
			fprintf(stderr, "hex2bin str truncated\n");
			return false;
		}
		hex_byte[0] = hexstr[0];
		hex_byte[1] = hexstr[1];
		*dest = (unsigned char) strtol(hex_byte, &ep, 16);
		if (*ep) {
			fprintf(stderr, "hex2bin failed on %s\n", hex_byte);
			return false;
		}
		dest++;
		hexstr += 2;
		len--;
	}

	return (len == 0 && *hexstr == 0) ? true : false;
}

/* Converts binary to hex string */
void bin2hex(char *s, const unsigned char *p, const size_t len) {
	int i;
	for (i = 0; i < len; i++)
		sprintf(s + (i * 2), "%02x", (unsigned int) p[i]);
}

bool fulltest(const uint32_t *hash, const uint32_t *target) {
	for (int i = 7; i >= 0; i--) {
		if (hash[i] > target[i]) {
			return false;
		}
		if (hash[i] < target[i]) {
			return true;
		}
	}
	return false;
}

void diff_to_target(uint32_t *target, double diff) {
	uint64_t m;
	int k;
	
	for (k = 6; k > 0 && diff > 1.0; k--)
		diff /= 4294967296.0;
	m = 4294901760.0 / diff;
	if (m == 0 && k == 6)
		memset(target, 0xff, 32);
	else {
		memset(target, 0, 32);
		target[k] = (uint32_t)m;
		target[k + 1] = (uint32_t)(m >> 32);
	}
}

void hexdump(unsigned char* data, int len) {
        int c;
       
        c=0;
        while(c < len)
        {
                printf("%.2x", data[c++]);
        }
        printf("\n");
}