#define F_CPU 16000000
#include <avr/io.h>
#include <util/delay.h>

#include "../include/BitFacilities.h"
#include "../include/uart0.h"
#include "../include/kernel.h"

void task1(void);
void task2(void);
void task3(void);

task des1;
task des2;
task des3;

stack task1_stk[STACK_SIZE];
stack task2_stk[STACK_SIZE];
stack task3_stk[STACK_SIZE];

semaphore MySem;
mailBox MymailBox;

char msg1[] = "Message 1 : Bouton S1 appuye\n\r";
char msg2[] = "Message 2 : Bouton S2 appuye\n\r";

int main(void)
{
    initOutPutInput();

    DDRB |= (1 << 5);
    DDRC &= ~((1 << 3) | (1 << 2) | (1 << 1));
    PORTC |= (1 << 3) | (1 << 2) | (1 << 1);

    USART_Init(MYUBRR);
    USART_MessageTransmit("TEST MAILBOX\n\r");

    MB_Initialise(&MymailBox);

    create(task1, 1, &task1_stk[STACK_SIZE - 1], &des1);
    create(task2, 2, &task2_stk[STACK_SIZE - 1], &des2);
    create(task3, 3, &task3_stk[STACK_SIZE - 1], &des3);

    startKernel();

    while (1) {
    }
}

void task1(void) {
    unsigned char flag1 = 1;
    unsigned char flag2 = 1;

    while (1) {
        if (!(PINC & (1 << 1)) && (flag1 == 1)) {
            MB_Send(&MymailBox, (void *)msg1);
            flag1 = 0;
            wait(20);
        }
        if (PINC & (1 << 1)) {
            flag1 = 1;
        }

        if (!(PINC & (1 << 2)) && (flag2 == 1)) {
            MB_Send(&MymailBox, (void *)msg2);
            flag2 = 0;
            wait(20);
        }
        if (PINC & (1 << 2)) {
            flag2 = 1;
        }
    }
}

void task2(void) {
    char *receivedMsg;
    while (1) {
        receivedMsg = (char *)MB_Get(&MymailBox);
        if (receivedMsg != NULL) {
            USART_MessageTransmit(receivedMsg);
        }
    }
}

void task3(void) {
    while (1) {
        if (PINC & (1 << 3)) PORTB |=  (1 << 5);
        else                 PORTB &= ~(1 << 5);
    }
}
