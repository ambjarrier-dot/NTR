#include "../include/kernel.h"


static task *runningTask;
static task *head = NULL;


// index of the task to be run
unsigned char indexTask;

// list of ready tasks: each element of the table points to the descriptor
// associated with the task.
// The element with the lowest index has the highest priority
static task *readyTasks[NB_TASKS] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

/***************************************************************/
/*******   task de fond : nécessaire lorsqu'aucune task   ******/
/*******                est activée                       ******/
/***************************************************************/
static stack background_task_stk[STACK_SIZE];
static task des_background_task;

static void background_task(void){
	init_quantum();
	ENABLE_IT();

	while(1){}
}


/////////////////////////// mechanism of preemption ///////////////////////////////

ISR(TIMER1_COMPA_vect, ISR_NAKED);

void init_quantum(void){
	// Initialise Timer 1 to trigger periodical interrupt
	// (WGM13, WGM12, WGM11, WGM10) must be set to (0,1,0,0) -> mode CTC
	TCCR1A = 0;      // WGM11, WGM10 = 0,0
	TCCR1B = 0x0C;   // prescaler = 256, WGM13, WGM12 = 0,1
	TIMSK1 = 2;      // Timer/Counter1, Output Compare A Match Interrupt Enable
	TCNT1 = 0;
	OCR1A = QUANTUM;
	ENABLE_IT();     // enabling interrupts
}

static void operateCounters(void){
	task *t = head;
	while (t != NULL) {
		if (t->counter != 0) {
			t->counter--;
			if (t->counter == 0) {
				t->state = READY;
				#ifdef PRIORITY
				readyTasks[(unsigned char)t->rank_prio] = t;
				#endif
			}
		}
		t = t->theNext;
	}
}

ISR(TIMER1_COMPA_vect, ISR_NAKED)
{
	SAVE_CONTEXT();
	RETRIEVE_SP();
	/*************************************************************/
	operateCounters();
	scheduler();
	UPDATE_SP();
	RESTORE_CONTEXT();
	START(); // START() exécute déjà reti()
}


//////////////////// primitives of the kernel /////////////////////////////////
void create(void (*f)(void), char prio, stack *stack_bottom, task *newTask){
	// renseigner le descripteur de tâche
	newTask->traitement = f;
	newTask->rank_prio = prio;
	newTask->counter = 0;
	newTask->theNextInTheQueue = NULL;

	// insertion dans la liste chaînée (utilisée par operateCounters)
	newTask->theNext = head;
	head = newTask;

	// initialisation de la pile pour le premier RESTORE_CONTEXT() + START()
	*(stack_bottom + 0) = (unsigned short int)f & 0xFF;  // adr de la tâche low
	*(stack_bottom - 1) = (unsigned short int)f >> 8;    // adr de la tâche high
	*(stack_bottom - 3) = 0x00;                          // SREG initial
	*(stack_bottom - 4) = 0x00;                          // r1 (__zero_reg__ à 0)

	newTask->addr_stack = stack_bottom - CONTEXT_SIZE;
	newTask->state = READY;

	#ifdef PRIORITY
	readyTasks[(unsigned char)newTask->rank_prio] = newTask;
	#endif
}

void switchTask(void){
	SAVE_CONTEXT();
	RETRIEVE_SP();
	scheduler();
	UPDATE_SP();
	RESTORE_CONTEXT();
	START();
}

//////////////////// starting the kernel and scheduling in the case of the Round Robin policy ///////////////////
#ifdef SHARED
void startKernel(void){
	CRITICAL_BEGINNING();
	create(background_task, NB_TASKS - 1, &background_task_stk[STACK_SIZE-1], &des_background_task);
	runningTask = head;  // pointe sur background_task, qui lancera init_quantum()
	UPDATE_SP();
	RESTORE_CONTEXT();
	START();
}

void scheduler(void){
	do {
		if (runningTask->theNext == NULL) runningTask = head;
		else runningTask = runningTask->theNext;
	} while (runningTask->state != READY);
}
#endif

