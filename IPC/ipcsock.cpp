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

#include <ctype.h>
#if !defined(WIN32)
#if !defined(_WRS_KERNEL)
#include <sys/time.h>
#endif
#include <arpa/inet.h>
#endif
#include <fcntl.h>
#include <signal.h>
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
#include "ipc.h"


    // define NOTSUPPORTED to print out message saying x is not supported

#if defined(_WRS_KERNEL)
#define NOTSUPPORTED(caller,what) { \
    (void)sprintf(last_message,"%s: Internal error (%s not " \
	"supported under VxWorks)\n",caller,what); \
    (void)fputs(last_message,stderr); }
#elif defined(WIN32)
#define NOTSUPPORTED(caller,what) { \
    (void)sprintf(last_message,"%s: Internal error (%s not " \
	"supported under Windows)\n",caller,what); \
    (void)fputs(last_message,stderr); }
#endif
#define IMPOSSIBLE(caller,what) { \
    (void)sprintf(last_message, \
	"%s: Internal error (%s not supported)\n",caller,what); \
    (void)fputs(last_message,stderr); }


    // define ERRNOMSG to print out errno-based message.  define OTHERMSG to
    // print out non-errno-based message.

#if defined(NOGETPID)
#define ERRNOMSG(caller,function) { \
    (void)sprintf(last_message, \
	"%s: %s (handle %d): %s\n",caller,function,handle,ERRNOTEXT); \
    if (output_message_on_error) (void)fputs(last_message,stderr); }
#define ERRNOMSG_INIT(caller,function) { \
    (void)sprintf(last_message, \
	"%s: %s: %s\n",caller,function,ERRNOTEXT); \
    (void)fputs(last_message,stderr); }
#define OTHERMSG(caller,message) { \
    (void)sprintf(last_message, \
	"%s: %s (handle %d)\n",caller,message,handle); \
    if (output_message_on_error) (void)fputs(last_message,stderr); }
#else
#define ERRNOMSG(caller,function) { \
    (void)sprintf(last_message, \
	"%s: %s (handle %d, pid %d): %s\n",caller,function,handle, \
	    getpid(),ERRNOTEXT); \
    if (output_message_on_error) (void)fputs(last_message,stderr); }
#define ERRNOMSG_INIT(caller,function) { \
    (void)sprintf(last_message, \
	"%s: %s (pid %d): %s\n",caller,function,getpid(), \
	    ERRNOTEXT); \
    (void)fputs(last_message,stderr); }
#define OTHERMSG(caller,function) { \
    (void)sprintf(last_message, \
	"%s: %s (handle %d, pid %d)\n",caller,function,handle,getpid()); \
    if (output_message_on_error) (void)fputs(last_message,stderr); }
#endif

    // make sure FAILED is defined

#undef FAILED
#define FAILED { failed = 1; return; }

    // DA_RSocketChan (public DA_RChan)

DA_RSocketChan::DA_RSocketChan() : DA_RChan(1)
{
    errors_are_fatal = 1;
}


DA_RSocketChan::DA_RSocketChan(int eaf) : DA_RChan(eaf)
{
    errors_are_fatal = eaf;
}


int DA_RSocketChan::accept_bytes(void *ptr,u_int nbytes,
    int read_everything,int timeout_in_ms)
{
    int status;
    u_int total_read,chunk;
#if defined(WIN32) || defined(_WRS_KERNEL)
    int sa_length;
#else
    socklen_t sa_length;
#endif

	/* while we haven't yet read everything... */

    total_read = 0;
    while (total_read < nbytes) {

	    /* read up to the amount requested */

	chunk = nbytes-total_read;
	if (message_ready(timeout_in_ms) != 1) {
	    OTHERMSG("DA_RSocketChan::accept_bytes","message_ready (timeout)");
	    return -1;
	}
	sa_length = sizeof(udp_sender);
	if (use_udp)
	    status = recvfrom(handle,(char *)ptr+total_read,chunk,0,&udp_sender,
		&sa_length);
#if defined(WIN32)
	else status = ::recv(handle,(char *)ptr+total_read,chunk,0);
#else
	else status = read(handle,(char *)ptr+total_read,chunk);
#endif

	    /* if we had an error... */

	if (status == -1) {

#if !defined(EINTR_NOT_AVAILABLE)
		/* if we just got a signal, trust that it's been handled
		   and keep going */

	    if (errno == EINTR) continue;
#endif

		/* otherwise bail out, showing error and pid as applicable */

	    if (output_message_on_error) ERRNOMSG("accept_bytes","read")
	    if (errors_are_fatal) FATAL_COMM_ERROR
	    return -1;
	}

	    /* update amount read with whatever we read just now */

	total_read += (u_int)status;

	    /* if we read nothing, sender must have died so return.  otherwise
	       continue trying to read remaining bytes. */

	if (status == 0) {
	    if (output_message_on_error)
		OTHERMSG("accept_bytes","read returned 0")
	    if (errors_are_fatal) FATAL_COMM_ERROR
	    return -1;
	}

	    /* if we don't care about getting everything (perhaps we weren't
	       sure how many bytes would be there so we requested 1000000)
	       be happy and break out of loop. */

	else if (!read_everything) break;
    }

	/* return total number of bytes read, in case we didn't know how
	   many to expect */

    bytes_received += total_read;
    return (int)total_read;
}


