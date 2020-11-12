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

    // make sure this file is only included once

#if !defined(IPC)
#define IPC

    // provide stdio.h and abbreviated name types

#include <stdio.h>
#if defined(WIN32)
typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
#endif
typedef unsigned long int u_longint;

    // include socket-related stuff

#if defined(_WRS_KERNEL)
#   include <vxWorks.h>
#   include <sys/times.h>
#   include <sockLib.h>
#   include <selectLib.h>
#   include <hostLib.h>
#endif
#if !defined(WIN32)
#   if !defined(VXWORKS)
#       include <netdb.h>
#   endif
#endif
#if defined(SOL2)
#   include <arpa/inet.h>
#endif
#if defined(WIN32)
#   include <io.h>
#   include <winsock.h>
#   include <process.h>
#endif

#if defined(__cplusplus)

    // define FDS_TYPE (probably int on non-Unix systems)

#if defined(WIN32)
#define FDS_TYPE struct fd_set
#else
#define FDS_TYPE fd_set
#endif

    // define BUFFEREDIO on machines which support it.  only machines which
    // support every needed feature (i.e., determining length of buffer
    // contents) get definition.  #define-ing BUFFEREDIO will improve
    // performance on some systems.  Interrupted library calls for file I/O
    // don't seem recoverable.  Failures are intermittent.)

#if defined(SOL2)
#define BUFFEREDIO
#endif

    // define NOPIPES on machines which don't support Unix pipe(2) system call.

#if defined(_WRS_KERNEL) || defined(WIN32)
#define NOPIPES
#endif

    // define NOGETPID on machines which don't support getpid(2) system call.
    // include WIN32 cautiously for Windows 95, even though NT has the call.

#if defined(_WRS_KERNEL) || defined(WIN32)
#define NOGETPID
#endif

    // define EINTR_NOT_AVAILABLE for machines without EINTR signal

#if defined(WIN32)
#define EINTR_NOT_AVAILABLE
#endif

    // define ERRNOTEXT to be the ASCII equivalent of the current errno value

#if defined(SOL2) || defined(GCC)
#   include <errno.h>
#endif
#define ERRNOTEXT strerror(errno)

    // define ipc_close macro to close a socket descriptor

#if defined(WIN32)
#define ipc_close(h)    ((void)closesocket((SOCKET)h))
#else
#define ipc_close(h)    ((void)close(h))
#endif

    // ********** NOTHING BELOW SHOULD BE MACHINE/OS DEPENDENT **********

    /* The first classes are not expected to be used for applications, but are
       rather used as a foundation for the other channel classes below. */

class DA_ChanBase {
friend class DA_SocketChan;
protected:
    int handle;
    int errors_are_fatal;
    int output_message_on_error;
    int is_client;
    int use_udp;
    struct sockaddr udp_sender;
    static int fd_limit;
public:
    int failed;
    enum { CHAR, UCHAR, SHORT, USHORT, INT, UINT, ULONGINT, FLOAT, DOUBLE,
	PTR, NUM_TYPES };
    static FILE *trace_fp;
    static char last_message[256];
    static int connect_fails_silently;

	// constructor

    DA_ChanBase(int eaf = 0);

	// misc

    virtual int get_handle(void) const { return handle; }
    virtual void set_handle(int h) { handle = h; }
    virtual void suppress_messages(void) { output_message_on_error = 0; }
    virtual void unsuppress_messages(void) { output_message_on_error = 1; }
    virtual char *last_error(void) const { return last_message; }

	// destructor

    virtual ~DA_ChanBase();
};


class DA_RChan: virtual public DA_ChanBase {
private:
public:
    u_int bytes_received;

	// constructor

    DA_RChan() : DA_ChanBase(1) {
	errors_are_fatal = 1; bytes_received = 0; }
    DA_RChan(int eaf) : DA_ChanBase(eaf) {
	errors_are_fatal = eaf; bytes_received = 0; }

	// functions to read various data types

    int accept(char *v_p,u_int n=1,int timeout_in_ms=-1,int read_everything=1);
    int accept(u_char *v_p,u_int n=1,int timeout_in_ms=-1,
	int read_everything=1);
    int accept(short *v_p,u_int n=1,int timeout_in_ms=-1,int read_everything=1);
    int accept(u_short *v_p,u_int n=1,int timeout_in_ms=-1,
	int read_everything=1);
    int accept(int *v_p,u_int n=1,int timeout_in_ms=-1,int read_everything=1);
    int accept(u_int *v_p,u_int n=1,int timeout_in_ms=-1,int read_everything=1);
    int accept(u_longint *v_p,u_int n=1,int timeout_in_ms=-1,
	int read_everything=1);
    int accept(float *v_p,u_int n=1,int timeout_in_ms=-1,int read_everything=1);
    int accept(double *v_p,u_int n=1,int timeout_in_ms=-1,
	int read_everything=1);

