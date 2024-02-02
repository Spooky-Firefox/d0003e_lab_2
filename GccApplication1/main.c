/*
 * GccApplication1.c
 *
 * Created: 2024-01-30 14:10:46
 * Author : oller
 */ 

#include <avr/io.h>
#include "lcd.h"
#include "primes.h"
// #include "switch_thread_interupt.c" //TODO add header file

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
			// yield();
		}
	}
}

int main() {
	setupLCD();
	spawn(computePrimes, 0);
	computePrimes(3);
}





// void yeild(){
// 	enqueue(current, &readyQ);
// 	dispatch(dequeue(&readyQ));
// }

// void dispatch(Thread *next){
// 	if (setjmp(current->context) == 0){
// 		current = next;
// 		longjmp(next->context, 1);
// 	}
// }

// void spawn(void (*fun)(int), int arg){
// 	Thread *t = malloc();
// 	t->fun = fun;
// 	t->arg = arg;
// 	if (setjmp(t->context) == 1){
// 		current->fun(current->arg);
// 		dispatch(dequeue(&readyQ));
// 	}
// 	STACKPTR(t->context) = malloc(...);
// 	enqueue(t, readyQ);
// 	...
// }

