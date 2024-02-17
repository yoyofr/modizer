
CC = gcc -fnon-call-exceptions
CFLAGS =  -Wno-pointer-sign -Wall -c  
LDFLAGS = -funroll-loops -Ofast


all: cpufan

main.o: main.c
	$(CC) -c -o $@ $< $(CFLAGS)

cpufan: main.o
	 $(CC) $(LDFLAGS) -o cpufan  main.o  -lm -lpthread -lgpiod 


clean:
	rm -f main.o
	rm -f fan
