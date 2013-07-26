// Alexander Breckel 2008
// http://www.aneas.org/knowledge/asus_f3jp_fan_control.php

#include <stdio.h> // printf
#include <stdlib.h> // atoi
#include <stdint.h> // uint8_t, uint16_t
#include <string.h> // strcmp
#include <sys/io.h> // inb, outb

// IO ports
const uint16_t AEIC = 0x025D; // command register
const uint16_t AEID = 0x025C; // data register

// waits for the status bit to clear, max 0x4000 tries
void WEIE() {
	uint16_t Local0 = 0x4000;
	uint8_t Local1 = inb(AEIC) & 0x02;
	while(Local0 != 0 && Local1 == 0x02) {
		Local1 = inb(AEIC) & 0x02;
		Local0--;
	}
}

// sets the fan speed
void WMFN(uint8_t Arg0) {
	WEIE();
	outb(0x98, AEIC);
	WEIE();
	outb(Arg0, AEID);
	WEIE();
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
		if(arg < 1 || arg > 15) {
			printf("Error: the speed %d is not possible\n", arg);
			return 1;
		}
		printf("setting speed to %d\n", arg);
		speed = (arg << 3) | 0x07;
	}

	if(ioperm(AEID, 1, 1)) {
		printf("Error: could not gain access to IO port AEID (0x025C)\n");
		return 1;
	}

	if(ioperm(AEIC, 1, 1)) {
		printf("Error: could not gain access to IO port AEIC (0x025D)\n");
		return 1;
	}

	WMFN(speed);

	printf("done.\n");
	return 0;
}
