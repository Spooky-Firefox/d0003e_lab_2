/*
 * GccApplication1.c
 *
 * Created: 2024-01-30 14:10:46
 * Author : oller
 */ 

#include <avr/io.h>
#include "lcd.h"
#include "primes.h"
#include "tinythreads.h"
// #include "switch_thread_interupt.c" //TODO add header file
mutex pp_mutex = MUTEX_INIT;
int pp;
void printAt(long num, int pos) {
	lock(&pp_mutex);	
	pp = pos;
	// to create lock conflict and create issues
	// for (volatile uint16_t i = 0; i < 10000; i++){};
	writeChar( (num % 100) / 10 + '0', pp);
	pp++;
	writeChar( num % 10 + '0', pp);
	unlock(&pp_mutex);
}

void computePrimes(int pos) {
	long n;

	for(n = 1; ; n++) {
		if (is_prime(n)) {
			printAt(n, pos);
			// yield();
		}
	}
}

int main() {
	setupLCD();
	spawn(computePrimes, 0);
	computePrimes(3);
}
