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

	create(task1, 3, &task1_stk[STACK_SIZE-1], &des1);
	create(task2, 2, &task2_stk[STACK_SIZE-1], &des2);
	create(task3, 1, &task3_stk[STACK_SIZE-1], &des3);

	startKernel();
	while(1){
	}
}

void task1(void){
	while(1){
		PINB = (1 << 5);     // inverse PB5 (LED D3) atomiquement
		wait(50);            // 50 * 10 ms = 500 ms par état (période 1 s)
	}
}

void task2(void){
	while(1){
		PINB = (1 << 4);     // inverse PB4 (LED D2) atomiquement
		wait(20);            // 20 * 10 ms = 200 ms par état (période 0,4 s)
	}
}

void task3(void){
	while(1){
		if (GetBooleanValue(PINC, maskBit1)) SetPORTBPin3();   // S1 relâché -> LED D1 éteinte
		else                                 ClearPORTBPin3(); // S1 appuyé  -> LED D1 allumée
	}
}
