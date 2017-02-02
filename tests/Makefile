# export the environment variable LINARO properly set

BIN = test-keyboard
LINARO = /home/david/Ingenieria/herramientas/compiladores/gcc-linaro-arm-linux-gnueabihf
CC = $(LINARO)/bin/arm-linux-gnueabihf-gcc
INC = -I$(LINARO)/arm-linux-gnueabihf/libc/usr/include -I/home/david/Ingenieria/Cuarto/Se2/TP6/Trabajo_Propio/kernel-rt/kernel/include/
LIBS = -static -L$(CC)/arm-linux-gnueabihf/libc/lib/arm-linux-gnueabihf -L$(CC)/arm-linux-gnueabihf/libc/usr/lib/arm-linux-gnueabihf

test-keyboard: test-keyboard.c
		$(CC) $(CFLAGS) $(INC) $(LIBS) -o $(BIN) test-keyboard.c
clean:
	rm $(BIN)
