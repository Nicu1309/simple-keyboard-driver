#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>

#include "../drivers/keyboard-driver.h"

static const char* path = "/dev/simple-keyboard";

int main(void){
	int fd,err;
	char key[2];

	printf("Trying to use keyboard, opening device\n");

	fd = open(path,O_RDWR);	//Try to open device

	if (fd < 0) {
			printf("ERROR WHILE OPENING DEVICE!!!\n");
			return -1;
	}

	printf("Device opened succesfully\n");
	printf("Configuring device...\n");

	/* Reset device */
/*	err = ioctl(fd,IO_KEYBOARD_RESET);
	if (fd < 0) {
			printf("ERROR WHILE RESETING DEVICE!!!\n");
			return -1;
	}
	printf("Reset device...\n");*/

	/* Configure device */
	err = ioctl(fd,IO_KEYBOARD_CONFIG);
	if (fd < 0) {
			printf("ERROR WHILE CONFIGURING DEVICE!!!\n");
			return -1;
	}
	printf("Device configured\n");

	/* Read from device */
	err = read(fd,key,1);	//Read 1 byte
	if (fd < 0) {
			printf("ERROR WHILE READING DEVICE!!!\n");
			return -1;
	}

	key[1] = '\0';		//Convert into string
	printf("Key pressed: %s\n",key);

	return 0;
}
