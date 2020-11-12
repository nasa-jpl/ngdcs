/* Copyright 2013, by the California Institute of Technology.  ALL RIGHTS
   RESERVED.  United States Government Sponsorship acknowledged.  Any 
   commercial use must be negotiated with the Office of Technology Transfer
   at the California Institute of Technology.

   This software may be subject to U.S. export control laws.  By accepting
   this software, the user agrees to comply with all applicable U.S. export
   laws and regulations.  User has the responsibility to obtain export 
   licenses, or other export authority as may be required before exporting
   such information to foreign countries or providing access to foreign
   persons.

   Please do not redistribute this file separate from the entire IPC source
   distribution.  For questions regarding this software, please contact the
   author, Alan Mazer, alan.s.mazer@jpl.nasa.gov */

#if defined(SOL2) || defined(GCC)
#include <stdlib.h>
#endif
#if defined(SOL2)
#include <unistd.h>
#endif
#include <string.h>
#include "ipc.h"

#define USE_UDP	1
#define USE_TCP	0

#define UDPPORT	1903
#define TCPPORT	1904

    /* server side */

int main(int argc,char *argv[])
{
    int servsock;
    DA_SocketChan *ch_udp,*ch_tcp;
    char *string;
    int length;

	/* create sockets, one UDP and one TCP */

#if 0
    servsock = DA_SocketChan::init(UDPPORT,NULL,USE_UDP);
    if (servsock == -1) {
	(void)fprintf(stderr,"init failed.\n");
	exit(1);
    }
    ch_udp = new DA_SocketChan(servsock,0,USE_UDP);
    if (ch_udp->failed)
	exit(1);
#endif

    servsock = DA_SocketChan::init(TCPPORT,NULL,USE_TCP);
    if (servsock == -1) {
	(void)fprintf(stderr,"init failed.\n");
	exit(1);
    }
    ch_tcp = new DA_SocketChan(servsock,0,USE_TCP);
    if (ch_tcp->failed)
	exit(1);

	/* loop, getting strings from client and returning their lengths */

    for (;;) {
#if 0
	if (ch_udp->accept(&string,"alan") == -1)
	    exit(1);
	length = strlen(string);
	(void)printf("server got string %s over UDP -- returning length %d\n",
	    string, length);
	if (ch_udp->send(length) == -1)
	    exit(1);
	if (ch_udp->send((float)length) == -1)
	    exit(1);
#endif

	if (ch_tcp->accept(&string,"\r\n") == -1)
	    exit(1);
	length = strlen(string);
	(void)printf("server got string %s over TCP -- returning length %d\n",
	    string, length);
#if 0
	if (ch_tcp->send(length) == -1)
	    exit(1);
	if (ch_tcp->send((float)length) == -1)
	    exit(1);
#endif
    }
#if defined(WIN32)
    return 0;
#endif
}
