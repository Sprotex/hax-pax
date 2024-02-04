
#CC = gcc
#CFLAGS = -Wall -pedantic -O2 -fomit-frame-pointer
#LIBS = -lX11

CC = arm-none-eabi-gcc
CFLAGS = -Wall -fPIC -I /usr/arm-linux-gnueabi/include/
LIBS =  /usr/arm-linux-gnueabi/lib/

all: zxem

rebuild: 
	make clean
	make all

z80tables.h: maketables.c
	$(CC) -Wall $< -o maketables
	./maketables > $@

z80emu.o: z80emu.c z80emu.h z80config.h z80user.h \
	z80instructions.h z80macros.h z80tables.h 
	$(CC) $(CFLAGS) -c $<

OBJECT_FILES = zxem.o z80emu.o 
#OBJECT_FILES += x11.o
OBJECT_FILES += pax.o gui.o

zxem: $(OBJECT_FILES)
#	$(CC) $(OBJECT_FILES) -o $@ $(LIBS)
	$(CC) $(CFLAGS) -shared -fPIC -o $@ $(OBJECT_FILES) -nostartfiles -static


clean:
	rm -f *.o zxem maketables

upload: zxem
	. env/bin/activate; python3 ../prolin-xcb-client/client.py ACM0 push zxem /data/app/MAINAPP/lib/libosal.so
	. env/bin/activate; python3 ../prolin-xcb-client/client.py ACM0 push zx48.rom /data/app/MAINAPP/zx48.rom
	. env/bin/activate; python3 ../prolin-xcb-client/client.py ACM0 push manic.sna /data/app/MAINAPP/manic.sna

# OLD, deprecated
push: zxem
	python3 ../../../prolin-xcb-client/client.py ACM0 push zxem /data/app/MAINAPP/lib/libosal.so
	python3 ../../../prolin-xcb-client/client.py ACM0 push zx48.rom /data/app/MAINAPP/zx48.rom
	python3 ../../../prolin-xcb-client/client.py ACM0 push manic.sna /data/app/MAINAPP/manic.sna
