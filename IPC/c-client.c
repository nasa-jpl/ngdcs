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

#define USE_UDP         1
#define USE_TCP         0

#define HOST            "127.0.0.1"
#define UDPPORT         1904
#define TCPPORT         1903

    /* client side */

int main(int argc,char *argv[])
{
    void *ch_udp,*ch_tcp;
    char string[513];
    int length;
    float float_length;

        /* connect to server with one UDP channel and one TCP */

    if (!(ch_udp=IPC_create_client_socket(HOST,UDPPORT,1,USE_UDP))) {
        (void)fprintf(stderr,"Couldn't create UDP client socket\n");
	exit(1);
    }

	/* loop, sending strings to the server and getting back their
	   lengths. */

    for (;;) {
	(void)printf("Enter string (up to 512 characters): ");
	if (!fgets(string,sizeof(string),stdin)) break;
	(void)printf("sending string...\n");

	if (IPC_send_STRING(ch_udp,string) == -1) exit(1);
	if (IPC_accept_INT(ch_udp,&length) == -1) exit(1);
	(void)printf("server says length across UDP is %d\n",length);
    }
}