int DA_RSocketChan::message_ready(int timeout_in_ms)
{
    FDS_TYPE readfds;
    struct timeval timeout_struct;
    int select_return;
    static int tablesize = 0;

	/* if timeout is -1, return 1 rather than blocking here */

    if (timeout_in_ms == -1) return 1;

	/* else return 1 if something is available to be read, else return 0
	   or -1 as appropriate */

    set_fd_in_mask(&readfds,handle);

    timeout_struct.tv_sec = timeout_in_ms / 1000;
    timeout_struct.tv_usec = (timeout_in_ms % 1000) * 1000;

    if (tablesize == 0) tablesize = determine_tablesize();
    select_return = select(tablesize,&readfds,NULL,NULL,&timeout_struct);
    if (select_return > 0) return 1;
    else if (select_return == -1) return -1;
    else return 0;
}


extern "C" int IPC_message_ready(void *ch)
{
    return ((DA_SocketChan *)ch)->message_ready();
}


int DA_RSocketChan::message_ready(int socknum,int timeout_in_ms)
{
    FDS_TYPE readfds;
    struct timeval timeout_struct;
    int select_return;
    static int tablesize = 0;

	/* if timeout is -1, return 1 rather than blocking here */

    if (timeout_in_ms == -1) return 1;

	/* else return 1 if something is available to be read, else return 0
	   or -1 as appropriate */

    set_fd_in_mask(&readfds,socknum);

    timeout_struct.tv_sec = timeout_in_ms / 1000;
    timeout_struct.tv_usec = (timeout_in_ms % 1000) * 1000;

    if (tablesize == 0) tablesize = determine_tablesize();
    select_return = select(tablesize,&readfds,NULL,NULL,&timeout_struct);
    if (select_return > 0) return 1;
    else if (select_return == -1) return -1;
    else return 0;
}


DA_RSocketChan::~DA_RSocketChan()
{
	/* make sure file descriptor gets closed */

    if (handle != -1) {
        ipc_close(handle);
#if defined(_WRS_KERNEL)
	shutdown(handle,2);
#endif
    }
    handle = -1;
}


    // DA_WSocketChan (public DA_WChan)

DA_WSocketChan::DA_WSocketChan() : DA_WChan(1)
{
    errors_are_fatal = 1;
}


DA_WSocketChan::DA_WSocketChan(int eaf) : DA_WChan(eaf)
{
    errors_are_fatal = eaf;
}


int DA_WSocketChan::send_bytes(const void *ptr,u_int nbytes)
{
    int status;
    u_int total_written,chunk;

	/* keep track of how much of message we've written so far */

    total_written = 0;

	/* while we still have stuff to write */

    while (total_written < nbytes) {

	    /* try to send all remaining bytes now */

	chunk = nbytes-total_written;
	if (is_client || !use_udp)
#if defined(WIN32) || defined(GCC)
	    status = ::send(handle,(const char *)ptr+total_written,chunk,0);
#elif defined(_WRS_KERNEL)
            status = ::send(handle,(char *)ptr+total_written,chunk,0);
#else
	    status = write(handle,(const char *)ptr+total_written,chunk);
#endif
	else {
#if defined(_WRS_KERNEL)
	    status = sendto(handle,(char *)ptr+total_written,chunk,0,
	        &udp_sender,sizeof(udp_sender));
#else
	    status = sendto(handle,(const char *)ptr+total_written,chunk,0,
		&udp_sender,sizeof(udp_sender));
#endif
	}

	    /* if we couldn't send any, receiver must have died.  write a
	       message if we haven't written a similar message from here
	       before, and set a flag to prevent future messages.  in some
	       cases, a message may trigger another, and we want to prevent
	       an endless sequence of these. */

	if (status == 0) {
	    static int wrote_message = 0;
	    if (!wrote_message && output_message_on_error) {
		OTHERMSG("send_bytes",
		    "write returned 0 -- suppressing further messages")
		wrote_message = 1;
	    }
	    if (errors_are_fatal) FATAL_COMM_ERROR
	    return -1;
	}

	    /* if we encountered an actual error, and it was a signal, assume
	       it's been handled and continue.  otherwise write a message as
	       above. */

	else if (status == -1) {
#if !defined(EINTR_NOT_AVAILABLE)
	    if (errno == EINTR) continue;
#endif
	    static int wrote_message = 0;
	    if (!wrote_message && output_message_on_error) {
		char msg[132];
#if defined(WIN32)
		(void)sprintf(msg,"write (h_errno = %d)",h_errno);
#else
		(void)sprintf(msg,"write");
#endif
		(void)strcat(msg, " -- suppressing further messages");
		ERRNOMSG("send_bytes",msg)
		wrote_message = 1;
	    }
	    if (errors_are_fatal) FATAL_COMM_ERROR
	    return -1;
	}

	    /* update number of bytes written, and loop back to top to see
	       if we're done */

	total_written += (u_int)status;
    }

	/* return normally */

    bytes_sent += total_written;
    return 0;
}


int DA_WSocketChan::intercept_stdout(void)
{
	/* redirect stdout onto our descriptor */

    return redirect_stdout(handle);
}


int DA_WSocketChan::intercept_stderr(void)
{
	/* redirect stderr onto our descriptor */

    return redirect_stderr(handle);
}


DA_WSocketChan::~DA_WSocketChan()
{
	/* make sure we close any open file descriptor */

    if (handle != -1) {
        ipc_close(handle);
#if defined(_WRS_KERNEL)
	shutdown(handle,2);
#endif
    }
    handle = -1;
}


    // DA_SocketChan (public DA_RSocketChan, DA_WSocketChan)

DA_SocketChan::DA_SocketChan() : DA_RSocketChan(1), DA_WSocketChan(1)
{
    handle = -1;
    use_udp = 0;
    is_client = 0;
    IMPOSSIBLE("DA_SocketChan","object without arguments")
    failed = 1;
}


