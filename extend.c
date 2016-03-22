#include <stdio.h>
#include <stdlib.h>

#define s0(x)           (ROTR(x, 7) ^ ROTR(x, 18) ^ (x >> 3))
#define s1(x)           (ROTR(x, 17) ^ ROTR(x, 19) ^ (x >> 10))
#define ROTR(x, n)      ((x >> n) | (x << (32 - n)))

unsigned int extend(unsigned int wi16, unsigned int wi15, unsigned int wi7, unsigned int wi2) {
	printf("s0 %08x\n", s0(wi15));
	printf("s1 %08x\n", s1(wi2));
	return wi16 + s0(wi15) + wi7 + s1(wi2);
}

int main() {
	unsigned int wi16 = 0x12345678;
	unsigned int wi15 = 0x12345678;
	unsigned int wi7 = 0x12345678;
	unsigned int wi2 = 0x12345678;
	unsigned int result;
	result = extend(wi16,wi15,wi7,wi2);
	printf("Resultado %08x \n", result);

	return 0;
}