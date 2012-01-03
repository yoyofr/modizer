CC=gcc
CPP=g++
LD=$(CPP)

CFLAGS=-DLINUX -I./VBA -DVERSION_STR=\"0.04\" -DHA_VERSION_STR=\"0.11\" -I./libresample-0.1.3/include
LDFLAGS=-lz -lresample -L./libresample-0.1.3

OBJS=gsf.o VBA/GBA.o VBA/Globals.o VBA/Sound.o VBA/Util.o VBA/bios.o VBA/memgzio.o VBA/snd_interp.o VBA/unzip.o linuxmain.o VBA/psftag.o

all: $(OBJS)
	$(LD) $(LDFLAGS) *.o -lresample -o playgsf

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

%.o: %.c
	$(CC) $(CFLAGS) -c $<

%.o: %.cpp
	$(CPP) $(CFLAGS) -c $<

%.o: %.cpp %.h
	$(CPP) $(CFLAGS) -c $<


clean:
	rm -f *.o VBA/*.o playgsf