    int accept(void **v_p,int timeout_in_ms=-1);
    int accept(char **s_p,int timeout_in_ms=-1);
    int accept(char **s_p,const char *term,int timeout_in_ms=-1);
    int accept(char ***array_p,int timeout_in_ms=-1);
    int accept(void **ptr,u_int nbytes,int timeout_in_ms,int read_everything=1);

    int accept(int indent,const char *descr,char *v_p,u_int n=1,
	int timeout_in_ms=-1,int read_everything=1);
    int accept(int indent,const char *descr,u_char *v_p,u_int n=1,
	int timeout_in_ms=-1,int read_everything=1);
    int accept(int indent,const char *descr,short *v_p,u_int n=1,
	int timeout_in_ms=-1,int read_everything=1);
    int accept(int indent,const char *descr,u_short *v_p,u_int n=1,
	int timeout_in_ms=-1,int read_everything=1);
    int accept(int indent,const char *descr,int *v_p,u_int n=1,
	int timeout_in_ms=-1,int read_everything=1);
    int accept(int indent,const char *descr,u_int *v_p,u_int n=1,
	int timeout_in_ms=-1,int read_everything=1);
    int accept(int indent,const char *descr,u_longint *v_p,u_int n=1,
	int timeout_in_ms=-1,int read_everything=1);
    int accept(int indent,const char *descr,float *v_p,u_int n=1,
	int timeout_in_ms=-1,int read_everything=1);
    int accept(int indent,const char *descr,double *v_p,u_int n=1,
	int timeout_in_ms=-1,int read_everything=1);

    int accept(int indent,const char *descr,void **v_p,int timeout_in_ms=-1);
    int accept(int indent,const char *descr,char **s_p,int timeout_in_ms=-1);
    int accept(int indent,const char *descr,char **s_p,const char *term,
	int timeout_in_ms=-1);
    int accept(int indent,const char *descr,char ***array_p,
	int timeout_in_ms=-1);
    int accept(int indent,const char *descr,void **ptr,u_int nbytes,
	int timeout_in_ms=-1,int read_everything=1);

	// low-level reading

    virtual int accept_bytes(void *ptr,u_int nbytes,int read_everything,
	int timeout_in_ms=-1);

	// asynchronous message-available checking

    virtual int message_ready(int timeout_in_ms=0);

	// channel-type-specific destructor

    virtual ~DA_RChan();
};


class DA_WChan: virtual public DA_ChanBase {
public:
    u_int bytes_sent;

	// constructor

    DA_WChan() : DA_ChanBase(1) {
	errors_are_fatal = 1; bytes_sent = 0; }
    DA_WChan(int eaf) : DA_ChanBase(eaf) {
	errors_are_fatal = eaf; bytes_sent = 0; }

	// functions to send various data types

    int send(char v);
    int send(u_char v);
    int send(short v);
    int send(u_short v);
    int send(int v);
    int send(u_int v);
    int send(u_longint v);
    int send(float v);
    int send(double v);
    int send(void *v);

    int send(const char *v,u_int n);
    int send(const u_char *v,u_int n);
    int send(const short *v,u_int n);
    int send(const u_short *v,u_int n);
    int send(const int *v,u_int n);
    int send(const u_int *v,u_int n);
    int send(const u_longint *v,u_int n);
    int send(const float *v,u_int n);
    int send(const double *v,u_int n);
    int send(const void **v,u_int n);

    int send(const char *s);
    int send(const char *s,const char *term);
    int send(const char *const *array,int length);
    int send(const void *ptr,u_int nbytes);

    int send(int indent,const char *descr,char v);
    int send(int indent,const char *descr,u_char v);
    int send(int indent,const char *descr,short v);
    int send(int indent,const char *descr,u_short v);
    int send(int indent,const char *descr,int v);
    int send(int indent,const char *descr,u_int v);
    int send(int indent,const char *descr,u_longint v);
    int send(int indent,const char *descr,float v);
    int send(int indent,const char *descr,double v);
    int send(int indent,const char *descr,void *v);

    int send(int indent,const char *descr,const char *v,u_int n);
    int send(int indent,const char *descr,const u_char *v,u_int n);
    int send(int indent,const char *descr,const short *v,u_int n);
    int send(int indent,const char *descr,const u_short *v,u_int n);
    int send(int indent,const char *descr,const int *v,u_int n);
    int send(int indent,const char *descr,const u_int *v,u_int n);
    int send(int indent,const char *descr,const u_longint *v,u_int n);
    int send(int indent,const char *descr,const float *v,u_int n);
    int send(int indent,const char *descr,const double *v,u_int n);
    int send(int indent,const char *descr,const void **v,u_int n);

