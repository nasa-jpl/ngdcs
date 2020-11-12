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

#define USE_UDP 1       
#define USE_TCP 0

#define UDPPORT 1903
#define TCPPORT 1904

    /* server side */

int main(int argc,char *argv[])
{
    int servsock;
    void *ch_udp,*ch_tcp;
    char *string;
    int length;

        /* create sockets, one UDP and one TCP */

    servsock = IPC_init(UDPPORT,NULL,USE_UDP);
    if (servsock == -1) {
	(void)fprintf(stderr,"init failed.\n");
	exit(1);
    }
    if (!(ch_udp = IPC_create_server_socket(servsock,0,USE_UDP))) {
	(void)fprintf(stderr,"Couldn't create UDP server socket\n");
	exit(1);
    }

    servsock = IPC_init(TCPPORT,NULL,USE_TCP);
    if (servsock == -1) {
	(void)fprintf(stderr,"init failed.\n");
	exit(1);
    }
    if (!(ch_tcp = IPC_create_server_socket(servsock,0,USE_TCP))) {
	(void)fprintf(stderr,"Couldn't create TCP server socket\n");
	exit(1);
    }

	/* loop, getting strings from client and returning their lengths */

    for (;;) {
	if (IPC_accept_TERMSTRING(ch_udp,&string,"alan") == -1) exit(1);
	length = strlen(string);
	(void)printf("server got string %s over UDP -- returning length %d\n",
	    string, length);
	if (IPC_send_INT(ch_udp,length) == -1) exit(1);
	if (IPC_send_FLOAT(ch_udp,(float)length) == -1) exit(1);

	if (IPC_accept_TERMSTRING(ch_tcp,&string,"alan") == -1) exit(1);
	length = strlen(string);
	(void)printf("server got string %s over TCP -- returning length %d\n",
	    string, length);
	if (IPC_send_INT(ch_tcp,length) == -1) exit(1);
	if (IPC_send_FLOAT(ch_tcp,(float)length) == -1) exit(1);
    }
}
