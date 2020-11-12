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
#include "ipc.h"

#define USE_UDP		1
#define USE_TCP		0

#define HOST		"localhost"
#define UDPPORT		1903
#define TCPPORT		1904

    /* client side */

int main(int argc,char *argv[])
{
    DA_SocketChan *ch_udp,*ch_tcp;
    char string[513];
    int length;
    float float_length;

	/* connect to server with one UDP channel and one TCP */

    ch_udp = new DA_SocketChan(HOST,UDPPORT,1,USE_UDP);
    if (ch_udp->failed)
	exit(1);

    ch_tcp = new DA_SocketChan(HOST,TCPPORT,1,USE_TCP);
    if (ch_tcp->failed)
	exit(1);

	/* loop, sending strings to the server and getting back their
	   lengths. */

    for (;;) {
	(void)printf("Enter string (up to 512 characters): ");
	if (!fgets(string,sizeof(string),stdin)) break;
	(void)printf("sending string...\n");

	if (ch_udp->send(string,"alan") == -1) exit(1);
	if (ch_udp->accept(&length) == -1) exit(1);
	if (ch_udp->accept(&float_length) == -1) exit(1);
	(void)printf("server says length across UDP is %d (%f)\n",length,float_length);

	if (ch_tcp->send(string,"alan") == -1) exit(1);
	if (ch_tcp->accept(&length) == -1) exit(1);
	if (ch_tcp->accept(&float_length) == -1) exit(1);
	(void)printf("server says length across TCP is %d (%f)\n",length,float_length);
    }
#if defined(WIN32)
    return 0;
#endif
}

