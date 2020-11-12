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

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "ipc.h"

#define USE_UDP 1

#define PORT 10362

#define OP_READINT	0
#define OP_WRITEINT	1
#define OP_READFLOAT	2
#define OP_WRITEFLOAT	3
#define OP_READLONG	4
#define OP_WRITELONG	5
#define OP_STROBE	6

#define FAIL_MSG printf("Connected dropped...waiting for reconnect\n");

    /* server side */

int main(int argc,char *argv[])
{
    int servsock;
    void *ch;
    int op;
    unsigned int address;
    unsigned int uival;
    float fval;
    unsigned long int ulival;
    unsigned char *memory;

	/* create fake memory space for testing */

    memory = (unsigned char *)malloc(64*1024);
    if (!memory) {
	(void)fprintf(stderr,"can't get required memory\n");
	exit(1);
    }
    (void)memset(memory,0,64*1024);

        /* create sockets, one UDP and one TCP */

    servsock = IPC_init(PORT,NULL,USE_UDP);
    if (servsock == -1) {
	(void)fprintf(stderr,"init failed\n");
	exit(1);
    }
    if (!(ch = IPC_create_server_socket(servsock,0,USE_UDP))) {
	(void)fprintf(stderr,"Couldn't create TCP server socket\n");
	exit(1);
    }

	/* loop, getting strings from client and returning their lengths */

    for (;;) {
	if (IPC_accept_INT(ch,&op) == -1)
	    FAIL_MSG
	else if (IPC_accept_UINT(ch,&address) == -1)
	    FAIL_MSG
	else switch (op) {
	    case OP_READINT:
		(void)printf("server got read request for 0x%08x\n", address);
		uival = *(unsigned int *)(memory+(address & 0xffff));
		if (IPC_send_UINT(ch,uival) == -1) FAIL_MSG
		else (void)printf("server sent value 0x%08x\n",uival);
		break;
	    case OP_WRITEINT:
		(void)printf("server got write request for 0x%08x\n", address);
		if (IPC_accept_UINT(ch,&uival) == -1) FAIL_MSG
		else {
		    *(unsigned int *)(memory+(address & 0xffff)) = uival;
		    (void)printf("server wrote 0x%08x\n",uival);
		}
		break;
	    case OP_READFLOAT:
		(void)printf("server got read request for 0x%08x\n", address);
		fval = *(float *)(memory+(address & 0xffff));
		if (IPC_send_FLOAT(ch,fval) == -1) FAIL_MSG
		else (void)printf("server sent value %f\n",fval);
		break;
	    case OP_WRITEFLOAT:
		(void)printf("server got write request for 0x%08x\n", address);
		if (IPC_accept_FLOAT(ch,&fval) == -1) FAIL_MSG
		else {
		    *(float *)(memory+(address & 0xffff)) = fval;
		    (void)printf("server wrote %f\n",fval);
		}
		break;
	    case OP_READLONG:
		(void)printf("server got read request for 0x%08x\n", address);
		ulival = *(unsigned long int *)(memory+(address & 0xffff));
		if (IPC_send_ULONGINT(ch,ulival) == -1) FAIL_MSG
		else (void)printf("server sent value 0x%016lx\n",ulival);
		break;
	    case OP_WRITELONG:
		(void)printf("server got write request for 0x%08x\n", address);
		if (IPC_accept_ULONGINT(ch,&ulival) == -1) FAIL_MSG
		else {
		    *(unsigned long int *)(memory+(address & 0xffff)) = ulival;
		    (void)printf("server wrote 0x%016lx\n",ulival);
		}
		break;
	    case OP_STROBE:
		(void)printf("server got strobe request for 0x%08x\n", address);
		break;
	    default:
		(void)printf("server got unknown request for address 0x%08x\n",
		    address);
		break;
	}
    }
}
