# caution: this makefile does NOT check for changes in header files! i.e. the "clean" target may need to be invoked manually

# for debugging it may be usefull to add these to gcc/g++: -g -rdynamic -funwind-tables
CC = gcc -fnon-call-exceptions
CXX = g++ -fnon-call-exceptions -std=c++11

#see https://gcc.gnu.org/onlinedocs/gcc/Option-Summary.html
#  -Wformat=2 -Wextra -Winline -pipe -fPIC
CFLAGS =  -DRPI4 -Wno-pointer-sign -Wall -c  
CXXFLAGS = -DRPI4 -fno-rtti -Wall -D__STDC_LIMIT_MACROS

# -OFast
LDFLAGS = -funroll-loops -O2

SRCDIR=../src
RSRCDIR=./src
WSRCDIR=./wiringPi

OBJDIR = ./obj
CCOBJS = $(OBJDIR)/cia.o $(OBJDIR)/cpu.o $(OBJDIR)/hacks.o $(OBJDIR)/memory.o $(OBJDIR)/vic.o  $(OBJDIR)/wiringPi.o 
CXXOBJS = $(OBJDIR)/core.o $(OBJDIR)/digi.o $(OBJDIR)/envelope.o $(OBJDIR)/filter.o $(OBJDIR)/loaders.o $(OBJDIR)/sid.o $(OBJDIR)/system.o $(OBJDIR)/wavegenerator.o $(OBJDIR)/sidplayer.o 
CXXROBJS = $(OBJDIR)/main.o $(OBJDIR)/rpi4_utils.o $(OBJDIR)/gpio_sid.o $(OBJDIR)/cp1252.o $(OBJDIR)/playback_handler.o $(OBJDIR)/device_driver_handler.o $(OBJDIR)/fallback_handler.o
	

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS)

$(OBJDIR)/%.o: $(RSRCDIR)/%.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS)

$(OBJDIR)/%.o: $(WSRCDIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

all: websid

websid: $(CCOBJS) $(CXXOBJS) $(CXXROBJS)
		 g++ $(LDFLAGS)  $(CCOBJS) $(CXXOBJS) $(CXXROBJS)                     -lm -lpthread -o websid
# in order to use the original wiringPi lib you need to add    -lwiringPi     in the gap in line above, 
# and you have to add -DUSE_ORIG_WIRINGPI to the CFLAGS and remove $(OBJDIR)//wiringPi.o from CCOBJS

clean:
	rm -f $(OBJDIR)/*.o
	rm -f websid