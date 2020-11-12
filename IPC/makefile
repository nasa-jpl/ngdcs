#----- MAKE WITH "PURIFY=purify" FOR PURIFIED VERSION -----

PURIFY =

#----- COMPILER -----

CC = gcc
CPP = g++

#----- USE -Dx IF ONE OF THE FOLLOWING OPERATING SYSTEMS: -----
#----- SOL2, T3D, VXWORKS, WIN32

CCFLAGS =
CFLAGS =

#----- FOR TRACE INFORMATION, INCLUDE -DTRACE_IPC

#CCFLAGS += -DTRACE_IPC
#CFLAGS += -DTRACE_IPC

#----- IF USING gcc COMPILER, INCLUDE -DGCC

CCFLAGS += -DGCC -g
CFLAGS += -DGCC -g

#----- ENVIRONMENT -----

OBJS = ipcsock.o ipcosdep.o ipcprims.o
INCLUDES = ipc.h

all: ipcprims.o ipcsock.o ipcosdep.o ipc0 ipc1 c-client c-server command telem dpserver

ipcprims.o: ipcprims.cpp $(INCLUDES)
	$(CPP) -o $@ $(CCFLAGS) -c ipcprims.cpp
	ar rv libipc.a ipcprims.o

ipcsock.o: ipcsock.cpp $(INCLUDES)
	$(CPP) -o $@ $(CCFLAGS) -c ipcsock.cpp
	ar rv libipc.a ipcsock.o

ipcosdep.o: ipcosdep.cpp $(INCLUDES)
	$(CPP) -o $@ $(CCFLAGS) -c ipcosdep.cpp
	ar rv libipc.a ipcosdep.o

ipc0: ipc0.cc ipcsock.o ipcprims.o ipcosdep.o
	$(CPP) $(CCFLAGS) ipc0.cc ipcsock.o ipcprims.o ipcosdep.o \
	    -o ipc0

ipc1: ipc1.cc ipcsock.o ipcprims.o ipcosdep.o
	$(CPP) $(CCFLAGS) ipc1.cc ipcsock.o ipcprims.o ipcosdep.o \
	    -o ipc1

c-client.o: c-client.c
	$(CC) $(CFLAGS) -c c-client.c

c-client: c-client.o ipcsock.o ipcprims.o ipcosdep.o
	$(CPP) $(CFLAGS) c-client.o ipcsock.o ipcprims.o ipcosdep.o \
	    -o c-client

c-server.o: c-server.c
	$(CC) $(CFLAGS) -c c-server.c

c-server: c-server.o ipcsock.o ipcprims.o ipcosdep.o
	$(CPP) $(CFLAGS) c-server.o ipcsock.o ipcprims.o ipcosdep.o \
	    -o c-server

telem: telem.cc ipcsock.o ipcprims.o ipcosdep.o
	$(CPP) $(CCFLAGS) telem.cc ipcsock.o ipcprims.o ipcosdep.o \
	    -o telem

command: command.cc ipcsock.o ipcprims.o ipcosdep.o
	$(CPP) $(CCFLAGS) command.cc ipcsock.o ipcprims.o ipcosdep.o \
	    -o command

dpserver.o: dpserver.c
	$(CC) $(CFLAGS) -c dpserver.c

dpserver: dpserver.o ipcsock.o ipcprims.o ipcosdep.o
	$(CPP) $(CCFLAGS) dpserver.o ipcsock.o ipcprims.o ipcosdep.o \
	    -o dpserver

#----- INSTALL -----

install: all
	cp -p libipc.a /usr/local/lib
	cp -p ipc.h /usr/local/include

#----- CLEAN -----

clean:
	rm -f *.o c-client.o c-server.o ipc0 ipc1 telem command libipc.a \
	    c-client c-server dpserver

#----- LINT -----

lint:
	/Users/alan/Lint/flint -DGCC -i /Users/alan/Lint -d_lint=1 \
	    std.lnt ipc.lnt ipcsock.cpp ipcosdep.cpp ipcprims.cpp
