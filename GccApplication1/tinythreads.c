#include <setjmp.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "tinythreads.h"
#include "joy_stick.h"

#define NULL            0
// Disable interrupts
#define DISABLE()       cli()
// Enable interrupts
#define ENABLE()        sei()

#define STACKSIZE       80

// max number of threads
#define NTHREADS        4
#define SETSTACK(buf,a) *((unsigned int *)(buf)+8) = (unsigned int)(a) + STACKSIZE - 4; \
*((unsigned int *)(buf)+9) = (unsigned int)(a) + STACKSIZE - 4

// a struck containing info about the thread,
struct thread_block {
	void (*function)(int);   // code to run
	int arg;                 // argument to the above
	thread next;             // A pointer to the next thread to execute
	// struct thread_block* next;
	jmp_buf context;         // machine state
	char stack[STACKSIZE];   // execution stack space
};

// the queue of threads to execute next
struct thread_block threads[NTHREADS];

// the initial thread
struct thread_block initp;

// a queue of empty thread_blocks
thread freeQ   = threads;
// a queue of threads ready to be run
thread readyQ  = NULL;
// the current thread running
thread current = &initp;

// is globals initalized
int initialized = 0;

static void initialize(void) {
	//disable interrupt
	DISABLE();
	int i;
	// make the EMPTY threads point toward the next thread
	for (i=0; i<NTHREADS-1; i++){
		threads[i].next = &threads[i+1];
	}
	// make last thread point to NULL (int* 0)
	threads[NTHREADS-1].next = NULL;
	// set initalized to true

	// enable joystick interrupt
	// enable external interrupts on pins 15 to 8
	EIMSK = EIMSK | (1<<PCIE1);
	// enable interrupt for pin 15, joystick down
	PCMSK1 = PCMSK1 | (1<< PCINT12);
	// enable pull upp resistor and other
	setupJOYSTICK();

	// enable timer 1 interrupt
	// use the 8 MHz system clock
	// prescaler 1024
	// clkio/256 = 0b101 = 0x5
	TCCR1B = 0x5 << CS10;
	// set compare on match
	// clear on timer match
	// set OCA1 to high on match
	// 0b11 = 0x3
	TCCR1A = 0x3 << COM1A0;
	// enable the interrupt of compare unit A
	// OCIE1A = Output Compare Interrupt Enable (Timer) 1 (Unit) A
	TIMSK1 = TIMSK1 | 1 << OCIE1A;

	// 8 MHz 
	// 8_000_000 / 1024 = 7812,5 clk/sec = 7,8125 clk/ms
	// 50 * 7,8125 = 390,625 clk / 50ms
	// 391 = 0x0187
	// writing to tmp higher byte off ouput compare register timer 1 module a
	OCR1AH = 0x01;
	// writing to lower byte, causing tmp to be written to higher
	OCR1AL = 0x87;

	TCNT1H = 0x00; // write to tmp
	TCNT1L = 0x00; // write to lower causing temp to write to higher
	

	initialized = 1;
	// enable interrupt
	ENABLE();
}

// ## Add thread to last position in queue, fifo
// - `p`: the thread to add
// - `queue`: the queue to add it to
static void enqueue(thread p, thread *queue) {
	// set the thread next to null
	p->next = NULL;

	// if the queue is empty make the queue point to p as the first element
	if (*queue == NULL) {
		*queue = p;
	} else {
		thread q = *queue;
		// loops until the last element in the queue, q->next == NULL
		while (q->next){
			q = q->next;
		}
		// make last element point to the new thread
		q->next = p;
	}
}

// ## Remove first element in queue and return it else return current
// - `queue`: The queue to operate on
// ### notes
// remember that thread is a pointer
// ```
//	queue -> intermidiate -> address in thread_block array
// ```
static thread dequeue(thread *queue) {
	// copy the first element of the queue to the p variable
	thread p = *queue;

	// if next item in queue exist (isn't null)
	// test if it actually points to thread_block in memory
	// dereference is necessary to test the final part of the chain the
	// intermediate given from caller actually points to a thread block
	// the intermediate is often freeQ and readyQ
	if (*queue) {
		// make the new first element of the queue be the next element
		*queue = (*queue)->next;
	} else {
		// Empty queue, kernel panic!!!
		// most likely trigger is that the queue of free thread blocks is empty,
		// meaning that the kernel dont have more thread to give out.
		//TODO make error code on screen
		while (1) ;  // not much else to do...
	}
	return p;
}

// ## Switches execution to the given thread
// - `next` the next thread t execute
static void dispatch(thread next) {
	// if thread jumps to setjmp, it return 1
	// so if a thread jumps here the function exits an allows continuation
	// of the thread that called dispatch
	// example, thread B calls dispatch to thread A
	// B calls setjmp returns 0
	// sets current thread to next, this case A
	// jumps to context of A
	// lands in "if" and return 1, A continues with its work
	if (setjmp(current->context) == 0) {
		current = next;
		longjmp(next->context,1);
	}
}

void spawn(void (* function)(int), int arg) {
	thread newp;

	// Disables interrupt
	DISABLE();

	// are globals initalized? if not initialize them
	if (!initialized){ initialize();}
	// get pointer to thread_block (aka thread) from one off the free blocks
	newp = dequeue(&freeQ);
	// set the function of the new thread
	newp->function = function;
	// the arguments
	newp->arg = arg;
	// and make the next
	newp->next = NULL;

	// is not run of first pass, runs when the thread is dispatched
	if (setjmp(newp->context) == 1) {
		ENABLE();
		current->function(current->arg);
		// thread is done
		DISABLE();
		// add current thread to queue of free thread_blocks
		// will remove the current thread from ready queue
		enqueue(current, &freeQ);
		// run next thread in the queue
		dispatch(dequeue(&readyQ));
	}

	// give up, get some help, its jover
	SETSTACK(&newp->context, &newp->stack);

	enqueue(newp, &readyQ);
	// enable interupt
	ENABLE();
}

// swith context to next thread
void yield(void) {
	DISABLE();
	// add curent thread to queue
	enqueue(current, &readyQ);
	// run the next thread
	ENABLE();
	dispatch(dequeue(&readyQ));
}

void lock(mutex *m) {
	DISABLE();
	if (m->locked){
		ENABLE();
		enqueue(current,&m->waitQ);
		dispatch(dequeue(&readyQ));
	} else {
		m->locked = 1;
		ENABLE();
	}
}

// a locks
// b tries to lock, gets put in m:s waitQ
// a unlocks, puts itself in readyQ, b takes over
void unlock(mutex *m) {
	DISABLE();
	if (m->waitQ){
		enqueue(current,&readyQ);
		ENABLE();
		dispatch(dequeue(&m->waitQ));// when yield is called on thread, thread is added to readyQ
	}
	else{
		m->locked = 0;
		ENABLE();
	}
}


ISR(PCINT1_vect){
	if (is_joistick_down()){
		while(1){};
		yield();
	}
}

ISR(TIMER1_COMPA_vect){
	// when interrupt executes bit is cleared
	TCNT1H = 0x00; // write to tmp
	TCNT1L = 0x00; // write to lower causing temp to write to higher
	yield();
}
