
.PHONY: all clean distclean

ATASM ?= atasm
#ATASMFLAGS=-s
#ATASMFLAGS=-v -s

HISIOSRC=hisio.inc hisiocode.src \
	hisiocode-break.src hisiocode-cleanup.src hisiocode-main.src \
	hisiocode-send.src hisiocode-check.src \
	hisiocode-receive.src hisiocode-vbi.src

all: autoboot.bin autoboot-hispeed.bin
all: autoboot.bin

clean:
	rm -f autoboot.bin autoboot-hispeed.bin

autoboot.bin: autoboot.src $(HISIOSRC);
	$(ATASM) $(ATASMFLAGS) -r -o$@ $<

autoboot-hispeed.bin: autoboot.src $(HISIOSRC);
	$(ATASM) $(ATASMFLAGS) -dAUTOBOOT_HISPEED -r -o$@ $<

autoboot.o: autoboot.s
	ca65 --target atari -o $@ $<

fastsio.o: fastsio.s
	ca65 --target atari -o $@ $<
