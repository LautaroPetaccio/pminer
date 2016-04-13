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
	char hash_to_check[65];
	char target_to_check[65];
	int i;
	bool rc = true;
	bin2hex(hash_to_check, (const unsigned char *) hash, 32);
	bin2hex(target_to_check, (const unsigned char *) target, 32);
	printf("Checking for possible share:\nHash: %s\nTarget: %s\n", hash_to_check, target_to_check);
	
	for (i = 7; i >= 0; i--) {
		if (hash[i] > target[i]) {
			rc = false;
			break;
		}
		if (hash[i] < target[i]) {
			rc = true;
			break;
		}
	}
	return rc;
}
