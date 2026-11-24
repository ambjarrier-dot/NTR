/*
 * BitFacilities.h
 *
 * Created: 05/05/2021 16:47:01
 *  Author: sbouter
 */ 

#include <avr/io.h>
#ifndef BITFACILITIES_H_
#define BITFACILITIES_H_

#define maskBit0 1
#define maskBit1 2
#define maskBit2 4
#define maskBit3 8
#define maskBit4 16
#define maskBit5 32
#define maskBit6 64
#define maskBit7 128

#define maskOrBit0 0x01 
#define maskOrBit1 0x02
#define maskOrBit2 0x04
#define maskOrBit3 0x08
#define maskOrBit4 0x10
#define maskOrBit5 0x20
#define maskOrBit6 0x40
#define maskOrBit7 0x80

#define maskAndBit0 0xFE
#define maskAndBit1 0xFD
#define maskAndBit2 0xFB
#define maskAndBit3 0xF7
#define maskAndBit4 0xEF
#define maskAndBit5 0xDF
#define maskAndBit6 0xBF
#define maskAndBit7 0x7F






#define GetBooleanValue( byte , maskAnd ) (( byte & maskAnd) != 0 )

// A1 PORTC PC1
// A2 PORTC PC2
// A3 PORTC PC3

////////////// don't use the macros below in the case of Real Time Multitask app //////////////////// 
#define SetBooleanValue( byte , maskOr ) (byte |= maskOr )
#define ClearBooleanValue( byte , maskAnd ) (byte &= maskAnd)
/////////////////////////////////////////////////////////////////////////////////////////////////////

////////////// the macros below can be used in any circumstances //////////////////////////////////// 
#define SetPORTBPin3() asm volatile ( "sbi 5,3")   // @portB = 5 
#define ClearPORTBPin3() asm volatile ( "cbi 5,3")

#define SetPORTBPin4() asm volatile ( "sbi 5,4")
#define ClearPORTBPin4() asm volatile ( "cbi 5,4")

#define SetPORTBPin5() asm volatile ( "sbi 5,5")
#define ClearPORTBPin5() asm volatile ( "cbi 5,5")


#define SetlatchPin() asm volatile ( "sbi 11,4") // @portD = 11 ou 0xB
#define ClearlatchPin() asm volatile ( "Cbi 11,4")

#define SetClockPin() asm volatile ( "sbi 11,7") // @portD = 11 ou 0xB
#define ClearClockPin() asm volatile ( "Cbi 11,7")

#define SetDataPin() asm volatile ( "sbi 5,0") // @portD = 5 
#define ClearDataPin() asm volatile ( "Cbi 5,0")
///////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////// a small delay /////////////////////////////////////////
#define microDelay() asm volatile ( "nop;nop;nop")


extern unsigned char disbuff[4];
extern signed char dotPos; // 0..3 pour allumer le point sur un digit, -1 pour aucun point

extern void display(void);
extern void initOutPutInput(void);
extern void shiftOut(uint8_t command);

#endif /* BITFACILITIES_H_ */