    int send(int indent,const char *descr,const char *s);
    int send(int indent,const char *descr,const char *s,const char *term);
    int send(int indent,const char *descr,const char *const *array,int length);
    int send(int indent,const char *descr,const void *ptr,u_int nbytes);

	// low-level writing

    virtual int send_bytes(const void *ptr,u_int nbytes);

	// flushing, if any

    virtual void flush(void);

	// stdio handling

    virtual int intercept_stdout(void);
    virtual int intercept_stderr(void);

	// channel-type-specific destructor

    virtual ~DA_WChan();
};


class DA_RSocketChan: public DA_RChan {
public:
    DA_RSocketChan();
    DA_RSocketChan(int eaf);
    virtual int accept_bytes(void *ptr,u_int nbytes,int read_everything,
	int timeout_in_ms=-1);
    virtual int message_ready(int timeout_in_ms=0);
    static int message_ready(int socknum,int timeout_in_ms); /*lint !e1411 */
    int get_fd(void) const { return handle; }
    virtual ~DA_RSocketChan();
};


class DA_WSocketChan: public DA_WChan {
public:
    DA_WSocketChan();
    DA_WSocketChan(int eaf);
    virtual int send_bytes(const void *ptr,u_int nbytes);
    virtual int intercept_stdout(void);
    virtual int intercept_stderr(void);
    virtual ~DA_WSocketChan();
};


    /* Following are the main classes to be used by applications: */

class DA_SocketChan: public DA_RSocketChan, public DA_WSocketChan {
private:
    int client_ctor_init(const char *name,int port,
	const struct timeval *timeout=NULL);
    int server_ctor_init(int sock,int timeout_in_ms,int keep_listen);
public:
    // generic ctor
    DA_SocketChan();
    // client side
    DA_SocketChan(const char *name,int port,int eaf,int use_udp=0,
	const struct timeval *timeout=NULL);
    // server side
    DA_SocketChan(int servsock,int eaf,int use_udp=0,int timeout_in_ms=-1,
	int keep_listen=0);

    DA_SocketChan(const DA_ChanBase *sc);
    static int  init(int port,FILE *trace_fp,int use_udp=0);
    static int  port_num_from_socket(int sock);
    static void close_listener(int sock);
    static void handle_alarm(int sig);

    virtual ~DA_SocketChan();
};


class DA_WPipeChan;

class DA_RPipeChan: public DA_RSocketChan {
friend class DA_WPipeChan;
public:
    DA_RPipeChan();
    DA_RPipeChan(const char *fd_ascii,int eaf);
    DA_RPipeChan(int eaf,int fd);
    DA_RPipeChan(int eaf,DA_WPipeChan **wchan_p);
    DA_RPipeChan(const DA_RPipeChan& other);
    virtual ~DA_RPipeChan();
};


class DA_WPipeChan: public DA_WSocketChan {
friend class DA_RPipeChan;
public:
    DA_WPipeChan();
    DA_WPipeChan(const char *fd_ascii,int eaf);
    DA_WPipeChan(int eaf,int fd);
    DA_WPipeChan(int eaf,DA_RPipeChan **rchan_p);
    DA_WPipeChan(const DA_WPipeChan& other);
    virtual ~DA_WPipeChan();
};


class DA_BufferedSocketChan: public DA_SocketChan {
#if defined(BUFFERED_IO)
protected:
    FILE *fp;
#endif
public:
    virtual int send_bytes(const void *ptr,u_int nbytes);
    virtual int accept_bytes(void *ptr,u_int nbytes,int read_everything,
	int timeout_in_ms);
    virtual int message_ready(int timeout_in_ms=0);
    virtual void flush(void);
    DA_BufferedSocketChan();
    DA_BufferedSocketChan(const char *name,int port,int eaf);	      // client
    DA_BufferedSocketChan(int servsock,int eaf,int timeout_in_ms=-1); // server
    DA_BufferedSocketChan(const DA_ChanBase *sc);
    virtual ~DA_BufferedSocketChan();
};


