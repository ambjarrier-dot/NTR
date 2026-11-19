#include "../include/kernel.h"

static task *runningTask;
static task *pt_operate;
static task *head = NULL;

char indexTask;

static task *readyTasks[NB_TASKS] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

/* Preemption */

ISR(TIMER1_COMPA_vect, ISR_NAKED);

void init_quantum(void) {
    TCCR1A = 0;
    TCCR1B = 0x0C;
    TIMSK1 = 2;
    TCNT1  = 0;
    OCR1A  = QUANTUM;
    ENABLE_IT();
}

static void operateCounters(void) {
    pt_operate = head;
    while (pt_operate != NULL) {
        if (pt_operate->state == WAITING && pt_operate->counter != 0) {
            pt_operate->counter--;
            if (pt_operate->counter == 0) {
                pt_operate->state = READY;
#ifdef PRIORITY
                readyTasks[(unsigned char)pt_operate->rank_prio] = pt_operate;
#endif
            }
        }
        pt_operate = pt_operate->theNext;
    }
}

ISR(TIMER1_COMPA_vect, ISR_NAKED) {
    SAVE_CONTEXT();
    RETRIEVE_SP();
    operateCounters();
    scheduler();
    UPDATE_SP();
    RESTORE_CONTEXT();
    START();
    reti();
}

/* Kernel primitives */

void create(void (*f)(void), char prio, stack *stack_bottom, task *newTask) {
    newTask->traitement = f;
    newTask->rank_prio  = prio;
    newTask->counter    = 0;
    newTask->state      = READY;

    newTask->theNext = head;
    head = newTask;

#ifdef PRIORITY
    readyTasks[(unsigned char)prio] = newTask;
#endif

    *(stack_bottom + 0) = (unsigned short int)f & 0xFF;
    *(stack_bottom - 1) = (unsigned short int)f / 256;

    newTask->addr_stack = stack_bottom - CONTEXT_SIZE;
}

void switchTask(void) {
    SAVE_CONTEXT();
    RETRIEVE_SP();
    scheduler();
    UPDATE_SP();
    RESTORE_CONTEXT();
    START();
}

/* Round Robin policy */
#ifdef SHARED
void startKernel(void) {
    CRITICAL_BEGINNING();
    create(background_task, 7, &background_task_stk[STACK_SIZE - 1], &des_background_task);
    runningTask = head;
    init_quantum();
    UPDATE_SP();
    RESTORE_CONTEXT();
    START();
}

void scheduler(void) {
    do {
        if (runningTask->theNext == NULL) runningTask = head;
        else runningTask = runningTask->theNext;
    } while (runningTask->state != READY);
}
#endif

/* Static priority policy */
#ifdef PRIORITY
void startKernel(void) {
    CRITICAL_BEGINNING();
    create(background_task, 7, &background_task_stk[STACK_SIZE - 1], &des_background_task);
    scheduler();
    init_quantum();
    UPDATE_SP();
    RESTORE_CONTEXT();
    START();
}

void scheduler(void) {
    for (indexTask = 0; indexTask < NB_TASKS; indexTask++) {
        if (readyTasks[(unsigned char)indexTask] != NULL) {
            runningTask = readyTasks[(unsigned char)indexTask];
            break;
        }
    }
}
#endif

/* Time management */

void wait(unsigned int quantaNumber) {
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

/* Semaphores */

void S_Initialise(semaphore *S, char tokenNb) {
    S->counter      = tokenNb;
    S->BGNING_QUEUE = NULL;
    S->END_QUEUE    = NULL;
}

void S_Pend(semaphore *S) {
    CRITICAL_BEGINNING();
    S->counter--;
    if (S->counter < 0) {
        runningTask->state = WAITING;
        runningTask->theNextInTheQueue = NULL;

        if (S->BGNING_QUEUE == NULL) {
            S->BGNING_QUEUE = runningTask;
        } else {
            S->END_QUEUE->theNextInTheQueue = runningTask;
        }
        S->END_QUEUE = runningTask;

#ifdef PRIORITY
        readyTasks[(unsigned char)runningTask->rank_prio] = NULL;
#endif
        switchTask();
    }
    CRITICAL_END();
}

void S_Release(semaphore *S) {
    task *tacheReveillee;

    CRITICAL_BEGINNING();
    S->counter++;
    if (S->BGNING_QUEUE != NULL) {
        tacheReveillee = S->BGNING_QUEUE;
        S->BGNING_QUEUE = tacheReveillee->theNextInTheQueue;
        if (S->BGNING_QUEUE == NULL) {
            S->END_QUEUE = NULL;
        }
        tacheReveillee->theNextInTheQueue = NULL;
        tacheReveillee->state = READY;

#ifdef PRIORITY
        readyTasks[(unsigned char)tacheReveillee->rank_prio] = tacheReveillee;
#endif
        switchTask();
    }
    CRITICAL_END();
}

/* Mailboxes */

void MB_Initialise(mailBox *MB) {
    MB->message      = NULL;
    MB->BGNING_QUEUE = NULL;
    MB->END_QUEUE    = NULL;
}

void MB_Send(mailBox *MB, void *donnees) {
    task *tacheReveillee;

    CRITICAL_BEGINNING();
    MB->message = donnees;
    if (MB->BGNING_QUEUE != NULL) {
        tacheReveillee = MB->BGNING_QUEUE;
        MB->BGNING_QUEUE = tacheReveillee->theNextInTheQueue;
        if (MB->BGNING_QUEUE == NULL) {
            MB->END_QUEUE = NULL;
        }
        tacheReveillee->theNextInTheQueue = NULL;
        tacheReveillee->state = READY;

#ifdef PRIORITY
        readyTasks[(unsigned char)tacheReveillee->rank_prio] = tacheReveillee;
#endif
        switchTask();
    }
    CRITICAL_END();
}

void *MB_Get(mailBox *MB) {
    void *response;

    CRITICAL_BEGINNING();
    while (MB->message == NULL) {
        runningTask->state = WAITING;
        runningTask->theNextInTheQueue = NULL;

        if (MB->BGNING_QUEUE == NULL) {
            MB->BGNING_QUEUE = runningTask;
        } else {
            MB->END_QUEUE->theNextInTheQueue = runningTask;
        }
        MB->END_QUEUE = runningTask;

#ifdef PRIORITY
        readyTasks[(unsigned char)runningTask->rank_prio] = NULL;
#endif
        switchTask();
    }
    response = MB->message;
    MB->message = NULL;
    CRITICAL_END();
    return response;
}

void *MB_LookAt(mailBox *MB) {
    void *reponse;

    CRITICAL_BEGINNING();
    reponse = MB->message;
    MB->message = NULL;
    CRITICAL_END();
    return reponse;
}