//////////////////// starting the kernel and scheduling in the case of the static priority policy ///////////////////
#ifdef PRIORITY
void startKernel(void){
	CRITICAL_BEGINNING();
	create(background_task, NB_TASKS - 1, &background_task_stk[STACK_SIZE-1], &des_background_task);
	runningTask = &des_background_task; // lance background_task qui initialise le timer puis est préemptée au 1er tick
	UPDATE_SP();
	RESTORE_CONTEXT();
	START();
}

void scheduler(void){
	indexTask = 0;
	while ((indexTask < NB_TASKS - 1) && (readyTasks[indexTask] == NULL)) {
		indexTask++;
	}
	runningTask = readyTasks[indexTask];
}
#endif

////////////////// time management : delaying a task or suspending a task for a number of quanta ////////////////////////
void wait(unsigned int quantaNumber){
	if (quantaNumber == 0) return;
	CRITICAL_BEGINNING();
	runningTask->counter = quantaNumber;
	runningTask->state   = WAITING;
	#ifdef PRIORITY
	readyTasks[(unsigned char)runningTask->rank_prio] = NULL;
	#endif
	switchTask();
	CRITICAL_END();
}

/************************************************************************************/
/*                                                                                  */
/*                    services associés la gestion des semaphores                   */
/*                                                                                  */
/************************************************************************************/

/******* appel du service S_Initialiser **********/
void S_Initialise(semaphore *S, char tokenNb){
	S->counter      = tokenNb;
	S->BGNING_QUEUE = NULL;
	S->END_QUEUE    = NULL;
}

/******* calling the primitive S_Pend, wait for a semaphore **********/
void S_Pend(semaphore *S){
	CRITICAL_BEGINNING();
	S->counter--;
	if (S->counter < 0) {
		runningTask->state = WAITING;
		#ifdef PRIORITY
		readyTasks[(unsigned char)runningTask->rank_prio] = NULL;
		#endif
		runningTask->theNextInTheQueue = NULL;

		if (S->BGNING_QUEUE == NULL) {
			S->BGNING_QUEUE = runningTask;
			S->END_QUEUE    = runningTask;
		} else {
			S->END_QUEUE->theNextInTheQueue = runningTask;
			S->END_QUEUE = runningTask;
		}
		switchTask();
	}
	CRITICAL_END();
}

/******* calling the primitive S_Release, release the semaphore **********/
void S_Release(semaphore *S){
	CRITICAL_BEGINNING();
	S->counter++;
	if (S->counter <= 0) {
		task *t = S->BGNING_QUEUE;
		S->BGNING_QUEUE = t->theNextInTheQueue;

		if (S->BGNING_QUEUE == NULL) {
			S->END_QUEUE = NULL;
		}
		t->theNextInTheQueue = NULL;
		t->state = READY;
		#ifdef PRIORITY
		readyTasks[(unsigned char)t->rank_prio] = t;
		#endif
		switchTask();
	}
	CRITICAL_END();
}

/************************************************************************************/
/*                                                                                  */
/*                  services associated to the management of the mailboxes          */
/*                                                                                  */
/************************************************************************************/

/******* calling MB_Initialise **********/
void MB_Initialise(mailBox *MB){

}

/******* calling MB_Send ***************************/
void MB_Send(mailBox *MB, void *donnees){
	CRITICAL_BEGINNING();

	CRITICAL_END();
}

/******* Calling the MB_Get, return **********/
/******* the address of the data to collect ************************/
/*** if there is no message, the RT Kernel suspends the task running **********/
void *MB_Get(mailBox *MB){
	void *response;

	response = MB->message;
	MB->message = NULL;
	return response;
	CRITICAL_END();
}

/******* appel du service MB_Consulter, retire le message, renvoie **********/
/******* l adresse de la donnée à recupérer ************************/
/* si il y a aucun message la primitive renvoie NULL ************************/
void *MB_LookAt(mailBox *MB){
	char *reponse = NULL;

	return reponse;
	CRITICAL_END();
}
