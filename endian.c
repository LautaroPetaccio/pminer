#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

uint32_t swap_uint32(uint32_t val) {
	return ((((val) << 24) & 0xff000000u) | (((val) << 8) & 0x00ff0000u) | (((val) >> 8) & 0x0000ff00u) | (((val) >> 24) & 0x000000ffu));
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

int main() {
	uint32_t be = swap_uint32(0x12345678);
	uint32_t be2 = swap_uint32(0x00000001);
	uint32_t be_res = be + be2;
	printf("Be res before converting \n");
	hexdump((unsigned char*) &be_res, 4);
	printf("Be res after converting \n");
	uint32_t converted = swap_uint32(be_res);
	hexdump((unsigned char*) &converted, 4);

	uint32_t le = 0x12345678;
	uint32_t le2 = 0x00000001;
	uint32_t le_res = le + le2;
	printf("Le res \n");
	hexdump((unsigned char*) &le_res, 4);

}