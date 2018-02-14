# Makefile for cygwin gcc
# Nate Lawson <nate@rootlabs.com>

CC = i586-mingw32msvc-gcc
RC = i586-mingw32msvc-windres
PCAP_PATH = ./WpdPack
#CFLAGS = -O -Wall -ggdb -DWPCAP
CFLAGS = -O -Wall -ggdb

#OBJS = cdp.o getphonecdp.o getphonenet.o getphonereg.o getphoneui.o getphoneusr.o getphone.o getphoneuires.o
OBJS = troubleshooter.o
#LIBS = -luser32 -lgdi32 -lwsock32 -lwininet -ladvapi32
LIBS = -liphlpapi -lwsock32
#INCL = -I ${PCAP_PATH}/Include
DEFS = -D_WIN32_WINNT=0x0501

all: troubleshooter.exe

troubleshooter.exe: ${OBJS}
	#${CC} ${CFLAGS} -o troubleshooter.exe ${OBJS} ${LIBS} -mwindows
	${CC} ${CFLAGS} ${DEFS} -o troubleshooter.exe ${OBJS} ${LIBS}

clean:
	rm -f ${OBJS} troubleshooter.exe

.c.o:
	${CC} ${CFLAGS} ${DEFS} ${INCL} -c -o $*.o $<

%.o: %.rc
	${RC} ${INCL} $< $*.o
