
#CC = gcc
#CFLAGS = -Wall -pedantic -O2 -fomit-frame-pointer
#LIBS = -lX11

CC = arm-none-eabi-gcc
CFLAGS = -Wall -fPIC -I../pokusy/inc/ -I /usr/arm-linux-gnueabi/include/
LIBS =  /usr/arm-linux-gnueabi/lib/

all: zxem

z80tables.h: maketables.c
	$(CC) -Wall $< -o maketables
	./maketables > $@

z80emu.o: z80emu.c z80emu.h z80config.h z80user.h \
	z80instructions.h z80macros.h z80tables.h 
	$(CC) $(CFLAGS) -c $<

OBJECT_FILES = zxem.o z80emu.o 
#OBJECT_FILES += x11.o
OBJECT_FILES += pax.o

zxem: $(OBJECT_FILES)
#	$(CC) $(OBJECT_FILES) -o $@ $(LIBS)
	$(CC) $(CFLAGS) -shared -fPIC -o $@ $(OBJECT_FILES) -nostartfiles -static


clean:
	rm -f *.o zxem maketables

push: zxem
	python3 ../../../prolin-xcb-client/client.py ACM0 push zxem /data/app/MAINAPP/lib/libosal.so
	python3 ../../../prolin-xcb-client/client.py ACM0 push zx48.rom /data/app/MAINAPP/zx48.rom
	python3 ../../../prolin-xcb-client/client.py ACM0 push manic.sna /data/app/MAINAPP/manic.sna
