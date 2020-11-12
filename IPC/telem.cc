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

#define CCSDS_HEADER_SIZE       8 /* words */
#define TELEM_DATA_SIZE         376 /* words */
#define TELEM_MESSAGE_SIZE      (CCSDS_HEADER_SIZE + TELEM_DATA_SIZE) /* words */


int main(int argc,char *argv[])
{
    int portnum,servsock;
    DA_SocketChan *ch;
    u_short telem[TELEM_MESSAGE_SIZE];
    int checksum;

	/* port number is assumed to be passed on command line -- get it now */

    if (argc != 2) {
	(void)fprintf(stderr,"put port number on command line\n");
	exit(1);
    }
    portnum = atoi(argv[1]);

	/* create a socket, listening on that port */

    servsock = DA_SocketChan::init(portnum,NULL,USE_UDP);
    if (servsock == -1) {
	(void)fprintf(stderr,"init failed.\n");
	exit(1);
    }

	/* wait for client to connect to us.  afterwards we'll have channel
	   with which to communicate. */

    ch = new DA_SocketChan(servsock,0,USE_UDP);
    if (ch->failed)
	exit(1);

	/* loop, getting telem from client and displaying it */

    for (;;) {
	if (ch->accept(telem, TELEM_MESSAGE_SIZE) == -1)
	    exit(1);
	for (int i=0;i < TELEM_MESSAGE_SIZE;i++)
	    printf("%04x ", telem[i]);
	checksum = 0;
	for (int i=14;i < TELEM_MESSAGE_SIZE-1;i++)
	    checksum += telem[i];
	checksum &= 0xffff;
	printf("\nChecksum 0x%04x\n", checksum);
	printf("\n");
    }
#if defined(WIN32)
    return 0;
#endif
}
