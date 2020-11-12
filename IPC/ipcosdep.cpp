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

    // includes

#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#if !defined(WIN32)
#   include <unistd.h>
#endif
#if !defined(_WRS_KERNEL) && !defined(WIN32)
#   include <sys/param.h>
#else
#   define MAXPATHLEN 1024
#endif
#if defined(linux)
#include <sys/time.h>
#include <sys/resource.h>
#endif
#include <sys/stat.h>
#include "ipc.h"


    // define RLIM_CAST -- this varies platform by platform.

#if defined(SOL2)
#define RLIM_CAST (rlim_t)
#else
#define RLIM_CAST (int)
#endif


    // canonize and uncanonize functions

int canonize(const void *from,int from_size,void *to,int to_size)
{
    int i = 1;
    if (*(char *)&i == 1) { /* we're little-endian... */
	if (from_size != to_size) { /* changing size */
	    if (from_size < to_size) {
		(void)memset((char *)to,0,to_size);
		(void)memcpy((char *)to,(const char *)from,from_size);
		if (flip_word(to_size,to) == -1) return -1;
	    }
	    else {
		(void)fprintf(stderr,"send_bytes: Can't canonize\n");
		FATAL_COMM_ERROR
	    }
	}
	else { /* not changing size */
	    (void)memcpy((char *)to,(const char *)from,from_size);
	    if (flip_word(to_size,to) == -1) return -1;
	}
    }
    else /* we're big-endian */ {
	if (from_size != to_size) { /* changing size */
	    if (from_size < to_size) {
		int sparebytes = to_size-from_size;
		(void)memset((char *)to,0,sparebytes);
		(void)memcpy((char *)to+sparebytes,(const char *)from,
		    from_size);
	    }
	    else {
		(void)fprintf(stderr,"send_bytes: Can't canonize\n");
		FATAL_COMM_ERROR
	    }
	}
	else { /* not changing size */
	    (void)memcpy((char *)to,(const char *)from,from_size);
	}
    }
    return 0;
}

int uncanonize(const void *from,int from_size,void *to,int to_size)
{
    int i = 1;
    if (*(char *)&i == 1) { /* we're little-endian... */
	if (from_size != to_size) { /* changing size */
	    if (from_size > to_size) {
		int sparebytes = from_size-to_size;
		(void)memcpy((char *)to,(const char *)from+sparebytes,to_size);
		if (flip_word(to_size,to) == -1) return -1;
	    }
	    else {
		(void)fprintf(stderr,"accept_bytes: Can't uncanonize\n");
		FATAL_COMM_ERROR
	    }
	}
	else { /* not changing size */
	    (void)memcpy((char *)to,(const char *)from,to_size);
	    if (flip_word(to_size,to) == -1) return -1;
	}
    }
    else /* we're big-endian */ {
	if (from_size != to_size) { /* changing size */
	    if (from_size > to_size) {
		int sparebytes = from_size-to_size;
		(void)memcpy((char *)to,(const char *)from+sparebytes,to_size);
	    }
	    else {
		(void)fprintf(stderr,"accept_bytes: Can't uncanonize\n");
		FATAL_COMM_ERROR
	    }
	}
	else { /* not changing size */
	    (void)memcpy((char *)to,(const char *)from,to_size);
	}
    }
    return 0;
}


    // OS-dependent helper functions

#if defined(TRACE_IPC)
char *tracefile_name(void)
{
    static char filename[MAXPATHLEN];
#if defined(_WRS_KERNEL)
    (void)sprintf(filename,"ipc.trace");
#elif defined(WIN32)
    (void)sprintf(filename,"ipc-trace");
#else
    (void)sprintf(filename,"/tmp/ipc.trace.%d",getpid());
#endif
    return filename;
}


void set_line_buffering(FILE *fp)
{
#if defined(SOL2) || defined(WIN32)
    (void)setvbuf(fp,NULL,_IOLBF,0);
#else
    (void)setlinebuf(fp);
#endif
}
#endif


