// prikolchik 2013
// http://pastebin.com/Hp2pWeyL

// Tested on ASUS UX32VD
// DO NOT TRY ON ANY OTHER COMPUTER UNLESS YOU KNOW WHAT YOU ARE DOING!
//
// code taken from http://www.aneas.org/knowledge/asus_f3jp_fan_control.php
// and modified for UX32VD

// File: fancntrl.c
// Compile: gcc fancntrl.c -o fanctrl
// Usage: sudo ./fanctrl <fan speed>
//      where <fan speed> is fan speed from 0x00 (off) to 0xFF (MAX)

// WARNING!!! This file is proof of concept and is an UGLY hack!


#include <stdio.h> // printf
#include <stdlib.h> // atoi
#include <stdint.h> // uint8_t, uint16_t
#include <string.h> // strcmp
#include <unistd.h> // usleep
#include <sys/io.h> // inb, outb

#define DEBUG

// IO ports
const uint16_t EC4D = 0x0257; // data register
const uint16_t EC4C = 0x0258; // command register

#define ERROR16 (0xFFFF)

// waits for the status bit to clear, max ERROR16 tries
uint8_t WEIE() {
#ifdef DEBUG
	printf("Called : WEIE\n");
#endif
	uint16_t Local0 = 0xFFFF;
	uint8_t Local1 = inb(EC4C) & 0x02;
	while(Local0 != 0 && Local1 == 0x02) {
		Local1 = inb(EC4C) & 0x02;
		Local0--;
		usleep(5*1000); //sleep 5 msec
	}
#ifdef DEBUG
	printf("Left : WEIE\n");
#endif

	return (Local0 == 0);
}

// waits for the status bit to clear, max ERROR16 tries
uint8_t WEOE () {
#ifdef DEBUG
	printf("Called : WEOE\n");
#endif
	uint16_t Local0 = 0xFFFF;
	uint8_t Local1 = inb(EC4C) & 0x01;
	while(Local0 != 0 && Local1 == 0x01) {
		Local1 = inb(EC4C) & 0x01;
		Local0--;
		usleep(5*1000); //sleep 5 msec
		inb(EC4D); // maybe not needed?
	}
#ifdef DEBUG
	printf("Left : WEOE\n");
#endif

	return (Local0 == 0);
}

uint8_t WEOF() {
#ifdef DEBUG
	printf("Called : WEOF\n");
#endif
	uint16_t Local0 = 0xFFFF;
	uint8_t Local1 = inb(EC4C) & 0x01;
	while(Local0 != 0 && Local1 == 0x01) {
		Local1 = inb(EC4C) & 0x01;
		Local0--;
		usleep(5*1000); //sleep 5 msec
	}
#ifdef DEBUG
	printf("Left : WEOF\n");
#endif

	return (Local0 == 0);
}
/* read from RAM */
uint16_t RRAM (uint16_t Arg0) {
	uint16_t Local0, Local1;
	Local0 = Arg0;
	Local1 = Local0 & 0xFF;
	Local0 = (Local0 >> 0x08);
	Local0 = Local0 & 0xFF;

	if (WEOE() != 0){ return ERROR16; }
	if (WEIE() != 0){ return ERROR16; }
	outb(0xFF, EC4C);
	if (WEIE() != 0){ return ERROR16; }
	outb(0x80, EC4C);
	if (WEIE() != 0){ return ERROR16; }
	outb(Local0, EC4D);
	if (WEIE() != 0){ return ERROR16; }
	outb(Local1, EC4D);
	if (WEIE() != 0){ return ERROR16; }
	if (WEOF() != 0){ return ERROR16; }
	
	return inb(EC4D);
}

/* write to RAM */
uint16_t WRAM (uint16_t Arg0, uint8_t Arg1) {
	uint16_t Local0, Local1;
	Local0 = Arg0;
	Local1 = Local0 & 0xFF;
	Local0 = (Local0 >> 0x08);
	Local0 = Local0 & 0xFF;

	if (WEOE() != 0){ return ERROR16; }
	if (WEIE() != 0){ return ERROR16; }
	outb(0xFF, EC4C);
	if (WEIE() != 0){ return ERROR16; }
	outb(0x81, EC4C);
	if (WEIE() != 0){ return ERROR16; }
	outb(Local0, EC4D);
	if (WEIE() != 0){ return ERROR16; }
	outb(Local1, EC4D);
	if (WEIE() != 0){ return ERROR16; }
	outb(Arg1, EC4D);
	if (WEIE() != 0){ return ERROR16; }

	return 0;
}

// sets the QUIET fan speed
uint16_t WMFN (uint8_t Arg0) { //ST98
#ifdef DEBUG
	printf("Called : WMFN 0x%X\n", Arg0);
#endif
	if (WEOE() != 0){ return ERROR16; }
	if (WEIE() != 0){ return ERROR16; }
	outb(0xFF, EC4C);
	if (WEIE() != 0){ return ERROR16; }
	outb(0x98, EC4C);
	if (WEIE() != 0){ return ERROR16; }
	outb(Arg0, EC4D);
	if (WEIE() != 0){ return ERROR16; }
	if (WEIE() != 0){ return ERROR16; } //not needed?
#ifdef DEBUG
	printf("Left : WMFN\n");
#endif
	
	return 0;
}

