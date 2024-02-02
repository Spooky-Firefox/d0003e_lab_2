#include "tinythreads.h"
#include "lcd.h"
#include "primes.h"
#include <stdbool.h>

void printAt(long num, int pos) {
	int pp = pos;
	writeChar( (num % 100) / 10 + '0', pp);
	pp++;
	writeChar( num % 10 + '0', pp);
}

void computePrimes(int pos) {
	long n;

	for(n = 1; ; n++) {
		if (is_prime(n)) {
			printAt(n, pos);
		}
	}
}

int main() {
	spawn(computePrimes, 0);
	computePrimes(3);
}
