/*
 * BitFacilities.c
 *
 * Created: 11/05/2021 16:41:12
 *  Author: sbouter
 */

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include "../include/BitFacilities.h"

static unsigned char Dis_table[] = {0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90};
static unsigned char Dis_buf[]   = {0xF1, 0xF2, 0xF4, 0xF8};

unsigned char disbuff[] = {0, 0, 0, 0};
signed char dotPos      = -1;

void initOutPutInput(void) {
	DDRB  |= 0x39; // PB3, PB4, PB5 (LEDs D1-D3) et PB0 (dataPin 7-segments) en sortie
	DDRD  |= 0x90; // PD4 (latchPin) et PD7 (clockPin) en sortie
	DDRC  &= 0xF0; // PC1, PC2, PC3 (boutons S1-S3) et PC0 (ADC0) en entrée

	PORTB |= 0x38; // LEDs D1-D3 éteintes au démarrage
}

void shiftOut(uint8_t command) {
	for (unsigned char i = 0; i < 8; i++) {
		unsigned char output = command & 0b10000000;
		command = command << 1;

		if (output) SetDataPin();
		else        ClearDataPin();

		SetClockPin();
		ClearClockPin();
	}
}

void display(void) {
	for (signed char i = 0; i <= 3; i++) {
		unsigned char segments = Dis_table[disbuff[i]];

		// Allume le point décimal (bit 7 à 0) sur le digit choisi
		if (i == dotPos) {
			segments &= 0x7F;
		}

		ClearlatchPin();
		shiftOut(segments);    // Envoie le motif des segments du chiffre
		shiftOut(Dis_buf[i]);  // Sélectionne le digit actif (0 à 3)
		SetlatchPin();
		_delay_ms(2);          // 2 ms par digit => 8 ms par image (~125 Hz, sans scintillement)
	}
}
