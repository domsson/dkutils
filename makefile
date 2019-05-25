CC = gcc
CFLAGS = -O2 -c -Wall
LINK = gcc
LFLAGS =
LIBS = -lslang -lpc

all: script.exe rsfx.exe cfg.exe xsfx.exe xgfx.exe dklevel.exe \
	dktext.exe terrain.exe exanim.exe dernc.exe rnc.exe

kit: utils.zip utilsrc.zip
	
utils.zip: all
	zip utils *.exe readme.*

utilsrc.zip: all
	zip utilsrc *.exe *.h *.c Makefile readme.txt
	
.c.o:
	$(CC) $(CFLAGS) $*.c

rnc.exe: rnc.o dernc_c.o
	$(LINK) -o rnc rnc.o dernc_c.o
	strip rnc
	coff2exe rnc

dernc.exe: dernc.o
	$(LINK) -o dernc dernc.o
	strip dernc
	coff2exe dernc

dernc.o: dernc.c
	$(CC) $(CFLAGS) -c -DMAIN -o dernc.o dernc.c

dernc_c.o: dernc.c
	$(CC) $(CFLAGS) -c -DCOMPRESSOR -o dernc_c.o dernc.c

rnc.o: rnc.c
script.o: script.c script.h
cfg.o:  cfg.c cfg.h
rsfx.o: rsfx.c
dktext.o: dktext.c
dklevel.o: dklevel.c
xsfx.o: xsfx.c
xgfx.o: xgfx.c
terrain.o: terrain.c
exanim.o: exanim.c

xsfx.exe: xsfx.o
	$(LINK) -o xsfx xsfx.o
	strip xsfx
	coff2exe xsfx

exanim.exe: exanim.o
	$(LINK) -o exanim exanim.o
	strip exanim
	coff2exe exanim

terrain.exe: terrain.o
	$(LINK) -o terrain terrain.o
	strip terrain
	coff2exe terrain

xgfx.exe: xgfx.o
	$(LINK) -o xgfx xgfx.o
	strip xgfx
	coff2exe xgfx

script.exe: script.o
	 $(LINK) -o script script.o $(LIBS)
	 strip script
	 coff2exe script

rsfx.exe: rsfx.o
	$(LINK) -o rsfx rsfx.o
	strip rsfx
	coff2exe rsfx

dktext.exe: dktext.o
	$(LINK) -o dktext dktext.o
	strip dktext
	coff2exe dktext

cfg.exe: cfg.o
	$(LINK) -o cfg cfg.o $(LIBS)
	strip cfg
	coff2exe cfg

dklevel.exe: dklevel.o
	$(LINK) -o dklevel dklevel.o
	strip dklevel
	coff2exe dklevel

clean:
	del *.o script dklevel cfg rsfx xgfx xsfx \
	exanim terain dktext *.exe *.zip dernc rnc
