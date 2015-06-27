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