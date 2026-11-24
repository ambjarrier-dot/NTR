/*
 * Chronometer application on Real Time Kernel with 7-segment display (VMA209)
 */
#define F_CPU 16000000UL
#include <avr/io.h>

#include "../include/BitFacilities.h"
#include "../include/numericString.h"
#include "../include/kernel.h"

/* Prototypes ADC */
void InitADC0(void);
unsigned int ADC_ReadUint10(void);

/* Prototypes des 3 tâches */
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

/* Boîte aux lettres et buffers de messages */
mailBox MymailBox;
char strTime[8];
char strTemp[8];

/* Initialisation de l'ADC sur ADC0 (potentiomètre A0 du shield) */
void InitADC0(void) {
	ADMUX  = 0x40; // Référence AVcc (5V), alignement à droite, canal ADC0 (0x44 pour LM35 sur A4)
	ADCSRA = 0xC7; // ADEN=1, ADSC=1, prescaler=128
	ADCSRB = 0x00;
}

/* Lecture 10 bits (0 à 1023) de l'ADC */
unsigned int ADC_ReadUint10(void) {
	ADCSRA |= 0x40;
	while ((ADCSRA & 0x40) != 0) {}
	return ADC;
}

int main(void) {
	initOutPutInput();
	InitADC0();
	MB_Initialise(&MymailBox);

	// Priorités statiques : 1 = Temps, 2 = Température/ADC, 3 = Affichage 7 segments
	create(taskTime,    1, &taskTime_stk[STACK_SIZE - 1],    &desTime);
	create(taskTemp,    2, &taskTemp_stk[STACK_SIZE - 1],    &desTemp);
	create(taskDisplay, 3, &taskDisplay_stk[STACK_SIZE - 1], &desDisplay);

	startKernel();
	while (1) {
	}
}

/* Tâche 1 (Priorité 1) : gestion du temps au 1/10 de seconde */
void taskTime(void) {
	unsigned char tenthSec = 0;
	unsigned char sec = 0;
	unsigned char min = 0;

	while (1) {
		wait(10); // 10 quanta * 10 ms = 100 ms (1/10 s)

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

		// Envoie le temps uniquement si S2 (PC2) n'est pas appuyé
		if (GetBooleanValue(PINC, maskBit2)) {
			if (!GetBooleanValue(PINC, maskBit1)) {
				// S1 appuyé : secondes et dixièmes (ex: "58.01")
				timeToString(sec, tenthSec, tenthSecAndSeconds, strTime);
			} else {
				// Aucun bouton appuyé : minutes et secondes (ex: "00:25")
				timeToString(min, sec, minutesAndSeconds, strTime);
			}
			MB_Send(&MymailBox, strTime);
		}
	}
}

/* Tâche 2 (Priorité 2) : mesure analogique si S2 (PC2) est appuyé */
void taskTemp(void) {
	unsigned int adcVal;

	while (1) {
		if (!GetBooleanValue(PINC, maskBit2)) {
			adcVal = ADC_ReadUint10();
			decimal4_to_ascii(adcVal, strTemp); // ex: "0554 "
			MB_Send(&MymailBox, strTemp);
		}
		wait(20); // Mesure toutes les 200 ms
	}
}

/* Tâche 3 (Priorité 3) : décode les messages et multiplexe l'afficheur 7 segments */
void taskDisplay(void) {
	char *msg;

	while (1) {
		// Lecture non bloquante pour ne jamais figer le multiplexage de l'afficheur
		msg = (char *)MB_LookAt(&MymailBox);

		if (msg != NULL) {
			if (msg[2] == ':' || msg[2] == '.') {
				// Format temps : "MM:SS" ou "SS.0D" -> on saute le séparateur en msg[2]
				disbuff[0] = (unsigned char)(msg[0] - '0');
				disbuff[1] = (unsigned char)(msg[1] - '0');
				disbuff[2] = (unsigned char)(msg[3] - '0');
				disbuff[3] = (unsigned char)(msg[4] - '0');
				dotPos = 1; // Point décimal allumé sur le 2e digit (ex: 00.25 ou 58.01)
			} else {
				// Format mesure 4 chiffres : "0554 "
				disbuff[0] = (unsigned char)(msg[0] - '0');
				disbuff[1] = (unsigned char)(msg[1] - '0');
				disbuff[2] = (unsigned char)(msg[2] - '0');
				disbuff[3] = (unsigned char)(msg[3] - '0');
				dotPos = -1; // Pas de point décimal
			}
		}

		display(); // Balaye les 4 digits en continu
	}
}