DA_SocketChan::DA_SocketChan(const char *name,int port,int eaf,int uu,
	const struct timeval *timeout) :
    DA_RSocketChan(eaf), DA_WSocketChan(eaf)
{
    int fd;

    handle = -1; // must be initialized for dtor
    use_udp = uu;
    if ((fd = client_ctor_init(name,port,timeout)) == -1) FAILED
    handle = fd;
    if (fd_limit == 0) fd_limit = determine_tablesize();
    if (increase_tablesize_if_necessary(&fd_limit,fd) == -1) FAILED
    is_client = 1;
#if !defined(_WRS_KERNEL) && !defined(WIN32)
    (void)signal(SIGPIPE,SIG_IGN);
#endif
}


extern "C" void *IPC_create_client_socket(const char *name,int port,int eaf,
    int uu)
{
    DA_SocketChan *ch;
    ch = new DA_SocketChan(name,port,eaf,uu);
    if (ch->failed) {
	delete ch;
	return NULL;
    }
    else return (void *)ch;
}


DA_SocketChan::DA_SocketChan(int servsock,int eaf,int uu,int timeout_in_ms,
	int keep_listen) :
    DA_RSocketChan(eaf), DA_WSocketChan(eaf)
{
    int fd;

    handle = -1; // must be initialized for dtor
    use_udp = uu;
    if ((fd = server_ctor_init(servsock,timeout_in_ms,keep_listen)) == -1)
	FAILED
    handle = fd;
    if (fd_limit == 0) fd_limit = determine_tablesize();
    if (increase_tablesize_if_necessary(&fd_limit,fd) == -1) FAILED
    is_client = 0;
#if !defined(_WRS_KERNEL) && !defined(WIN32)
    (void)signal(SIGPIPE,SIG_IGN);
#endif
}


extern "C" void *IPC_create_server_socket(int servsock,int eaf,int uu)
{
    DA_SocketChan *ch;
    ch = new DA_SocketChan(servsock,eaf,uu);
    if (ch->failed) {
	delete ch;
	return NULL;
    }
    else return (void *)ch;
}


extern "C" void *IPC_create_server_socket_with_timeout(int servsock,int eaf,
    int uu,int timeout_in_ms)
{
    DA_SocketChan *ch;
    ch = new DA_SocketChan(servsock,eaf,uu,timeout_in_ms);
    if (ch->failed) {
	delete ch;
	return NULL;
    }
    else return (void *)ch;
}


DA_SocketChan::DA_SocketChan(const DA_ChanBase *sc) : DA_RSocketChan(0),
    DA_WSocketChan(0)
{
    errors_are_fatal = sc->errors_are_fatal;
#if defined(_WRS_KERNEL)
    handle = sc->handle; // not optimal since fd will be closed twice but...
#elif defined(WIN32)
    handle = _dup(sc->handle);
#else
    handle = dup(sc->handle);
#endif
    is_client = sc->is_client;
    use_udp = sc->use_udp;
}


void DA_SocketChan::handle_alarm(int) { }


