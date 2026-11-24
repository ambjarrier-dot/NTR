#ifndef KERNEL_H_
#define KERNEL_H_

#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>    // Needed to use interrupts

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

// directives of compilation
#define SHARED
//#define PRIORITY

// definition of type stack
typedef uint8_t stack;

#define STACK_SIZE 100
#define CONTEXT_SIZE 35
#define QUANTUM 625           // 16 MHz / 256 / 625 = 100 Hz => 1 quantum = 10 ms

/* nb of the tasks to be scheduled */
#define NB_TASKS 8

// State of a task
#define RUNNING		1
#define READY		2
#define WAITING		3
#define SLEEPING	4

#define ENABLE_IT()  sei()
#define DISABLE_IT() cli()

#define CRITICAL_BEGINNING() DISABLE_IT() /* zone critique qui ne peut pas etre interrompu */
#define CRITICAL_END()       ENABLE_IT()

#define RETRIEVE_SP() (runningTask->addr_stack) = (stack *)SP
#define UPDATE_SP()   SP = (uint16_t)(runningTask->addr_stack)
#define START()       reti()

#define SAVE_CONTEXT() asm volatile( \
  "push  r0                    \n\t"  \
  "in    r0, __SREG__          \n\t"  \
  "cli                         \n\t"  \
  "push  r0                    \n\t"  \
  "push  r1                    \n\t"  \
  "clr   r1                    \n\t"  \
  "push  r2                    \n\t"  \
  "push  r3                    \n\t"  \
  "push  r4                    \n\t"  \
  "push  r5                    \n\t"  \
  "push  r6                    \n\t"  \
  "push  r7                    \n\t"  \
  "push  r8                    \n\t"  \
  "push  r9                    \n\t"  \
  "push  r10                   \n\t"  \
  "push  r11                   \n\t"  \
  "push  r12                   \n\t"  \
  "push  r13                   \n\t"  \
  "push  r14                   \n\t"  \
  "push  r15                   \n\t"  \
  "push  r16                   \n\t"  \
  "push  r17                   \n\t"  \
  "push  r18                   \n\t"  \
  "push  r19                   \n\t"  \
  "push  r20                   \n\t"  \
  "push  r21                   \n\t"  \
  "push  r22                   \n\t"  \
  "push  r23                   \n\t"  \
  "push  r24                   \n\t"  \
  "push  r25                   \n\t"  \
  "push  r26                   \n\t"  \
  "push  r27                   \n\t"  \
  "push  r28                   \n\t"  \
  "push  r29                   \n\t"  \
  "push  r30                   \n\t"  \
  "push  r31                   \n\t"  \
)

#define RESTORE_CONTEXT() asm volatile ( \
  "pop  r31                    \n\t" \
  "pop  r30                    \n\t" \
  "pop  r29                    \n\t" \
  "pop  r28                    \n\t" \
  "pop  r27                    \n\t" \
  "pop  r26                    \n\t" \
  "pop  r25                    \n\t" \
  "pop  r24                    \n\t" \
  "pop  r23                    \n\t" \
  "pop  r22                    \n\t" \
  "pop  r21                    \n\t" \
  "pop  r20                    \n\t" \
  "pop  r19                    \n\t" \
  "pop  r18                    \n\t" \
  "pop  r17                    \n\t" \
  "pop  r16                    \n\t" \
  "pop  r15                    \n\t" \
  "pop  r14                    \n\t" \
  "pop  r13                    \n\t" \
  "pop  r12                    \n\t" \
  "pop  r11                    \n\t" \
  "pop  r10                    \n\t" \
  "pop  r9                     \n\t" \
  "pop  r8                     \n\t" \
  "pop  r7                     \n\t" \
  "pop  r6                     \n\t" \
  "pop  r5                     \n\t" \
  "pop  r4                     \n\t" \
  "pop  r3                     \n\t" \
  "pop  r2                     \n\t" \
  "pop  r1                     \n\t" \
  "pop  r0                     \n\t" \
  "out  __SREG__, r0           \n\t" \
  "pop  r0                     \n\t" \
)

/***********************************************************/
// definition of the type Task Control Structure
typedef struct TaskControl {
	stack *addr_stack;                     // address of the stack associated with the task
	void (*traitement)(void);              // address of the task
	char rank_prio;                        // priority
	char state;                            // state: RUNNING, READY, WAITING, SLEEPING
	unsigned int counter;                  // time of waiting state of the task in time quanta
	struct TaskControl *theNextInTheQueue; // points to the next task waiting for a resource or an event
	struct TaskControl *theNext;           // points to the next task in the linked list
} task;

/***********************************************************/
// definition of the type semaphore
typedef struct {
	task *BGNING_QUEUE;
	task *END_QUEUE;
	signed char counter;
} semaphore;

/***********************************************************************/
// definition du type MailBox
typedef struct {
	task *BGNING_QUEUE;
	task *END_QUEUE;
	void *message;
} mailBox;

/***********************************************************************/
extern void init_quantum(void);

extern void create(void (*f)(void), char prio, stack *stackBottom, task *newTask);
extern void switchTask(void) __attribute__((naked));
extern void startKernel(void);
extern void scheduler(void);

extern void wait(unsigned int quantaNumber);

////////////////////// semaphore management ////////////////////////
extern void S_Initialise(semaphore *S, char tokenNb);
extern void S_Pend(semaphore *S);
extern void S_Release(semaphore *S);

////////////////////// Mailbox management ////////////////////////
extern void MB_Initialise(mailBox *MB);
extern void MB_Send(mailBox *MB, void *donnees);
extern void *MB_Get(mailBox *MB);
extern void *MB_LookAt(mailBox *MB);

#endif /* KERNEL_H_ */