class DA_BufferedRPipeChan: public DA_RPipeChan {
#if defined(BUFFERED_IO)
protected:
    FILE *fp;
#endif
public:
    virtual int accept_bytes(void *ptr,u_int nbytes,int read_everything,
	int timeout_in_ms);
    virtual int message_ready(int timeout_in_ms=0);
    DA_BufferedRPipeChan();
    DA_BufferedRPipeChan(const char *fd_ascii,int eaf);
    DA_BufferedRPipeChan(int eaf,int fd);
    DA_BufferedRPipeChan(int eaf,DA_WPipeChan **wchan_p);
    virtual ~DA_BufferedRPipeChan();
};


class DA_BufferedWPipeChan: public DA_WPipeChan {
#if defined(BUFFERED_IO)
protected:
    FILE *fp;
#endif
public:
    virtual int send_bytes(const void *ptr,u_int nbytes);
    virtual void flush(void);
    DA_BufferedWPipeChan();
    DA_BufferedWPipeChan(const char *fd_ascii,int eaf);
    DA_BufferedWPipeChan(int eaf,int fd);
    DA_BufferedWPipeChan(int eaf,DA_RPipeChan **rchan_p);
    virtual ~DA_BufferedWPipeChan();
};


#if !defined(FATAL_COMM_ERROR)
#if defined(WIN32)
#define FATAL_COMM_ERROR { \
    (void)fprintf(stderr,"Fatal comm error\n"); WSACleanup(); exit(1); }
#elif defined(_WRS_KERNEL)
#define FATAL_COMM_ERROR { \
    (void)fprintf(stderr,"Fatal comm error\n"); return -1; }
#else
#define FATAL_COMM_ERROR { \
    (void)fprintf(stderr,"Fatal comm error\n"); exit(1); }
#endif
#endif


    // global data

extern const u_int canonical_sizes[];

    // OS-dependent helper functions

extern int canonize(const void *from,int from_size,void *to,int to_size);
extern int uncanonize(const void *from,int from_size,void *to,int to_size);
extern int flip_word(u_int size,void *p);
#if defined(TRACE_IPC)
extern char *tracefile_name(void);
extern void set_line_buffering(FILE *fp);
#endif
extern int determine_tablesize(void);
extern int increase_tablesize_if_necessary(const int *current_limit_p,int fd);
extern int set_close_on_exec(int fd);
extern int redirect_stdout(int fd);
extern int redirect_stderr(int fd);
extern void set_fd_in_mask(FDS_TYPE *mask_p,int fd);
extern int init_environment(void);
extern void restore_environment(void);

#endif /* defined(__cplusplus) */

