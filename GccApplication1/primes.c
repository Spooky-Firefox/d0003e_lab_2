#include <stdint.h>

uint32_t prime = 3;

uint8_t is_prime(uint32_t  i){
	uint32_t  n = 2;
	while(n < i){
		if (i % n == 0){
			return 0;
		}
		n++;
	}
	return 1;
}

// setts prime to be the next prime larger than the current prime
void next_prime(){
	prime = prime +2;
	while(!is_prime(prime)){
		prime = prime +2;
	}
}