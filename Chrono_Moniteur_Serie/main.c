/*
 * Chronometer application on Real Time Kernel (ATmega328P Xplained Mini)
 */
#define F_CPU 16000000
#include <avr/io.h>

#include "../include/BitFacilities.h"
#include "../include/uart0.h"
#include "../include/numericString.h"
#include "../include/kernel.h"

/* Prototypes des fonctions ADC (page 35) */
void InitADC0(void);
unsigned int ADC_ReadUint10(void);

/* Prototypes des 3 tâches (page 72) */
void taskTime(void);
void taskTemp(void);
void taskDisplay(void);

/* Descripteurs et piles des 3 tâches */
task desTime;
task desTemp;
task desDisplay;

stack taskTime_stk[STACK_SIZE];
stack taskTemp_stk[STACK_SIZE];
stack taskDisplay_stk[STACK_SIZE];

/* Boîte aux lettres et buffers globaux pour les messages */
mailBox MymailBox;
char strTime[8];
char strTemp[8];

/* Initialisation de l'ADC sur le canal ADC0 (potentiomètre A0 du shield VMA209) */
void InitADC0(void) {
	ADMUX  = 0x40; // Référence AVcc (5V), alignement à droite, canal ADC0 (0x44 pour LM35 sur A4)
	ADCSRA = 0xC7; // ADEN=1 (actif), ADSC=1 (1ère conversion), prescaler=128 (125 kHz)
	ADCSRB = 0x00;
}

/* Lecture 10 bits (0 à 1023) de l'ADC */
unsigned int ADC_ReadUint10(void) {
	ADCSRA |= 0x40;                  // Lance une conversion (bit ADSC à 1)
	while ( (ADCSRA & 0x40) != 0 ){} // Attend que ADSC repasse à 0 (fin de conversion)
	return ADC;
}

int main(void)
{
	initOutPutInput();
	InitADC0();
	USART_Init(MYUBRR);
	MB_Initialise(&MymailBox);

	// Création des 3 tâches avec les priorités de la page 72 :
	// 1 = Time processing, 2 = Temperature measurement, 3 = Displaying
	create(taskTime,    1, &taskTime_stk[STACK_SIZE - 1],    &desTime);
	create(taskTemp,    2, &taskTemp_stk[STACK_SIZE - 1],    &desTemp);
	create(taskDisplay, 3, &taskDisplay_stk[STACK_SIZE - 1], &desDisplay);

	startKernel();
	while (1) {
	}
}

/* Tâche 1 (Priorité 1) : gestion du temps (1/10 s) et choix d'affichage temps */
void taskTime(void) {
	unsigned char tenthSec = 0;
	unsigned char sec = 0;
	unsigned char min = 0;

	while (1) {
		// 1 quantum = 10 ms => wait(10) = 100 ms = 1/10 de seconde
		wait(10);

		tenthSec++;
		if (tenthSec >= 10) {
			tenthSec = 0;
			sec++;
			if (sec >= 60) {
				sec = 0;
				min++;
				if (min >= 60) {
					min = 0;
				}
			}
		}

		// On n'envoie le temps que si le bouton S2 (PC2) n'est PAS appuyé
		// (pour laisser la priorité d'affichage à la température quand S2 est appuyé)
		if (GetBooleanValue(PINC, maskBit2)) {
			if (!GetBooleanValue(PINC, maskBit1)) {
				// Bouton S1 (PC1) appuyé -> afficher sec.dixième (ex: 58.01)
				timeToString(sec, tenthSec, tenthSecAndSeconds, strTime);
			} else {
				// Aucun bouton appuyé -> afficher mn:sec (ex: 00:25)
				timeToString(min, sec, minutesAndSeconds, strTime);
			}
			MB_Send(&MymailBox, strTime);
		}
	}
}

/* Tâche 2 (Priorité 2) : mesure analogique lorsque le bouton S2 (PC2) est appuyé */
void taskTemp(void) {
	unsigned int adcVal;

	while (1) {
		// Bouton S2 (PC2) appuyé (état bas = 0)
		if (!GetBooleanValue(PINC, maskBit2)) {
			adcVal = ADC_ReadUint10();
			decimal4_to_ascii(adcVal, strTemp);
			MB_Send(&MymailBox, strTemp);
		}
		wait(20); // Mesure toutes les 200 ms
	}
}

/* Tâche 3 (Priorité 3) : attend un message dans la boîte aux lettres et l'affiche via UART */
void taskDisplay(void) {
	unsigned char *msg;

	while (1) {
		msg = (unsigned char *)MB_Get(&MymailBox);
		USART_MessageTransmit(msg);
	}
}
