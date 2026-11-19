/*
 * IOWithoutArduino.c
 *
 * Created: 05/05/2021 16:08:00
 * Author : sbouter
 */
#define F_CPU 16000000    // AVR clock frequency in Hz, used by util/delay.h
#include <avr/io.h>
#include <util/delay.h>

#include "../include/BitFacilities.h"
#include "../include/uart0.h"
#include "../include/kernel.h"

void task1(void);
void task2(void);
void task3(void);
void task4(void);

task des1;
task des2;
task des3;
task des4;

stack task1_stk[STACK_SIZE];
stack task2_stk[STACK_SIZE];
stack task3_stk[STACK_SIZE];
stack task4_stk[STACK_SIZE];

semaphore MySem;
mailBox MymailBox;

int main(void)
{
	initOutPutInput();
	USART_Init(MYUBRR);
	USART_MessageTransmit((unsigned char *)"HELLO\n\r");

	// Initialisation du sémaphore de synchronisation à 0 jeton
	S_Initialise(&MySem, 0);

	// Priorités uniques entre 0 et 6 (7 est réservé à background_task)
	// Plus le chiffre est petit, plus la priorité est élevée
	create(task1, 3, &task1_stk[STACK_SIZE-1], &des1);
	create(task2, 2, &task2_stk[STACK_SIZE-1], &des2);
	create(task3, 1, &task3_stk[STACK_SIZE-1], &des3);

	startKernel();
	while(1){
	}
}

// Tâche 1 (prio 3) : détection d'un appui sur S1 (PC1) -> libère le sémaphore
void task1(void){
	unsigned char flag = 1;
	while(1){
		if (!GetBooleanValue(PINC, maskBit1) && (flag == 1)) {
			S_Release(&MySem);
			flag = 0;
		}
		if (GetBooleanValue(PINC, maskBit1) && (flag == 0)) {
			flag = 1;
		}
		wait(4); // suspend 40 ms : laisse tourner task4 (prio 4) + fait anti-rebond
	}
}

// Tâche 2 (prio 2) : attend le sémaphore et complémente la LED D3 (PB5)
void task2(void){
	while(1){
		S_Pend(&MySem);
		PINB = (1 << 5); // inverse PORTB5 (LED D3) atomiquement
	}
}

// Tâche 3 (prio 1 - la plus haute) : LED D1 (PB3) suit l'état du bouton S2 (PC2)
void task3(void){
	while(1){
		if (GetBooleanValue(PINC, maskBit2)) SetPORTBPin3();   // S2 relâché -> D1 éteinte
		else                                 ClearPORTBPin3(); // S2 appuyé  -> D1 allumée
		wait(4); 
	}
}
