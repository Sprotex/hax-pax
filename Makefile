
#CC = gcc
#CFLAGS = -Wall -pedantic -O2 -fomit-frame-pointer
#LIBS = -lX11

CC = arm-none-eabi-gcc
CFLAGS = -Wall -fPIC -I /usr/arm-linux-gnueabi/include/
LIBS = /usr/arm-linux-gnueabi/lib/
UPLOAD_COMMON_PATH = /data/app/MAINAPP
PYTHON_UPLOADER_PATH = ../prolin-xcb-client/client.py
PUSH_COMMAND = python3 $(PYTHON_UPLOADER_PATH) ACM0 push

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
	$(CC) $(CFLAGS) -shared -fPIC -o $@ $(OBJECT_FILES) -nostartfiles

pmi:
	$(CC) $(CFLAGS) -o pmi pmi/pmi.c -nostartfiles -shared
	. env/bin/activate; $(PUSH_COMMAND) pmi $(UPLOAD_COMMON_PATH)/lib/libosal.so

transfer:
	$(CC) $(CFLAGS) -o transfer telnet-test/tcpsh2.c -nostartfiles -shared
	. env/bin/activate; $(PUSH_COMMAND) transfer $(UPLOAD_COMMON_PATH)/lib/libosal.so

clean:
	rm -f *.o zxem maketables transfer

upload: zxem
	. env/bin/activate; make push

push: zxem
	$(PUSH_COMMAND) zxem $(UPLOAD_COMMON_PATH)/lib/libosal.so
	$(PUSH_COMMAND) zx48.rom $(UPLOAD_COMMON_PATH)/zx48.rom
# $(PUSH_COMMAND) manic.sna $(UPLOAD_COMMON_PATH)/manic.sna
