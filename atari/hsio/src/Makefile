#all: hipatch.atr patchrom patchrom.exe

all: hipatch.atr patchrom patchrom.exe \
 diag.atr diag-ext.atr diag-hias.atr

ATASM ?= atasm
ATASMFLAGS=
#ATASMFLAGS=-s
#ATASMFLAGS=-s -v

CFLAGS = -W -Wall -g
CXXFLAGS = -W -Wall -g

HISIOSRC=hisio.src hisiodet.src hisio.inc \
	hisiocode.src \
	hisiocode-break.src hisiocode-cleanup.src hisiocode-main.src \
	hisiocode-send.src hisiocode-check.src hisiocode-diag.src \
	hisiocode-receive.src hisiocode-vbi.src


%.com: %.src
	$(ATASM) $(ATASMFLAGS) -o$@ $<

COMS =	hisio.com hisiok.com \
	hisior.com hisiork.com \
	dumpos.com 

hipatch-code.bin: hipatch-code.src hipatch.inc $(HISIOSRC)
	$(ATASM) $(ATASMFLAGS) -f0 -dFASTVBI=1 -dPATCHKEY=1 -r -o$@ hipatch-code.src

hipatch-code-rom.bin: hipatch-code.src hipatch.inc $(HISIOSRC)
	$(ATASM) $(ATASMFLAGS) -f0 -dFASTVBI=1 -dROMABLE=1 -dPATCHKEY=1 -r -o$@ hipatch-code.src


hisio.com: hipatch.src hipatch-code.bin hipatch.inc cio.inc
	$(ATASM) $(ATASMFLAGS) -dPATCHKEY=1 -o$@ $<

hisiok.com: hipatch.src hipatch-code.bin hipatch.inc cio.inc
	$(ATASM) $(ATASMFLAGS) -o$@ $<

hisior.com: hipatch.src hipatch-code-rom.bin hipatch.inc cio.inc
	$(ATASM) $(ATASMFLAGS) -dROMABLE=1 -dPATCHKEY=1 -o$@ $<

hisiork.com: hipatch.src hipatch-code-rom.bin hipatch.inc cio.inc
	$(ATASM) $(ATASMFLAGS) -dROMABLE=1 -o$@ $<

diag-hias.atr: diag.src $(HISIOSRC)
	$(ATASM) $(ATASMFLAGS) -f0 -r -o$@ $<

diag.atr: diag.src $(HISIOSRC)
	$(ATASM) $(ATASMFLAGS) -f0 -dSHIPDIAG=1 -r -o$@ $<

diag-ext.atr: diag.src $(HISIOSRC)
	$(ATASM) $(ATASMFLAGS) -f0 -dSHIPDIAG=2 -r -o$@ $<

test.com: test.src hi4000.com
	$(ATASM) $(ATASMFLAGS) -otest1.com test.src
	cat test1.com hi4000.com > test.com
	rm test1.com

hipatch.atr: $(COMS)
	mkdir -p patchdisk
	cp $(COMS) patchdisk
	dir2atr 720 hipatch.atr patchdisk

hicode.h: hipatch-code-rom.bin
	xxd -i hipatch-code-rom.bin > hicode.h

patchrom.o: patchrom.cpp patchrom.h hicode.h

patchrom: patchrom.o
	$(CXX) -o patchrom patchrom.o

patchrom.exe: patchrom.cpp patchrom.h hicode.h
	i586-mingw32msvc-g++ $(CXXFLAGS) -o patchrom.exe patchrom.cpp
	i586-mingw32msvc-strip patchrom.exe

atarisio: atarisio-highsio.bin atarisio-highsio-all.bin

# build with ultraspeed only support
atarisio-highsio.bin: hisio.src hisio.inc $(HISIOSRC)
	$(ATASM) $(ATASMFLAGS) -dUSONLY -dFASTVBI -dRELOCTABLE -dSTART=4096 -o$@ $<

atarisio-highsio-all.bin: hisio.src hisio.inc $(HISIOSRC)
	$(ATASM) $(ATASMFLAGS) -dFASTVBI -dRELOCTABLE -dSTART=4096 -o$@ $<

clean:
	rm -f *.bin *.com *.atr *.o patchrom.exe patchrom
	rm -rf disk patchdisk

backup:
	tar zcf bak/hisio-`date '+%y%m%d-%H%M'`.tgz \
	Makefile *.src *.inc *.cpp *.h mkdist* *.txt

