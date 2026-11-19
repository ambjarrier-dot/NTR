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

mailBox MymailBox;

char msg1[] = "Message 1 : Bouton S1 appuye\n\r";
char msg2[] = "Message 2 : Bouton S2 appuye\n\r";

int main(void)
{
	/* Initialise PB3..PB5 (LEDs) en sortie et PC1..PC3 (boutons) en entree via BitFacilities */
	initOutPutInput();

	/* Activation des pull-ups internes sur les boutons S1 (PC1), S2 (PC2), S3 (PC3) */
	PORTC |= (1 << 3) | (1 << 2) | (1 << 1);

	/* Configuration des broches du registre a decalage pour display() (PD4=Latch, PD7=Clock, PB0=Data) */
	DDRD |= (1 << 7) | (1 << 4);
	DDRB |= (1 << 0);

	/* Initialisation du tampon d'affichage 7 segments de BitFacilities */
	disbuff[0] = 0;
	disbuff[1] = 0;
	disbuff[2] = 0;
	disbuff[3] = 0;

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

/* Tache 1 : Producteur (surveille S1 et S2 avec flag1 et flag2) */
void task1(void) {
	unsigned char flag1 = 1;
	unsigned char flag2 = 1;

	while (1) {
		if (!(PINC & (1 << 1)) && (flag1 == 1)) {
			MB_Send(&MymailBox, (void *)msg1);
			disbuff[3] = 1;
			flag1 = 0;
			wait(20);
		}
		if (PINC & (1 << 1)) {
			flag1 = 1;
		}

		if (!(PINC & (1 << 2)) && (flag2 == 1)) {
			MB_Send(&MymailBox, (void *)msg2);
			disbuff[3] = 2;
			flag2 = 0;
			wait(20);
		}
		if (PINC & (1 << 2)) {
			flag2 = 1;
		}
	}
}

/* Tache 2 : Consommateur (recupere le message dans la mailbox et l'envoie sur l'UART) */
void task2(void) {
	char *receivedMsg;
	while (1) {
		receivedMsg = (char *)MB_Get(&MymailBox);
		if (receivedMsg != NULL) {
			USART_MessageTransmit(receivedMsg);
		}
	}
}

/* Tache 3 : Tache temoin (S3 -> LED D3 + multiplexage afficheur BitFacilities) */
void task3(void) {
	while (1) {
		if (PINC & (1 << 3)) PORTB |=  (1 << 5);
		else                 PORTB &= ~(1 << 5);

		display();
	}
}
