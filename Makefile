CODE_DIRECTORIES = src src/*
INCLUDE_DIRECTORIES = src /usr/arm-linux-gnueabi/include/
LIBS = /usr/arm-linux-gnueabi/lib/
SOURCE_FILES = $(foreach directory,$(CODE_DIRECTORIES),$(wildcard $(directory)/*.c))
OBJECT_FILES = $(patsubst %.c,%.o,$(SOURCE_FILES))
DEPENDENCY_FILES = $(patsubst %.c,%.d,$(SOURCE_FILES))
ROMS = $(foreach directory,snapshots-roms,$(wildcard $(directory)/*))
OUTPUT_FOLDER = bin

CC = arm-none-eabi-gcc
CFLAGS = -Wall -fPIC $(foreach DIRECTORY,$(INCLUDE_DIRECTORIES),-I$(DIRECTORY)) -nostartfiles
DEPENDENCY_FLAGS = -MMD -MP
UPLOAD_COMMON_PATH = /data/app/MAINAPP
PYTHON_UPLOADER_PATH = ../prolin-xcb-client/client.py

PUSH_COMMAND = python3 $(PYTHON_UPLOADER_PATH) ACM0 push

.PHONY: all clean generic-upload loader-upload upload-roms
.SECONDEXPANSION:

all: zxem pmi telnet

rebuild: clean all

print-%:
	@echo "$* = $($*)"

clean:
	rm -f $(OBJECT_FILES) $(DEPENDENCY_FILES)

z80tables.h: maketables.c
	$(CC) -Wall $< -o maketables $(DEPENDENCY_FLAGS)
	./maketables > $@

%.o: %.c
	$(CC) $(CFLAGS) $(DEPENDENCY_FLAGS) -c -o $@ $<

generic-upload:
	. env/bin/activate; $(PUSH_COMMAND) $(TARGET) $(UPLOAD_COMMON_PATH)/$(TARGET)

loader-upload:
	. env/bin/activate; $(PUSH_COMMAND) $(OUTPUT_FOLDER)/$(TARGET) $(UPLOAD_COMMON_PATH)/lib/libosal.so

compile-and-upload-loader:
	$(CC) $(CFLAGS) $(DEPENDENCY_FLAGS) -shared -o $(OUTPUT_FOLDER)/$(TARGET) $(SOURCE)
	$(MAKE) loader-upload TARGET=$(TARGET)

zxem: src/zxem.o src/z80emu.o src/pax.o src/gui.o
	$(MAKE) compile-and-upload-loader TARGET=$@ SOURCE="$^"

pmi: src/pmi/*.o
	$(MAKE) compile-and-upload-loader TARGET=$@ SOURCE="$^"

telnet: src/telnet/telnet.o
	$(MAKE) compile-and-upload-loader TARGET=$@ SOURCE="$^"

upload-roms:
	for rom in $(ROMS); do $(MAKE) generic-upload TARGET=$$rom; done

# obsolete, left here for compatibility reasons
push: zxem
	$(MAKE) upload-roms