int determine_tablesize(void)
{
    int tablesize;
#if defined(_WRS_KERNEL) || defined(WIN32)
    tablesize = 32;
#elif defined(SOL2)
    struct rlimit rl;
    static int wrote_message = 0;
    if (getrlimit(RLIMIT_NOFILE,&rl) == -1) {
	if (!wrote_message) {
	    (void)sprintf(DA_ChanBase::last_message,
		"determine_tablesize: getrlimit failed -- "
		"recovering and suppressing further messages\n");
	    (void)fprintf(stderr,DA_ChanBase::last_message);
	    wrote_message = 1;
	}
	tablesize = 32;
    }
    else tablesize = (int)rl.rlim_cur;
#else
    tablesize = getdtablesize();
#endif
    return tablesize;
}


int increase_tablesize_if_necessary(const int *current_limit_p,int fd)
{
#if !defined(_WRS_KERNEL) && !defined(WIN32)
    if (fd > *current_limit_p - 10) {
	struct rlimit l;
	if (getrlimit(RLIMIT_NOFILE,&l) == -1) {
	    (void)sprintf(DA_ChanBase::last_message,
		"increase_tablesize_if_necessary: %s\n",
		ERRNOTEXT);
	    (void)fprintf(stderr,"%s",DA_ChanBase::last_message);
	    return -1;
        }
        l.rlim_cur = RLIM_CAST(*current_limit_p+50); /*lint !e732 */
        if (l.rlim_cur > l.rlim_max) l.rlim_cur = l.rlim_max;
        if (setrlimit(RLIMIT_NOFILE,&l) == -1) {
	    (void)sprintf(DA_ChanBase::last_message,
	        "increase_tablesize_if_necessary: %s\n",
	        ERRNOTEXT);
	    (void)fprintf(stderr,"%s",DA_ChanBase::last_message);
            return -1;
        }
    }
#endif
    return 0;
}


int set_close_on_exec(int fd)
{
#if !defined(_WRS_KERNEL) && !defined(WIN32)
    if (fcntl(fd,F_SETFD,1) == -1) {
	(void)sprintf(DA_ChanBase::last_message,
	    "set_close_on_exec: fcntl: %s\n",ERRNOTEXT);
	(void)fprintf(stderr,"%s",DA_ChanBase::last_message);
        return -1;
    }
#endif
    return 0;
}


int redirect_stdout(int fd)
{
#if defined(_WRS_KERNEL)
    return -1;
#elif defined(WIN32)
    return _dup2((int)fd,_fileno(stdout));
#else
    return dup2((int)fd,fileno(stdout));
#endif
}


int redirect_stderr(int fd)
{
#if defined(_WRS_KERNEL)
    return -1;
#elif defined(WIN32)
    return _dup2((int)fd,_fileno(stdout));
#else
    return dup2((int)fd,fileno(stderr));
#endif
}


void set_fd_in_mask(FDS_TYPE *mask_p,int fd)
{
    FD_ZERO(mask_p);   /*lint !e534 */
    FD_SET(fd,mask_p); /*lint !e573 !e701 !e717 !e737 */
}


#if defined(WIN32)
static int instance_count = 0;
#endif

int init_environment(void)
{
#if defined(WIN32)
    if (instance_count == 0) {
	USHORT wVersionRequested = 0x0101; /* version 1.1 */
	WSADATA wsaData;
	int nErrorStatus;

	nErrorStatus = WSAStartup(wVersionRequested,&wsaData);
	if (nErrorStatus != 0) {
	    (void)sprintf(DA_ChanBase::last_message,
	        "init_environment: WSAStartup: nErrorStatus = %d\n",
		nErrorStatus);
	    (void)fprintf(stderr,"%s",DA_ChanBase::last_message);
	    return -1;
	}
    }
    instance_count++;
#endif
    return 0;
}


void restore_environment(void)
{
#if defined(WIN32)
    instance_count--;
    if (instance_count == 0) {
	WSACleanup();
    }
#endif
}
