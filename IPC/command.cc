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
#include <string.h>
#include "ipc.h"

#define USE_UDP	1

#define CCSDS_PRIMARY_SIZE	3 /* words */
#define CCSDS_SECONDARY_SIZE	5 /* words */
#define CCSDS_TOTAL_HEADER_SIZE	(CCSDS_PRIMARY_SIZE + CCSDS_SECONDARY_SIZE)
#define COMMAND_DATA_SIZE       35 /* words */
#define COMMAND_PACKET_SIZE	(COMMAND_DATA_SIZE + CCSDS_TOTAL_HEADER_SIZE)

#define CCSDS_VERSIONTYPE       0x1
#define CCSDS_SCND_HDR_FLAG     0x1
#define CCSDS_COMMAND_APID      0x08e
#define CCSDS_SEQ_FLAG          0x0 /* special for OPALS */
#define CCSDS_LENGTH            ((CCSDS_SECONDARY_SIZE + COMMAND_DATA_SIZE) * \
				    sizeof(short) - 1)


int main(int argc,char *argv[])
{
    int portnum;
    DA_SocketChan *ch;
    u_short command[COMMAND_PACKET_SIZE];
    int number;

	/* init command */

    (void)memset(command, 0, sizeof(command));
    command[0] = (CCSDS_VERSIONTYPE << 12) |
        (CCSDS_SCND_HDR_FLAG << 11) | CCSDS_COMMAND_APID;
    command[2] = CCSDS_LENGTH;
    for (int i=CCSDS_TOTAL_HEADER_SIZE;i < COMMAND_PACKET_SIZE;i++)
	command[i] = i - CCSDS_TOTAL_HEADER_SIZE + 1;

	/* port number is assumed to be passed on command line -- get it now */

    if (argc != 3) {
	(void)fprintf(stderr,"put port number and hostname on command line\n");
	exit(1);
    }
    portnum = atoi(argv[1]);

	/* connect to server using specified port -- host is "localhost" but
	   any hostname can be used -- resulting in a channel object. */

    ch = new DA_SocketChan(argv[2],portnum,0,USE_UDP);
    if (ch->failed)
	exit(1);

	/* loop, getting strings from the user and sending them to the server */

    number = 0;
    for (;;) {
	(void)printf("Hit RETURN to send a command packet? ");
	while (getchar() != '\n') ;
	command[1] = (CCSDS_SEQ_FLAG << 14) | (++number & 0x3fff);
	if (ch->send(command, COMMAND_PACKET_SIZE) == -1) exit(1);
    }
#if defined(WIN32)
    return 0;
#endif
}

