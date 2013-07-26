/* Nicolai Rostov 2013
 *
 * Tested on Asus UX31A
 * DO NOT TRY ON ANY OTHER COMPUTER UNLESS YOU KNOW WHAT YOU ARE DOING!
 *
 * Based on code taken from:
 *  http://www.aneas.org/knowledge/asus_f3jp_fan_control.php
 *  http://pastebin.com/Hp2pWeyL
 * and modified for UX31A.
 *
 * File:    ux31a.c
 * Compile: gcc ux31a.c -o fanctrl
 * Usage:   fanctrl <speed>
 *      
 *  Where <speed> is auto or any value from 1 to 255
 *
 */

#include <stdio.h> // printf
#include <stdlib.h> // atoi
#include <stdint.h> // uint8_t, uint16_t
#include <string.h> // strcmp
#include <sys/io.h> // inb, outb


// IO ports
const uint16_t EC4C = 0x0258; // command register
const uint16_t EC4D = 0x0257; // data register

#define ERROR16 (0xFFFF)

// waits for the status bit to clear, max 0x4000 tries
uint8_t WEIE() {
	uint16_t Local0 = 0x4000;
	uint8_t Local1 = inb(EC4C) & 0x02;
	while(Local0 != 0 && Local1 == 0x02) {
		Local1 = inb(EC4C) & 0x02;
		Local0--;
		usleep(5*1000); //sleep 5 msec
	}
	return (Local0 == 0);
}

uint8_t WEOE () {
	uint16_t Local0 = 0xFFFF;
	uint8_t Local1 = inb(EC4C) & 0x01;
	while(Local0 != 0 && Local1 == 0x01) {
		Local1 = inb(EC4C) & 0x01;
		Local0--;
		usleep(5*1000); //sleep 5 msec
		inb(EC4D); // maybe not needed?
	}

	return (Local0 == 0);
}


// sets the fan speed
uint16_t WMFN (uint8_t Arg0) { //ST98
	if (WEOE() != 0){ return ERROR16; }
	if (WEIE() != 0){ return ERROR16; }
	outb(0xFF, EC4C);
	if (WEIE() != 0){ return ERROR16; }
	outb(0x98, EC4C);
	if (WEIE() != 0){ return ERROR16; }
	outb(Arg0, EC4D);
	if (WEIE() != 0){ return ERROR16; }
	if (WEIE() != 0){ return ERROR16; } //not needed?
	return 0;
}

int main(int argc, char ** argv) {
	if(argc != 2) {
		printf("Usage: %s {auto|1..255}\n", argv[0]);
		return 1;
	}

	uint8_t speed = 0xFF;
	uint8_t level = 0xFF;
	uint8_t max_speed = 0xFF;
	if(strcmp(argv[1], "auto") == 0)
		printf("speed: auto\n");
	else {
		int arg = atoi(argv[1]);
		if(arg < 1 || arg > max_speed) {
			printf("Error: the speed %d is not possible\n", arg);
			return 1;
		}
		speed = arg;
		level = (speed | 0x07) >> 3;
		printf("speed: %d (0x%x) [level %d]\n", speed, speed, level);
	}

	if(ioperm(EC4D, 1, 1)) {
		printf("Error: could not gain access to IO port EC4D (0x%X). Are you root?\n", EC4D);
		return 1;
	}

	if(ioperm(EC4C, 1, 1)) {
		printf("Error: could not gain access to IO port EC4C (0x%X). Are you root?\n", EC4C);
		return 1;
	}

	WMFN(speed);
	return 0;
}