#if defined(__cplusplus)
extern "C" {
#endif

    /* C wrappers -- creation */

void *IPC_create_client_socket(const char *name,int port,int eaf,int uu);
void *IPC_create_server_socket(int servsock,int eaf,int uu);
void *IPC_create_server_socket_with_timeout(int servsock,int eaf,int uu,
    int timeout_in_ms);
int IPC_init(int port,FILE *local_trace_fp,int use_udp);
int IPC_port_num_from_socket(int sock);

    /* C wrappers -- deletion */

void IPC_free_socket(void *ch);

    /* C wrappers -- send */

int IPC_send_CHAR(void *ch,char v);
int IPC_send_UCHAR(void *ch,u_char v);
int IPC_send_SHORT(void *ch,short v);
int IPC_send_USHORT(void *ch,u_short v);
int IPC_send_INT(void *ch,int v);
int IPC_send_UINT(void *ch,u_int v);
int IPC_send_ULONGINT(void *ch,u_longint v);
int IPC_send_PTR(void *ch,void *v);
int IPC_send_FLOAT(void *ch,float v);
int IPC_send_DOUBLE(void *ch,double v);

int IPC_send_CHARS(void *ch,const char *v_p,u_int n);
int IPC_send_UCHARS(void *ch,const u_char *v_p,u_int n);
int IPC_send_SHORTS(void *ch,const short *v_p,u_int n);
int IPC_send_USHORTS(void *ch,const u_short *v_p,u_int n);
int IPC_send_INTS(void *ch,const int *v_p,u_int n);
int IPC_send_UINTS(void *ch,const u_int *v_p,u_int n);
int IPC_send_ULONGINTS(void *ch,const u_longint *v_p,u_int n);
int IPC_send_PTRS(void *ch,const void **v_p,u_int n);
int IPC_send_FLOATS(void *ch,const float *v_p,u_int n);
int IPC_send_DOUBLES(void *ch,const double *v_p,u_int n);

int IPC_send_STRING(void *ch,const char *s);
int IPC_send_TERMSTRING(void *ch,const char *s,const char *term);
int IPC_send_STRING_ARRAY(void *ch,const char *const *array,int length);
int IPC_send_N_BYTES(void *ch,const void *ptr,u_int nbytes);

    /* C wrappers -- accept */

int IPC_accept_CHAR(void *ch,char *v_p);
int IPC_accept_UCHAR(void *ch,u_char *v_p);
int IPC_accept_SHORT(void *ch,short *v_p);
int IPC_accept_USHORT(void *ch,u_short *v_p);
int IPC_accept_INT(void *ch,int *v_p);
int IPC_accept_UINT(void *ch,u_int *v_p);
int IPC_accept_ULONGINT(void *ch,u_longint *v_p);
int IPC_accept_FLOAT(void *ch,float *v_p);
int IPC_accept_DOUBLE(void *ch,double *v_p);

int IPC_accept_CHARS(void *ch,char *v_p,u_int n);
int IPC_accept_UCHARS(void *ch,u_char *v_p,u_int n);
int IPC_accept_SHORTS(void *ch,short *v_p,u_int n);
int IPC_accept_USHORTS(void *ch,u_short *v_p,u_int n);
int IPC_accept_INTS(void *ch,int *v_p,u_int n);
int IPC_accept_UINTS(void *ch,u_int *v_p,u_int n);
int IPC_accept_ULONGINTS(void *ch,u_longint *v_p,u_int n);
int IPC_accept_FLOATS(void *ch,float *v_p,u_int n);
int IPC_accept_DOUBLES(void *ch,double *v_p,u_int n);

int IPC_accept_CHAR_with_timeout(void *ch,char *v_p,int timeout_in_ms);
int IPC_accept_UCHAR_with_timeout(void *ch,u_char *v_p,int timeout_in_ms);
int IPC_accept_SHORT_with_timeout(void *ch,short *v_p,int timeout_in_ms);
int IPC_accept_USHORT_with_timeout(void *ch,u_short *v_p,int timeout_in_ms);
int IPC_accept_INT_with_timeout(void *ch,int *v_p,int timeout_in_ms);
int IPC_accept_UINT_with_timeout(void *ch,u_int *v_p,int timeout_in_ms);
int IPC_accept_ULONGINT_with_timeout(void *ch,u_longint *v_p,int timeout_in_ms);
int IPC_accept_FLOAT_with_timeout(void *ch,float *v_p,int timeout_in_ms);
int IPC_accept_DOUBLE_with_timeout(void *ch,double *v_p,int timeout_in_ms);

int IPC_accept_CHARS_with_timeout(void *ch,char *v_p,u_int n,int timeout_in_ms);
int IPC_accept_UCHARS_with_timeout(void *ch,u_char *v_p,u_int n,
    int timeout_in_ms);
int IPC_accept_SHORTS_with_timeout(void *ch,short *v_p,u_int n,
    int timeout_in_ms);
int IPC_accept_USHORTS_with_timeout(void *ch,u_short *v_p,u_int n,
    int timeout_in_ms);
int IPC_accept_INTS_with_timeout(void *ch,int *v_p,u_int n,int timeout_in_ms);
int IPC_accept_UINTS_with_timeout(void *ch,u_int *v_p,u_int n,
    int timeout_in_ms);
int IPC_accept_ULONGINTS_with_timeout(void *ch,u_longint *v_p,u_int n,
    int timeout_in_ms);
int IPC_accept_FLOATS_with_timeout(void *ch,float *v_p,u_int n,
    int timeout_in_ms);
int IPC_accept_DOUBLES_with_timeout(void *ch,double *v_p,u_int n,
    int timeout_in_ms);

int IPC_accept_PTR(void *ch,void **v_p);
int IPC_accept_PTR_with_timeout(void *ch,void **v_p,int timeout_in_ms);
int IPC_accept_STRING(void *ch,char **s_p);
int IPC_accept_STRING_with_timeout(void *ch,void **s_p,int timeout_in_ms);
int IPC_accept_TERMSTRING(void *ch,char **s_p,const char *term);
int IPC_accept_TERMSTRING_with_timeout(void *ch,char **s_p,const char *term,
    int timeout_in_ms);
int IPC_accept_STRING_ARRAY(void *ch,char ***array_p);
int IPC_accept_STRING_ARRAY_with_timeout(void *ch,char ***array_p,
    int timeout_in_ms);
int IPC_accept_N_BYTES(void *ch,void **ptr,u_int nbytes);
int IPC_accept_N_BYTES_with_timeout(void *ch,void **ptr,u_int nbytes,
    int timeout_in_ms);

    /* C wrappers -- misc */

int IPC_message_ready(void *ch);
void IPC_flush(void *ch);
void IPC_connect_fails_silently(int flag);
void IPC_suppress_messages(void *ch);
void IPC_unsuppress_messages(void *ch);

#if defined(__cplusplus)
}
#endif

#endif