/*
ST83 Arg0 Arg1
	Read speed of the fan(s). DOES NOT WORK IN MANUAL MODE
  Args:
	Arg0 	- 0 = fan1 
		- 1 = fan2
  Returns:
		- fan speed 0x0(OFF) - 0xFF(MAX)
*/
uint16_t RFOV (uint8_t Arg0) { //ST83
#ifdef DEBUG
	printf("Called : RFOV 0x%X\n", Arg0);
#endif
	if (Arg0 > 0x01) {
		return ERROR16;
	}
	if (WEOE() != 0){ return ERROR16; }
	if (WEIE() != 0){ return ERROR16; }
	outb(0xFF, EC4C);
	if (WEIE() != 0){ return ERROR16; }
	outb(0x83, EC4C);
	if (WEIE() != 0){ return ERROR16; }
	outb(Arg0, EC4D);
	if (WEIE() != 0){ return ERROR16; }
	if (WEOF() != 0){ return ERROR16; }
#ifdef DEBUG
	printf("Left : RFOV\n");
#endif

	return inb(EC4D);
}

/*
ST84 Arg0 Arg1
	Sets speed of the fan(s)
  Args:
	Arg0 	- 0 = fan1 
		- 1 = fan2
	Arg1	- fan speed 0x0(OFF) - 0xFF(MAX)
*/

uint16_t WFOV (uint8_t Arg0, uint8_t Arg1) { //ST84
	if (Arg0 > 0x01) {
		return ERROR16;
	}
	if (WEOE() != 0){ return ERROR16; }
	if (WEIE() != 0){ return ERROR16; }
	outb(0xFF, EC4C);
	if (WEIE() != 0){ return ERROR16; }
	outb(0x84, EC4C);
	if (WEIE() != 0){ return ERROR16; }
	outb(Arg0, EC4D);
	if (WEIE() != 0){ return ERROR16; }
	outb(Arg1, EC4D);
	if (WEIE() != 0){ return ERROR16; }
	
	return 0;
}

/*
SFNV Arg0 Arg1
	Sets speed of the fan(s)
  Args:
	Arg0 	- 0 to reset fans to AUTO 
		- 1 = fan1
		- 2 = fan2
	Arg1 	- if Arg0 == 0, then 0x1 bit reset fan1 0x2 bit reset fan2
		- fan speed 0x0(OFF) - 0xFF(MAX)
*/
#define SFNV_SET_FAN1 (0x01)
#define SFNV_SET_FAN2 (0x02)
#define SFNV_AUTO_FAN1 (0x01)
#define SFNV_AUTO_FAN2 (0x02)
#define SFNV_AUTO_ALL (SFNV_AUTO_FAN1 | SFNV_AUTO_FAN2)

uint16_t SFNV (uint8_t Arg0, uint8_t Arg1) {
	uint16_t Local0;
	// Set the fans to automatic
	if (Arg0 == 0) {
		// set fan1 to AUTO
		if (Arg1 & SFNV_AUTO_FAN1) {
			if ((Local0 = RRAM(0x0521)) == ERROR16) {return ERROR16; };
			Local0 |= 0x80;
			WRAM (0x0521, (uint8_t) (0xFF & Local0)); //ignore error?
		}
		// set fan2 to AUTO
		if (Arg1 & SFNV_AUTO_FAN2) {
			if ((Local0 = RRAM(0x0522)) == ERROR16) {return ERROR16; };
			Local0 |= 0x80;
			WRAM (0x0522, (uint8_t) (0xFF & Local0)); //ignore error?
		}
		return 0;
	}
	// Set the speed of fan1
	else if (Arg0 == SFNV_SET_FAN1) {
		if ((Local0 = RRAM(0x0521)) == ERROR16) {return ERROR16; };
		Local0 &= 0x7F;
		WRAM (0x0521, (uint8_t) (0xFF & Local0)); //ignore error?
		// DECF |= 0x1
		return WFOV (0x00, Arg1);
	}
	// Set the speed of fan2
	else if (Arg0 == SFNV_SET_FAN2) {
		if ((Local0 = RRAM(0x0522) == ERROR16)) {return ERROR16; };
		Local0 &= 0x7F;
		WRAM (0x0522, (uint8_t) (0xFF & Local0)); //ignore error?
		// DECF |= 0x2
		return WFOV (0x01, Arg1);
	}
	return ERROR16;
} 

int main(int argc, char ** argv) {
	if(argc != 2) {
		printf("usage: %s speed\n", argv[0]);
		printf("speed: `auto' or a value between 1 and 15\n");
		printf("keep in mind that `auto' will be even faster than 15!\n");
		return 1;
	}

	uint8_t speed = 0xFF;
	if(strcmp(argv[1], "auto") == 0)
		printf("setting speed to 'auto'\n");
	else {
		int arg = atoi(argv[1]);
		if(arg < 1 || arg > 255) {
			printf("Error: the speed %d is not possible\n", arg);
			return 1;
		}
		printf("setting speed to %d\n", arg);
		speed = arg;
	}

	if(ioperm(EC4D, 1, 1)) {
		printf("Error: could not gain access to IO port EC4D (0x%X). Are you root?\n", EC4D);
		return 1;
	}

	if(ioperm(EC4C, 1, 1)) {
		printf("Error: could not gain access to IO port EC4C (0x%X). Are you root?\n", EC4C);
		return 1;
	}

#if 0
	printf("Fan speeds: \tFan1: %d, Fan2: %d\n", RFOV(0x00), RFOV(0x01));
#endif
	if (WMFN(speed)) {
		printf("error\n");
	}
	else {
		printf("good\n");
	}

	// Set FAN1 speed to 0xFF (MAX)
//	SFNV(0x1, 0xFF);
	// Set FAN2 speed to 0x00 (OFF)
//	SFNV(0x2, 0x00);
	// reset both fans back to AUTO control
//	SFNV(0x0, 0x01|0x02);

	return 0;
}