int DA_SocketChan::client_ctor_init(const char *name,int port,
    const struct timeval *timeout)
{
    u_int len;
    struct sockaddr_in sin;
#if !defined(_WRS_KERNEL)
    struct hostent *hp;
#endif
#if defined(WIN32)
    u_int laddr;
#else
    in_addr_t laddr;
#endif
    int sock_fd;
    char msg[132];
#if !defined(WIN32) && !defined(_WRS_KERNEL)
    struct itimerval timerval;
    struct sigaction sa;
#endif

    /* create the socket */
    if (use_udp)
	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    else sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock_fd < 0) {
#if defined(WIN32)
	(void)sprintf(msg,"socket (h_errno = %d)",h_errno);
#else
	(void)sprintf(msg,"socket");
#endif
	if (!DA_ChanBase::connect_fails_silently)
	    ERRNOMSG("DA_SocketChan::client_ctor_init",msg)
	return -1;
    }

    /* set all but address in descriptor */
    len = sizeof(sin);
    memset(&sin,0,len);
    sin.sin_family = AF_INET;
    sin.sin_port = htons((u_short)port);

    /* see if the host was a numeric IP address */
    if (isdigit(name[0]))
        sin.sin_addr.s_addr = inet_addr(name);
    else {
        /* get the display server address */
#if defined(_WRS_KERNEL)
	laddr = hostGetByName((char *)name);
	(void)fprintf(stderr,"host address for %s is 0x%x\n",name,laddr);
	if (laddr == 0) {
#else
	hp = gethostbyname(name);
	if (hp == NULL) {
#endif
	    (void)sprintf(last_message,"Host \"%s\" is unknown.\n",name);
	    if (!DA_ChanBase::connect_fails_silently &&
		    output_message_on_error)
		(void)fprintf(stderr,"%s",last_message);
	    return -1;
        }
#if !defined(_WRS_KERNEL)
#if !defined(SOL2)
#if defined(WIN32)
	if (hp->h_length != sizeof(unsigned long)) {
#else
	if (hp->h_length != (int)sizeof(in_addr_t)) {
#endif
	    (void)sprintf(last_message,"Unexpected address format returned by "
		"gethostbyname() (%d bytes).\n",hp->h_length);
	    if (!DA_ChanBase::connect_fails_silently &&
		    output_message_on_error)
		(void)fprintf(stderr,"%s",last_message);
            return -1;
        }
#endif
#if defined(WIN32)
	(void)memcpy(&laddr, hp->h_addr_list[0], sizeof(unsigned long));
#else
	(void)memcpy(&laddr, hp->h_addr_list[0], sizeof(in_addr_t));
#endif
	sin.sin_addr.s_addr = laddr;
#else
	sin.sin_addr.s_addr = laddr;
#endif
    }

    /* make the connection -- this is wrapped with an alarm to avoid long
       hangs if the connection can't be made, except on Windows */
#if !defined(WIN32) && !defined(_WRS_KERNEL)
    if (timeout) {
	memset(&timerval.it_interval, 0, sizeof(timerval.it_interval));
	timerval.it_value = *timeout;
	if (timerval.it_value.tv_sec == 0 && timerval.it_value.tv_usec == 0)
	    timerval.it_value.tv_usec = 1000; /* 1 ms is min -- 0 would block */
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handle_alarm;
	(void)sigaction(SIGALRM, &sa, NULL);
	(void)setitimer(ITIMER_REAL, &timerval, NULL);
    }
#endif
#if defined(WIN32)
    if (connect(sock_fd,(LPSOCKADDR)&sin,len) < 0) {
#else
    if (connect(sock_fd,(struct sockaddr *)&sin,len) < 0) { /*lint !e740 */
#if !defined(_WRS_KERNEL)
	if (timeout) {
	    memset(&timerval, 0, sizeof(timerval));
	    (void)setitimer(ITIMER_REAL, &timerval, NULL);
	}
#endif
#endif
	(void)sprintf(msg, "connect (can't connect to %s)", name);
	if (!DA_ChanBase::connect_fails_silently)
	    ERRNOMSG("DA_SocketChan::client_ctor_init",msg);
	ipc_close(sock_fd);
	return -1;
    }
#if !defined(WIN32) && !defined(_WRS_KERNEL)
    if (timeout) {
	memset(&timerval, 0, sizeof(timerval));
	(void)setitimer(ITIMER_REAL, &timerval, NULL);
    }
#endif

    return sock_fd;
}


int DA_SocketChan::server_ctor_init(int sock,int timeout_in_ms,int keep_listen)
{
    int insock;
    struct sockaddr_in sin;
#if defined(GCC)
    socklen_t len;
#else
    int len;
#endif

    /* if using UDP, we don't want to do this */
    if (use_udp)
	return sock;

    /* if timeout specified, call select for limited wait */
    if (timeout_in_ms != -1) {
	FDS_TYPE readfds;
	struct timeval timeout_struct;
	int select_return;
	static int tablesize = 0;

	    /* wait until remote end is ready for accept */

	set_fd_in_mask(&readfds,sock);

	timeout_struct.tv_sec = timeout_in_ms / 1000;
	timeout_struct.tv_usec = (timeout_in_ms % 1000) * 1000;

	if (tablesize == 0) tablesize = determine_tablesize();
	select_return = select(tablesize,&readfds,NULL,NULL,&timeout_struct);

	    /* if we timed out, return immediately */

	if (select_return == 0) {
	    (void)sprintf(last_message,
		"Can't establish connection (accept timed out).\n");
	    if (!DA_ChanBase::connect_fails_silently &&
		    output_message_on_error)
		(void)fprintf(stderr,"%s",last_message);
	    return -1;
	}

	    /* if we hit an error of some sort, return immediately */

	else if (select_return == -1) {
	    if (!DA_ChanBase::connect_fails_silently)
		ERRNOMSG("DA_SocketChan::server_ctor_init",
		    "select (accept would block)");
	    return -1;
	}
    }

    /* accept connections */
    len = sizeof(sin);
#if defined(EINTR_NOT_AVAILABLE)
    insock = ::accept(sock, (struct sockaddr *)&sin, &len);
#else
    while (((insock = ::accept(sock, (struct sockaddr *)&sin, /*lint !e740 */
		&len)) == -1) &&
	    (errno == EINTR))
	/* do nothing */ ;
#endif
    if (insock == -1) {
	(void)sprintf(last_message,"server_ctor_init: %s\n",ERRNOTEXT);
	if (!DA_ChanBase::connect_fails_silently &&
		output_message_on_error)
	    (void)fprintf(stderr,"%s",last_message);
    }
    if (!keep_listen)
        close_listener(sock);

    return insock;
}

void DA_SocketChan::close_listener(int sock)
{
    ipc_close(sock);
#if defined(_WRS_KERNEL)
    shutdown(sock,2);
#endif

    restore_environment();
}


/*lint --e{578} */
int DA_SocketChan::init(int port,FILE *local_trace_fp,int use_udp)
{
    struct sockaddr_in sin;
    u_int len;
    int sock;
    int on;

    /* make sure that environment is initialized -- socket MUST be
       be created to allow cleanup from DA_ChanBase destructor! */

    if (init_environment() == -1) {
	(void)sprintf(last_message,
#if defined(WIN32)
	    "init: init_environment failed (pid %d)\n",_getpid());
#else
	    "init: init_environment failed (pid %d)\n",getpid());
#endif
	(void)fprintf(stderr,"%s",last_message);
	return -1;
    }

    /* create socket */
    if (use_udp)
	sock = socket(AF_INET, SOCK_DGRAM, 0);
    else sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
	ERRNOMSG_INIT("DA_SocketChan::init","socket")
        return -1;
    }

    on = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
            (char *) &on, sizeof(on)) == -1) {
	ERRNOMSG_INIT("DA_SocketChan::init","setsockopt")
        return -1;
    }

    /* bind address */
    len = sizeof(sin);
    (void)memset(&sin,0,len);
    sin.sin_addr.s_addr = htonl(INADDR_ANY); /*lint !e835 !e572 !e845 */
    sin.sin_family = AF_INET;
    sin.sin_port = htons((u_short)port);
    if (bind(sock, (struct sockaddr *)&sin, len) == -1) { /*lint !e740 */
	ERRNOMSG_INIT("DA_SocketChan::init","bind")
        return -1;
    }

    /* if not UDP, listen for connections */
    if (!use_udp) {
	if (local_trace_fp)
	    (void)fprintf(local_trace_fp,
		"ready for connections on TCP port %d\n",
		port_num_from_socket(sock));
	if (listen(sock,1) == -1) {
	    ERRNOMSG_INIT("DA_SocketChan::init","listen")
	    return -1;
	}
    }

    return sock;
}


