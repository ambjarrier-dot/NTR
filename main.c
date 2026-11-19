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

task des1;
task des2;
task des3;

stack task1_stk[STACK_SIZE];
stack task2_stk[STACK_SIZE];
stack task3_stk[STACK_SIZE];

semaphore MySem;
mailBox MymailBox;

int main(void)
{
	initOutPutInput();
	USART_Init(MYUBRR);
	USART_MessageTransmit((unsigned char *)"HELLO\n\r");

	// Initialisation du sémaphore de synchronisation à 0 jeton
	S_Initialise(&MySem, 0);

	create(task1, 3, &task1_stk[STACK_SIZE-1], &des1);
	create(task2, 2, &task2_stk[STACK_SIZE-1], &des2);
	create(task3, 1, &task3_stk[STACK_SIZE-1], &des3);

	startKernel();
	while(1){
	}
}

// Tâche 1 : détection d'un appui sur S1 (PC1) -> libère le sémaphore
void task1(void){
	unsigned char flag = 1;
	while(1){
		// Bouton S1 appuyé (état 0) et pas encore pris en compte
		if (!GetBooleanValue(PINC, maskBit1) && (flag == 1)) {
			S_Release(&MySem);
			flag = 0;
			wait(3); // anti-rebond de 30 ms
		}
		// Bouton S1 relâché (état 1) -> réarmement du flag
		if (GetBooleanValue(PINC, maskBit1) && (flag == 0)) {
			flag = 1;
			wait(3); // anti-rebond de 30 ms
		}
	}
}

// Tâche 2 : attend le sémaphore et complémente la LED D3 (PB5)
void task2(void){
	while(1){
		S_Pend(&MySem);
		PINB = (1 << 5); // écriture d'un 1 dans PINB5 -> inverse PORTB5 atomiquement
	}
}

// Tâche 3 (témoin) : LED D1 (PB3) suit directement l'état du bouton S2 (PC2)
void task3(void){
	while(1){
		PINB = (1 << 4); // inverse PORTB4 (LED D2)
		wait(50);        // 50 * 10 ms = 500 ms par état (période de 1 s)
	}
}