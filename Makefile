CODE_DIRECTORIES = . pmi telnet zxem
INCLUDE_DIRECTORIES = . /usr/arm-linux-gnueabi/include/
LIBS = /usr/arm-linux-gnueabi/lib/
SOURCE_FILES = $(foreach directory,$(CODE_DIRECTORIES),$(wildcard $(directory)/*.c))
OBJECT_FILES = $(patsubst %.c,%.o,$(SOURCE_FILES))
DEPENDENCY_FILES = $(patsubst %.c,%.d,$(SOURCE_FILES))

CC = arm-none-eabi-gcc
CFLAGS = -Wall -fPIC $(foreach DIRECTORY,$(INCLUDE_DIRECTORIES),-I$(DIRECTORY)) -nostartfiles
DEPENDENCY_FLAGS = -MMD -MP
UPLOAD_COMMON_PATH = /data/app/MAINAPP
PYTHON_UPLOADER_PATH = ../prolin-xcb-client/client.py

PUSH_COMMAND = python3 $(PYTHON_UPLOADER_PATH) ACM0 push
OUTPUT_FOLDER = binaries

all: zxem telnet pmi

rebuild: clean all

echo:
	@echo $(SOURCE_FILES)
	@echo $(OBJECT_FILES)
	@echo $(DEPENDENCY_FILES)

clean:
	rm -f $(OBJECT_FILES) $(DEPENDENCY_FILES)

z80tables.h: maketables.c
	$(CC) -Wall $< -o maketables $(DEPENDENCY_FLAGS)
	./maketables > $@

%.o: %.c
	$(CC) $(CFLAGS) $(DEPENDENCY_FLAGS) -c -o $@ $<

zxem: $(OBJECT_FILES)
	$(CC) $(CFLAGS) $(DEPENDENCY_FLAGS) -shared -o $@ $^

compile-upload:
	$(CC) $(CFLAGS) -o $(OUTPUT_FOLDER)/$(TARGET) $(SOURCE) -shared
	. env/bin/activate; $(PUSH_COMMAND) $@ $(UPLOAD_COMMON_PATH)/lib/libosal.so

pmi: pmi.o
	. env/bin/activate; $(PUSH_COMMAND) $(OUTPUT_FOLDER)/pmi $(UPLOAD_COMMON_PATH)/lib/libosal.so

telnet: telnet/simple-telnet.c
	$(MAKE) compile-upload TARGET=$@ SOURCE=$^

upload: zxem
	. env/bin/activate; make push

push: zxem
	$(PUSH_COMMAND) zxem $(UPLOAD_COMMON_PATH)/lib/libosal.so
	$(PUSH_COMMAND) zx48.rom $(UPLOAD_COMMON_PATH)/zx48.rom
	$(PUSH_COMMAND) manic.sna $(UPLOAD_COMMON_PATH)/manic.sna

.PHONY: all clean upload push telnet rebuild