extern "C" int IPC_init(int port,FILE *local_trace_fp,int use_udp)
{
    return DA_SocketChan::init(port,local_trace_fp,use_udp);
}


int DA_SocketChan::port_num_from_socket(int sock)
{
    struct sockaddr name;
#if defined(GCC)
    socklen_t namelen;
#else
    int namelen;
#endif
    u_short portnum;

    namelen = sizeof(name);
    if (getsockname(sock,&name,&namelen) == -1) {
	(void)sprintf(last_message,"port_num_from_socket: %s\n",ERRNOTEXT);
	(void)fprintf(stderr,"%s",last_message);
	return -1;
    }
    (void)memcpy((char *)&portnum,(char *)name.sa_data,sizeof(portnum));
    portnum = ntohs(portnum);
    return (int)portnum;
}


extern "C" int IPC_port_num_from_socket(int sock)
{
    return DA_SocketChan::port_num_from_socket(sock);
}


DA_SocketChan::~DA_SocketChan() { }


extern "C" void IPC_free_socket(void *ch)
{ 
    delete (DA_SocketChan *)ch;
}



    // DA_RPipeChan (public DA_RSocketChan)

DA_RPipeChan::DA_RPipeChan() : DA_RSocketChan(1)
{
#if defined(NOPIPES)
    handle = -1; // must be initialized for dtor
    NOTSUPPORTED("DA_RPipeChan","pipes")
    failed = 1;
#else
    handle = 0;
    if (fd_limit == 0) fd_limit = determine_tablesize();
#endif
}


DA_RPipeChan::DA_RPipeChan(const char *fd_ascii,int eaf) : DA_RSocketChan(eaf)
{
#if defined(NOPIPES)
    handle = -1; // must be initialized for dtor
    NOTSUPPORTED("DA_RPipeChan","pipes")
    failed = 1;
#else
    handle = atoi(fd_ascii);
    if (set_close_on_exec(handle) == -1) FAILED
    if (fd_limit == 0) fd_limit = determine_tablesize();
    if (increase_tablesize_if_necessary(&fd_limit,handle) == -1) FAILED
#endif
}


DA_RPipeChan::DA_RPipeChan(int eaf,int fd) : DA_RSocketChan(eaf)
{
#if defined(NOPIPES)
    handle = -1; // must be initialized for dtor
    NOTSUPPORTED("DA_RPipeChan","pipes")
    failed = 1;
#else
    handle = fd;
#endif
}


