#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "keyboard-driver.h"
#include "keyboard-interrupt.h"

static const char* path = "/dev/keyboard";

int main(void){
	int fd,err;
	char key[2];
	
	printf("Trying to use keyboard, opening device\n");	

	fd = open(path,O_RDONLY);	//Try to open device
		
	if (fd < 0) return -1;
	
	printf("Device opened succesfully"\n);
	printf("Configuring device...\n");	
	
	/* Reset device */
	err = ioctl(fd,IO_KEYBOARD_RESET);
	if (err < 0) return -1;
	printf("Reset device...\n");	
	
	/* Configure device */
	err = ioctl(fd,IO_KEYBOARD_CONFIG);
	if (err < 0) return -1;
	printf("Device configured\n");	
	
	/* Read from device */
	err = read(fd,key,1);	//Read 1 byte
	if (err < 0) return -1;
	
	key[1] = '\0';		//Convert into string
	printf("Key pressed: %s\n",key);
	
	return 0;	
}
