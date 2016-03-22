#include "sha256.h"

int main() {
	char hash[65];
	sha256_hash("abc", 3, hash);
	return 0;
}