DA_RPipeChan::DA_RPipeChan(int eaf,DA_WPipeChan **wchan_p) : DA_RSocketChan(eaf)
{
    handle = -1; // must be initialized for dtor
#if defined(NOPIPES)
    NOTSUPPORTED("DA_RPipeChan","pipes")
    failed = 1;
#else
    int fds[2];
#if defined(WIN32)
    if (_pipe(fds,4096,O_BINARY) == -1) {
#else
    if (pipe(fds) == -1) {
#endif
	ERRNOMSG("DA_RPipeChan","pipe");
	FAILED
    }
    handle = fds[0];
    *wchan_p = new DA_WPipeChan(eaf,fds[1]); /*lint !e1732 */
    if ((*wchan_p)->failed)
	FAILED
    if (set_close_on_exec(handle) == -1) FAILED
    if (fd_limit == 0) fd_limit = determine_tablesize();
    if (increase_tablesize_if_necessary(&fd_limit,fds[0]) == -1) FAILED
    if (increase_tablesize_if_necessary(&fd_limit,fds[1]) == -1) FAILED
#endif
}


DA_RPipeChan::DA_RPipeChan(const DA_RPipeChan& other) : DA_RSocketChan(other)
{
    handle = other.handle;
    errors_are_fatal = other.errors_are_fatal;
    output_message_on_error = other.output_message_on_error;
    is_client = other.is_client;
    use_udp = other.use_udp;
    udp_sender = other.udp_sender;
    bytes_received = other.bytes_received;
}


DA_RPipeChan::~DA_RPipeChan()
{
    if (handle != -1) {
        ipc_close(handle);
        handle = -1;
    }
}


    // DA_WPipeChan (public DA_WSocketChan)

DA_WPipeChan::DA_WPipeChan() : DA_WSocketChan(1)
{
#if defined(NOPIPES)
    handle = -1; // must be initialized for dtor
    NOTSUPPORTED("DA_WPipeChan","pipes")
    failed = 1;
#else
    handle = 1;
#endif
}


DA_WPipeChan::DA_WPipeChan(const char *fd_ascii,int eaf) : DA_WSocketChan(eaf)
{
#if defined(NOPIPES)
    handle = -1; // must be initialized for dtor
    NOTSUPPORTED("DA_WPipeChan","pipes")
    failed = 1;
#else
	/* save ASCII file descriptor as handle and make sure it gets closed
	   on exec.  otherwise forked processes will fill up file descriptor
	   table with stuff they're not using. */

    handle = atoi(fd_ascii);
    if (set_close_on_exec(handle) == -1) FAILED
    if (fd_limit == 0) fd_limit = determine_tablesize();
    if (increase_tablesize_if_necessary(&fd_limit,handle) == -1) FAILED
#endif
}


DA_WPipeChan::DA_WPipeChan(int eaf,int fd) : DA_WSocketChan(eaf)
{
#if defined(NOPIPES)
    handle = -1; // must be initialized for dtor
    NOTSUPPORTED("DA_WPipeChan","pipes")
    failed = 1;
#else
    handle = fd;
#endif
}


DA_WPipeChan::DA_WPipeChan(int eaf,DA_RPipeChan **rchan_p) : DA_WSocketChan(eaf)
{
    handle = -1; // must be initialized for dtor
#if defined(NOPIPES)
    NOTSUPPORTED("DA_WPipeChan","pipes")
    failed = 1;
#else
	/* get two file descriptors, forming pipe */

    int fds[2];
#if defined(WIN32)
    if (_pipe(fds,4096,O_BINARY) == -1) {
#else
    if (pipe(fds) == -1) {
#endif
	ERRNOMSG("DA_WPipeChan","pipe");
	FAILED
    }

	/* store one file descriptor and return other, making sure saved
	   handle gets closed on exec.  otherwise forked processes will fill
	   up file descriptor table with stuff they're not using. */

    handle = fds[1];
    *rchan_p = new DA_RPipeChan(eaf,fds[0]); /*lint !e1732 */
    if ((*rchan_p)->failed)
	FAILED
    if (set_close_on_exec(handle) == -1) FAILED
    if (fd_limit == 0) fd_limit = determine_tablesize();
    if (increase_tablesize_if_necessary(&fd_limit,fds[0]) == -1) FAILED
    if (increase_tablesize_if_necessary(&fd_limit,fds[1]) == -1) FAILED
#endif
}


DA_WPipeChan::DA_WPipeChan(const DA_WPipeChan& other) : DA_WSocketChan(other)
{
    handle = other.handle;
    errors_are_fatal = other.errors_are_fatal;
    output_message_on_error = other.output_message_on_error;
    is_client = other.is_client;
    use_udp = other.use_udp;
    udp_sender = other.udp_sender;
    bytes_sent = other.bytes_sent;
}


DA_WPipeChan::~DA_WPipeChan()
{
    if (handle != -1) {
        ipc_close(handle);
        handle = -1;
    }
}


    // DA_BufferedSocketChan (public DA_SocketChan)

DA_BufferedSocketChan::DA_BufferedSocketChan() :
    DA_SocketChan("localhost",1000,1)
{
#if defined(BUFFEREDIO)
    fp = NULL;
#endif
    IMPOSSIBLE("DA_BufferedSocketChan","object without arguments")
    failed = 1;
}


DA_BufferedSocketChan::DA_BufferedSocketChan(const char *name,int port,
	int eaf) :
    DA_SocketChan(name,port,eaf)
{
#if defined(BUFFEREDIO)
    if (!failed) {
	fp = fdopen(handle,"r+");
	if (!fp) {
	    ERRNOMSG("DA_BufferedSocketChan","fdopen")
	    failed = 1;
	}
    }
    else fp = NULL;
#endif
}


DA_BufferedSocketChan::DA_BufferedSocketChan(int servsock,int eaf,
	int timeout_in_ms) :
    DA_SocketChan(servsock,eaf,timeout_in_ms)
{
#if defined(BUFFEREDIO)
    if (!failed) {
	fp = fdopen(handle,"r+");
	if (!fp) {
	    ERRNOMSG("DA_BufferedSocketChan","fdopen")
	    failed = 1;
	}
    }
    else fp = NULL;
#endif
}


DA_BufferedSocketChan::DA_BufferedSocketChan(const DA_ChanBase *sc) :
    DA_SocketChan(sc)
{
#if defined(BUFFEREDIO)
    if (!failed) {
	fp = fdopen(handle,"r+");
	if (!fp) {
	    ERRNOMSG("DA_BufferedSocketChan","fdopen")
	    failed = 1;
	}
    }
    else fp = NULL;
#endif
}


int DA_BufferedSocketChan::send_bytes(const void *ptr,u_int nbytes)
{
#if defined(BUFFEREDIO)
    int status;
    u_int total_written,chunk;

	/* keep track of how much of message we've written so far */

    total_written = 0;

	/* while we still have stuff to write */

    while (total_written < nbytes) {

	    /* try to send all remaining bytes now */

	chunk = nbytes-total_written;
	status = fwrite((char *)ptr+total_written,1,chunk,fp);

	    /* if we couldn't send any, and we haven't reached eof, try again.
	       otherwise return or exit immediately. */

	if (status == 0) {
	    if (!feof(fp)) continue;
	    else {
		static int wrote_message = 0;
		if (!wrote_message && output_message_on_error) {
		    OTHERMSG("send_bytes",
			"fwrite returned 0 -- suppressing further messages")
		    wrote_message = 1;
		}
		if (errors_are_fatal) FATAL_COMM_ERROR
		return -1;
	    }
	}

	    /* update number of bytes written, and loop back to top to see
	       if we're done */

	total_written += status;
    }
    bytes_sent += total_written;
    return 0;
#else
    int total_written;
    total_written = DA_SocketChan::send_bytes(ptr,nbytes);
    if (total_written != -1) bytes_sent += (u_int)total_written;
    return total_written;
#endif
}


int DA_BufferedSocketChan::accept_bytes(void *ptr,u_int nbytes,
    int read_everything,int timeout_in_ms)
{
#if defined(BUFFEREDIO)
    int status,mr;
    u_int total_read;
    total_read = 0;

	/* read requested data */

    while (total_read < nbytes) {
	if (message_ready(timeout_in_ms) != 1) {
	    OTHERMSG("DA_BufferedSocketChan::accept_bytes",
		"message_ready (timeout)");
	    return -1;
	}
	if (fp->_cnt >= nbytes-total_read || read_everything) {
	    status = fread((char *)ptr+total_read,1,nbytes-total_read,fp);
	    if (status == 0) {
		if (!feof(fp)) continue;
		if (output_message_on_error)
		    OTHERMSG("accept_bytes",
			"buffered read of socket channel hit EOF")
		if (errors_are_fatal) FATAL_COMM_ERROR
		return -1;
	    }
	    total_read += status;
	}
	else {
	    while (fp->_cnt != 0 && total_read < nbytes) {
		status = fread((char *)ptr+total_read,1,fp->_cnt,fp);
		if (status == 0 && ferror(fp)) {
		    if (output_message_on_error)
			OTHERMSG("accept_bytes",
			    "buffered read of socket channel hit EOF")
		    if (errors_are_fatal) FATAL_COMM_ERROR
		    return -1;
		}
		total_read += status;
	    }
	    while ((mr=message_ready(0)) && !fp->_cnt && total_read < nbytes)
		*((char *)ptr+total_read++) = fgetc(fp);
	    if (!mr) break;
	}
    }

	/* return total number of bytes read, in case we didn't know how
	   many to expect */

    bytes_received += total_read;
    return total_read;
#else
    int total_read;
    total_read = DA_SocketChan::accept_bytes(ptr,nbytes,read_everything,
	timeout_in_ms);
    if (total_read != -1) bytes_received += (u_int)total_read;
    return total_read;
#endif
}


void DA_BufferedSocketChan::flush(void)
{
#if defined(BUFFEREDIO)
    (void)fflush(fp);
#endif
}


extern "C" void IPC_flush(void *ch)
{
    ((DA_SocketChan *)ch)->flush();
}


int DA_BufferedSocketChan::message_ready(int timeout_in_ms)
{
#if defined(BUFFEREDIO)
    FDS_TYPE readfds;
    struct timeval timeout_struct;
    int select_return;
    static int tablesize = 0;

	/* if timeout is -1, return 1 rather than blocking here */

    if (timeout_in_ms == -1) return 1;

	/* return 1 is something is available to be read, else return 0
	   or -1 as appropriate */

    FD_ZERO(&readfds);
    FD_SET((int)fileno(fp),&readfds);

    timeout_struct.tv_sec = timeout_in_ms / 1000;
    timeout_struct.tv_usec = (timeout_in_ms % 1000) * 1000;

    if (tablesize == 0) tablesize = determine_tablesize();
    select_return = select(tablesize,&readfds,NULL,NULL,&timeout_struct);
    if (select_return > 0) return 1;
    else if (select_return == -1) return -1;
    else if (fp->_cnt) return 1;
    else return 0;
#else
    return DA_SocketChan::message_ready(timeout_in_ms);
#endif /* BUFFEREDIO */
}


DA_BufferedSocketChan::~DA_BufferedSocketChan()
{
#if defined(BUFFEREDIO)
    if (fp) {
	if (fclose(fp) == EOF) ERRNOMSG("DA_BufferedSocketChan","fclose")
	fp = NULL;
	handle = -1;
    }
#endif
}


    // DA_BufferedRPipeChan (public DA_RPipeChan)

DA_BufferedRPipeChan::DA_BufferedRPipeChan() : DA_RPipeChan(0,0)
{
#if defined(BUFFEREDIO)
    fp = NULL;
#endif
    IMPOSSIBLE("DA_BufferedRPipeChan","object without arguments")
    failed = 1;
}


DA_BufferedRPipeChan::DA_BufferedRPipeChan(const char *fd_ascii,int eaf) :
    DA_RPipeChan(fd_ascii,eaf)
{
#if defined(BUFFEREDIO)
    if (!failed) {
	fp = fdopen(handle,"r");
	if (!fp) {
	    ERRNOMSG("DA_BufferedRPipeChan","fdopen")
	    failed = 1;
	}
    }
    else fp = NULL;
#endif
}


DA_BufferedRPipeChan::DA_BufferedRPipeChan(int eaf,int fd) :
    DA_RPipeChan(eaf,fd)
{
#if defined(BUFFEREDIO)
    if (!failed) {
	fp = fdopen(handle,"r");
	if (!fp) {
	    ERRNOMSG("DA_BufferedRPipeChan","fdopen")
	    failed = 1;
	}
    }
    else fp = NULL;
#endif
}


DA_BufferedRPipeChan::DA_BufferedRPipeChan(int eaf,DA_WPipeChan **wchan_p) :
    DA_RPipeChan(eaf,wchan_p)
{
#if defined(BUFFEREDIO)
    if (!failed) {
	fp = fdopen(handle,"r");
	if (!fp) {
	    ERRNOMSG("DA_BufferedRPipeChan","fdopen")
	    failed = 1;
	}
    }
    else fp = NULL;
#endif
}


int DA_BufferedRPipeChan::accept_bytes(void *ptr,u_int nbytes,
    int read_everything,int timeout_in_ms)
{
#if defined(BUFFEREDIO)
    int status,mr;
    u_int total_read;
    total_read = 0;

	/* read requested data */

    while (total_read < nbytes) {
	if (message_ready(timeout_in_ms) != 1) {
	    OTHERMSG("DA_BufferedRPipeChan::accept_bytes",
		"message_ready (timeout)");
	    return -1;
	}
	if (fp->_cnt >= nbytes-total_read || read_everything) {
	    status = fread((char *)ptr+total_read,1,nbytes-total_read,fp);
	    if (status == 0) {
		if (!feof(fp)) continue;
		if (output_message_on_error)
		    OTHERMSG("accept_bytes",
			"buffered read of socket channel hit EOF")
		if (errors_are_fatal) FATAL_COMM_ERROR
		return -1;
	    }
	    total_read += status;
	}
	else {
	    while (fp->_cnt != 0 && total_read < nbytes) {
		status = fread((char *)ptr+total_read,1,fp->_cnt,fp);
		if (status == 0 && ferror(fp)) {
		    if (output_message_on_error)
			OTHERMSG("accept_bytes",
			    "buffered read of socket channel hit EOF")
		    if (errors_are_fatal) FATAL_COMM_ERROR
		    return -1;
		}
		total_read += status;
	    }
	    while ((mr=message_ready(0)) && !fp->_cnt && total_read < nbytes)
		*((char *)ptr+total_read++) = fgetc(fp);
	    if (!mr) break;
	}
    }

	/* return total number of bytes read, in case we didn't know how
	   many to expect */

    bytes_received += total_read;
    return total_read;
#else
    int total_read;
    total_read = DA_RPipeChan::accept_bytes(ptr,nbytes,read_everything,
	timeout_in_ms);
    if (total_read != -1) bytes_received += (u_int)total_read;
    return total_read;
#endif
}


int DA_BufferedRPipeChan::message_ready(int timeout_in_ms)
{
#if defined(BUFFEREDIO)
    FDS_TYPE readfds;
    struct timeval timeout_struct;
    int select_return;
    static int tablesize = 0;

	/* if timeout is -1, return 1 rather than blocking here */

    if (timeout_in_ms == -1) return 1;

	/* return 1 is something is available to be read, else return 0
	   or -1 as appropriate */

    FD_ZERO(&readfds);
    FD_SET((int)fileno(fp),&readfds);

    timeout_struct.tv_sec = timeout_in_ms / 1000;
    timeout_struct.tv_usec = (timeout_in_ms % 1000) * 1000;

    if (tablesize == 0) tablesize = determine_tablesize();
    select_return = select(tablesize,&readfds,NULL,NULL,&timeout_struct);
    if (select_return > 0) return 1;
    else if (select_return == -1) return -1;
    else if (fp->_cnt) return 1;
    else return 0;
#else
    return DA_RPipeChan::message_ready(timeout_in_ms);
#endif /* BUFFEREDIO */
}


DA_BufferedRPipeChan::~DA_BufferedRPipeChan()
{
#if defined(BUFFEREDIO)
    if (fp) {
	if (fclose(fp) == EOF) ERRNOMSG("DA_BufferedRPipeChan","fclose")
	fp = NULL;
	handle = -1;
    }
#endif
}


    // DA_BufferedWPipeChan (public DA_WPipeChan)

DA_BufferedWPipeChan::DA_BufferedWPipeChan() : DA_WPipeChan(1,0)
{
#if defined(BUFFEREDIO)
    fp = NULL;
#endif
    IMPOSSIBLE("DA_BufferedWPipeChan","object without arguments")
    failed = 1;
}


DA_BufferedWPipeChan::DA_BufferedWPipeChan(const char *fd_ascii,int eaf) :
    DA_WPipeChan(fd_ascii,eaf)
{
#if defined(BUFFEREDIO)
    if (!failed) {
	fp = fdopen(handle,"w");
	if (!fp) {
	    ERRNOMSG("DA_BufferedWPipeChan","fdopen")
	    failed = 1;
	}
    }
    else fp = NULL;
#endif
}


DA_BufferedWPipeChan::DA_BufferedWPipeChan(int eaf,int fd) :
    DA_WPipeChan(eaf,fd)
{
#if defined(BUFFEREDIO)
    if (!failed) {
	fp = fdopen(handle,"w");
	if (!fp) {
	    ERRNOMSG("DA_BufferedWPipeChan","fdopen")
	    failed = 1;
	}
    }
    else fp = NULL;
#endif
}


DA_BufferedWPipeChan::DA_BufferedWPipeChan(int eaf,DA_RPipeChan **rchan_p) :
    DA_WPipeChan(eaf,rchan_p)
{
#if defined(BUFFEREDIO)
    if (!failed) {
	fp = fdopen(handle,"w");
	if (!fp) {
	    ERRNOMSG("DA_BufferedWPipeChan","fdopen")
	    failed = 1;
	}
    }
    else fp = NULL;
#endif
}


int DA_BufferedWPipeChan::send_bytes(const void *ptr,u_int nbytes)
{
#if defined(BUFFEREDIO)
    int status;
    u_int total_written,chunk;

	/* keep track of how much of message we've written so far */

    total_written = 0;

	/* while we still have stuff to write */

    while (total_written < nbytes) {

	    /* try to send all remaining bytes now */

	chunk = nbytes-total_written;
	status = fwrite((char *)ptr+total_written,1,chunk,fp);

	    /* if we couldn't send any, and we haven't reached eof, try again.
	       otherwise return or exit immediately. */

	if (status == 0) {
	    if (!feof(fp)) continue;
	    else {
		static int wrote_message = 0;
		if (!wrote_message && output_message_on_error) {
		    OTHERMSG("send_bytes",
			"fwrite returned 0 -- suppressing further messages")
		    wrote_message = 1;
		}
		if (errors_are_fatal) FATAL_COMM_ERROR
		return -1;
	    }
	}

	    /* update number of bytes written, and loop back to top to see
	       if we're done */

	total_written += status;
    }
    bytes_sent += total_written;
    return 0;
#else
    int total_written;
    total_written = DA_WPipeChan::send_bytes(ptr,nbytes);
    if (total_written != -1) bytes_sent += (u_int)total_written;
    return total_written;
#endif
}


void DA_BufferedWPipeChan::flush(void)
{
#if defined(BUFFEREDIO)
    (void)fflush(fp);
#endif
}


DA_BufferedWPipeChan::~DA_BufferedWPipeChan()
{
#if defined(BUFFEREDIO)
    if (fp) {
	if (fclose(fp) == EOF) ERRNOMSG("DA_BufferedWPipeChan","fclose")
	fp = NULL;
	handle = -1;
    }
#endif
}


    // message supression

extern "C" void IPC_suppress_messages(void *ch)
{ 
    ((DA_SocketChan *)ch)->suppress_messages();
}


extern "C" void IPC_unsuppress_messages(void *ch)
{ 
    ((DA_SocketChan *)ch)->unsuppress_messages();
}
