#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>

#include "../drivers/keyboard-public.h"

static const char* path = "/dev/simple-keyboard";
static struct pin_conf custom_pinmux = {
	.irq_pin = 932,
	.vcc_pin = 911,
	.right_key_pin = 912,
	.start_key_pin = 913,
	.up_key_pin = 914,
	.down_key_pin = 917,
	.escape_key_pin = 925,
	.left_key_pin = 927,
  };

int main(int argc, char *argv[]){
	int fd,err;
	char key;
	unsigned long cmd;

	if (argc < 2){
		printf("Not enough arguments\n");
		return -1;
	}

	printf("Trying to use keyboard, opening device\n");

	fd = open(path,O_RDWR);	//Try to open device

	if (fd < 0) {
			printf("ERROR WHILE OPENING DEVICE!!!\n");
			return -1;
	}

	printf("Device opened succesfully\n");
	printf("Configuring device...\n");

	/* Check irqs mode */
	if (strcmp(argv[1],"s") == 0){
		cmd = IO_KEYBOARD_CONFIG_SINGLE_LINE;
		printf("Single line configured\n");
	} else {
		cmd = IO_KEYBOARD_CONFIG_MULTI_LINE;
		printf("Multi line configured\n");
	}

	/* Reset device */
	err = ioctl(fd,IO_KEYBOARD_RESET);
	if (fd < 0) {
			printf("ERROR WHILE RESETING DEVICE!!!\n");
			return -1;
	}
	printf("Reset device\n");

	/* Configure custom pinmuxing  */
	err = ioctl(fd,IO_KEYBOARD_CONFIG_PINMUX,&custom_pinmux);
	if (fd < 0) {
			printf("ERROR WHILE CUSTOMIZING PINMUX!!!\n");
			return -1;
	}
	printf("Custom pinmux configured\n");

	/* Configure device */
	err = ioctl(fd,cmd);
	if (fd < 0) {
			printf("ERROR WHILE CONFIGURING DEVICE!!!\n");
			return -1;
	}
	printf("Device configured\n");

	/* Read from device */
	err = read(fd,&key,sizeof(char));	//Read 1 byte
	if (fd < 0) {
			printf("ERROR WHILE READING DEVICE!!!\n");
			return -1;
	}

	printf("Key pressed: %c\n",key);

	return 0;
}
