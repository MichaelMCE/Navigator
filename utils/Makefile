

CC=gcc
CP=g++


x64 := 1
ifeq ($(x64),1)
MDIR=x64
BITMODE=-m64
else
MDIR=x32
BITMODE=-m32
endif



CFLAGS = $(BITMODE) -D_GNU_SOURCE -Ilibmylcd/include -O2 -D_WIN32_WINNT=0x0601 -DWINVER=0x0601 -D__WIN32__=1 -std=gnu23 -Wall -march=native -mtune=native
# CPPFLAGS = $(BITMODE) -D_GNU_SOURCE -Ilibmylcd/include -O2 -D_WIN32_WINNT=0x0601 -DWINVER=0x0601 -D__WIN32__=1 -std=c++23 -Wall -march=native -mtune=native
LIBS = $(BITMODE) -D _GNU_SOURCE -Llibmylcd/libs/x64 -L"lib" -lm -lmylcddll -lole32 -luuid -lwininet

EXAMPLES=serial1_console.exe


# -static-libgcc -static-libstdc++

all : $(EXAMPLES)


serial1_console.exe: serial1_console.o
	$(CC) -static-libgcc -o $@ $^ $(LIBS)
	strip.exe $@

split.exe: split.o fileal.o
	$(CC) -static-libgcc -o $@ $^ $(LIBS)
	strip.exe $@

pack.exe: pack.o 
	$(CC) -static-libgcc -o $@ $^ $(LIBS)
	strip.exe $@

poigen.exe: poigen.o libmylcd/fileal.o
	$(CC) -static-libgcc -o $@ $^ $(LIBS)
	strip.exe $@

%.o: %.c $(DEPS) 
	$(CC) -c -o $@ $< $(CFLAGS)

%.o: %.cpp $(DEPS) 
	$(CP) -c -o $@ $< $(CPPFLAGS)

clean :
	rm -f *.exe *.o *.bak


### PHONY define
.PHONY: all all-before all-after clean clean-custom



