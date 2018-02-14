#CC = i586-mingw32msvc-gcc
CC = i686-w64-mingw32-gcc
#RC = i586-mingw32msvc-windres
RC = i686-w64-mingw32-windres
PCAP_PATH = ./WpdPack
CFLAGS = -O -Wall -ggdb

OBJS = troubleshooter.o
LIBS = -liphlpapi -lwsock32
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
