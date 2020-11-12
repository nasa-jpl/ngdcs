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
#include <sys/stat.h>
#include "ipc.h"


    // sizes of canonical types

const u_int canonical_sizes[DA_ChanBase::NUM_TYPES] = {1,1,2,2,4,4,8,4,8,8};


    // check_type returns -1 with message if argument type is invalid

static int check_type(int type)
{
    if (type < DA_ChanBase::CHAR || type > DA_ChanBase::PTR) {
	(void)sprintf(DA_ChanBase::last_message,
	    "check_type: Internal error (didn't expect data "
	    "item with type %d)\n",type);
	(void)fprintf(stderr,"%s",DA_ChanBase::last_message);
	return -1;
    }
    return 0;
}


    // flip_word switches between big- and little-endian

int flip_word(u_int size,void *p)
{
    u_int i;
    char temp;

    for (i=0;i < size/2;i++) {
	temp = *((char *)p+i);
	*((char *)p+i) = *((char *)p+size-1-i); /*lint !e834 */
	*((char *)p+size-1-i) = temp; 		/*lint !e834 */
    }
    return 0;
}


    // alloc_mem allocates working space for canonization

static int alloc_mem(int type,u_int n,char **out_p,u_int *outsize_p)
{
    char *out;

    out = new char[n * canonical_sizes[type]];
    *out_p = out;
    *outsize_p = n * canonical_sizes[type];
    return 0;
}


    // DA_ChanBase class -- constructor and destructor

FILE *DA_ChanBase::trace_fp = stdout;
char DA_ChanBase::last_message[256] = "No error";
int DA_ChanBase::connect_fails_silently = 0;
int DA_ChanBase::fd_limit = 0;

DA_ChanBase::DA_ChanBase(int eaf)
{
    handle = -1;
    if (init_environment() == -1) {
	failed = 1;
	return;
    }
#if defined(TRACE_IPC)
    if (!trace_fp) {
	char *filename = tracefile_name();
	trace_fp = fopen(filename,"w");
	if (!trace_fp) {
	    (void)sprintf(last_message,"Couldn't open tracing file \"%s\".\n",
		filename);
	    (void)fprintf(stderr,"%s",last_message);
	}
	else set_line_buffering(trace_fp);
    }
#endif /* TRACE_IPC */
    errors_are_fatal = eaf;
    output_message_on_error = 1;
    is_client = 0;
    use_udp = 0;
    memset(&udp_sender, 0, sizeof(udp_sender));
    failed = 0;
}


DA_ChanBase::~DA_ChanBase()
{
    restore_environment();
}


extern "C" void IPC_connect_fails_silently(int flag)
{
    DA_ChanBase::connect_fails_silently = flag;
}


    // DA_WChan class (public DA_ChanBase)

#define SIMPLE_SEND(ctype,type) \
    int DA_WChan::send(ctype v) { unsigned char temp[8]; \
	if (check_type(type) == -1) return -1; \
	if (canonize(&v,sizeof(ctype),temp,canonical_sizes[type]) == -1) \
	    return -1; \
	return send_bytes((void *)temp,canonical_sizes[type]); } \
    int DA_WChan::send(const ctype *v_p,u_int n) { u_int i,outsize; char *out; \
	if (check_type(type) == -1) return -1; \
	if (alloc_mem(type,n,&out,&outsize) == -1) return -1; \
        for (i=0;i < (n);i++) { \
            if (canonize(v_p+i,sizeof(ctype),out+i*canonical_sizes[type], \
		    canonical_sizes[type]) == -1) { \
                delete[] out; return -1;} \
        } \
        if (send_bytes((void *)out,outsize) == -1) {delete[] out; return -1;} \
	delete[] out; return 0; } \
    extern "C" int IPC_send_ ## type(void *ch,ctype v) { \
	return ((DA_SocketChan *)ch)->send(v); } \
    extern "C" int IPC_send_ ## type ## S(void *ch,const ctype *v_p,u_int n) { \
	return ((DA_SocketChan *)ch)->send(v_p,n); }

SIMPLE_SEND(char,CHAR)
SIMPLE_SEND(u_char,UCHAR)
SIMPLE_SEND(short,SHORT)
SIMPLE_SEND(u_short,USHORT)
SIMPLE_SEND(int,INT)
SIMPLE_SEND(u_int,UINT)
SIMPLE_SEND(u_longint,ULONGINT)
SIMPLE_SEND(void *,PTR) /*lint !e818 */
SIMPLE_SEND(float,FLOAT)
SIMPLE_SEND(double,DOUBLE)

#define TRACED_SEND(ctype,printf_code,type) \
    int DA_WChan::send(int indent,const char *descr,ctype v) { \
	if (trace_fp) \
	    (void)fprintf(trace_fp,"OUT %*s%s: " printf_code "\n", \
		4*indent,"",descr,v); \
	return send(v); } \
    int DA_WChan::send(int indent,const char *descr,const ctype *v_p, \
	    u_int n) { \
	if (trace_fp) \
	    (void)fprintf(trace_fp,"OUT %*s%s: %d-element array\n", \
		4*indent,"",descr,(int)n); \
	return send(v_p,n); }

TRACED_SEND(char,"%d",CHAR)
TRACED_SEND(u_char,"%u",UCHAR)
TRACED_SEND(short,"%d",SHORT)
TRACED_SEND(u_short,"%u",USHORT)
TRACED_SEND(int,"%d",INT)
TRACED_SEND(u_int,"%u",UINT)
TRACED_SEND(u_longint,"%lu",ULONGINT)
TRACED_SEND(float,"%g",FLOAT)
TRACED_SEND(double,"%g",DOUBLE)

int DA_WChan::send(const char *s)
{
    int length;

    if (s == NULL) length = -1;
    else length = (int)strlen(s);
    if (send(length) == -1) return -1;
    if (length == -1) return 0;
    else return send_bytes(s,(u_int)length);
}

extern "C" int IPC_send_STRING(void *ch,const char *s)
{
    return ((DA_SocketChan *)ch)->send(s);
}

int DA_WChan::send(const char *s,const char *term)
{
    char *buf;
    int result;

    if (s == NULL) return -1;
    buf = new char[strlen(s)+strlen(term)+1];
    strcpy(buf,s);
    strcat(buf,term);
    result = send_bytes(buf,strlen(buf));
    delete[] buf;
    return result;
}

extern "C" int IPC_send_TERMSTRING(void *ch,const char *s,const char *term)
{
    return ((DA_SocketChan *)ch)->send(s,term);
}

int DA_WChan::send(const char *const *array,int length)
{
    int i;
    if (send(length) == -1) return -1;
    for (i=0;i < length;i++) if (send(array[i]) == -1) return -1;
    return 0;
}

extern "C" int IPC_send_STRING_ARRAY(void *ch,const char *const *array,
    int length)
{
    return ((DA_SocketChan *)ch)->send(array,length);
}

int DA_WChan::send(const void *ptr,u_int nbytes)
{
    return send_bytes((const void *)ptr,nbytes);
}

extern "C" int IPC_send_N_BYTES(void *ch,const void *ptr,u_int nbytes)
{
    return ((DA_SocketChan *)ch)->send(ptr,nbytes);
}

int DA_WChan::send(int indent,const char *descr,const char *s)
{
    int length;

    if (trace_fp) {
	if (s)
	    (void)fprintf(trace_fp,"OUT %*s%s: \"%s\"\n",4*indent,"",descr,s);
	else (void)fprintf(trace_fp,"OUT %*s%s: NULL\n",4*indent,"",descr);
    }
    if (s == NULL) length = -1;
    else length = (int)strlen(s);
    if (send(length) == -1) return -1;
    if (length == -1) return 0;
    else return send_bytes(s,(u_int)length);
}

int DA_WChan::send(int indent,const char *descr,const char *s,const char *term)
{
    char *buf;
    int result;

    if (s == NULL) return -1;
    buf = new char[strlen(s)+strlen(term)+1];
    strcpy(buf,s);
    strcat(buf,term);
    result = send_bytes(buf,strlen(buf));
    if (trace_fp)
	(void)fprintf(trace_fp,"OUT %*s%s: \"%s\"\n",4*indent,"",descr,s);
    delete[] buf;
    return result;
}

int DA_WChan::send(int indent,const char *descr,const char *const *array,
    int length)
{
    int i;
    if (trace_fp)
	(void)fprintf(trace_fp,"OUT %*s%s: %d strings\n",4*indent,"",
	    descr,length);
    if (send(length) == -1) return -1;
    for (i=0;i < length;i++) if (send(0,"string",array[i]) == -1) return -1;
    return 0;
}

int DA_WChan::send(int indent,const char *descr,const void *ptr,u_int nbytes)
{
    if (trace_fp)
	(void)fprintf(trace_fp,"OUT %*s%s: %d bytes\n",4*indent,"",
	    descr,(int)nbytes);
    return send_bytes((const void *)ptr,nbytes);
}


int DA_WChan::send_bytes(const void *,u_int)
{
    (void)sprintf(last_message,"send_bytes: Internal error\n");
    (void)fprintf(stderr,"%s",last_message);
    FATAL_COMM_ERROR
}

int DA_WChan::send(int indent,const char *descr,void *v) {
    if (trace_fp)
	(void)fprintf(trace_fp,"OUT %*s%s: 0x%lx\n",
	    4*indent,"",descr,(unsigned long)v);
    return send(v);
}

int DA_WChan::send(int indent,const char *descr,const void **v_p,u_int n) {
    if (trace_fp)
	(void)fprintf(trace_fp,"OUT %*s%s: %d-element array\n",
	    4*indent,"",descr,(int)n);
    return send(v_p,n);
}

int DA_WChan::intercept_stdout(void) { return -1; }

int DA_WChan::intercept_stderr(void) { return -1; }

DA_WChan::~DA_WChan() { }

void DA_WChan::flush(void) { }


    // DA_RChan class (public DA_ChanBase)

#define SIMPLE_ACCEPT(ctype,type) \
    int DA_RChan::accept(ctype *v_p,u_int n,int timeout_in_ms, \
	    int read_everything) { \
	if (check_type(type) == -1) return -1; \
	int bytes_read,items_read; \
	if (n == 1) { unsigned char temp[8]; \
	    bytes_read = accept_bytes((void *)temp,canonical_sizes[type], \
		read_everything,timeout_in_ms); \
	    if (bytes_read <= 0) return -1; \
	    items_read = bytes_read / (int)canonical_sizes[type]; \
	    if (uncanonize(temp,canonical_sizes[type],v_p,sizeof(ctype)) == -1) \
		return -1; } \
	else { u_int i,insize; char *in; \
	    if (alloc_mem(type,n,&in,&insize) == -1) return -1; \
            bytes_read = accept_bytes((void *)in,insize,read_everything, \
		timeout_in_ms); \
	    if (bytes_read <= 0) { delete[] in; return -1; } \
	    items_read = bytes_read / (int)canonical_sizes[type]; \
            for (i=0;(int)i < items_read;i++) { \
                if (uncanonize(in+i*canonical_sizes[type],canonical_sizes[type], \
		        v_p+i,sizeof(ctype)) == -1) { \
		    delete[] in; return -1; } } \
	    delete[] in; } \
	return items_read; } \
    extern "C" int IPC_accept_ ## type(void *ch,ctype *v_p) { \
	return ((DA_SocketChan *)ch)->accept(v_p); } \
    extern "C" int IPC_accept_ ## type ## S(void *ch,ctype *v_p,u_int n) { \
	return ((DA_SocketChan *)ch)->accept(v_p,n); } \
    extern "C" int IPC_accept_ ## type ## _with_timeout(void *ch,ctype *v_p, \
	    int timeout_in_ms) { \
	return ((DA_SocketChan *)ch)->accept(v_p,1,timeout_in_ms); } \
    extern "C" int IPC_accept_ ## type ## S_with_timeout(void *ch, \
	    ctype *v_p,u_int n,int timeout_in_ms) { \
	return ((DA_SocketChan *)ch)->accept(v_p,n,timeout_in_ms); }

SIMPLE_ACCEPT(char,CHAR)
SIMPLE_ACCEPT(u_char,UCHAR)
SIMPLE_ACCEPT(short,SHORT)
SIMPLE_ACCEPT(u_short,USHORT)
SIMPLE_ACCEPT(int,INT)
SIMPLE_ACCEPT(u_int,UINT)
SIMPLE_ACCEPT(u_longint,ULONGINT)
SIMPLE_ACCEPT(float,FLOAT)
SIMPLE_ACCEPT(double,DOUBLE)

#define TRACED_ACCEPT(ctype,printf_code,type) \
    int DA_RChan::accept(int indent,const char *descr,ctype *v_p,u_int n, \
	    int timeout_in_ms,int read_everything) { \
	int items_read = accept(v_p,n,timeout_in_ms,read_everything); \
	if (items_read <= 0) return -1; \
	if (!trace_fp) ; \
	else if (n == 1) (void)fprintf(trace_fp, \
	    "IN  %*s%s: " printf_code "\n",4*indent,"",descr,*v_p); \
	else (void)fprintf(trace_fp,"IN  %*s%s: %d-element array\n", \
	    4*indent,"",descr,(int)n); \
	return items_read; }

TRACED_ACCEPT(char,"%d",CHAR)
TRACED_ACCEPT(u_char,"%u",UCHAR)
TRACED_ACCEPT(short,"%d",SHORT)
TRACED_ACCEPT(u_short,"%u",USHORT)
TRACED_ACCEPT(int,"%d",INT)
TRACED_ACCEPT(u_int,"%u",UINT)
TRACED_ACCEPT(u_longint,"%lu",ULONGINT)
TRACED_ACCEPT(float,"%g",FLOAT)
TRACED_ACCEPT(double,"%g",DOUBLE)

int DA_RChan::accept(void **v_p,int timeout_in_ms)
{
    unsigned char temp[8];
    if (accept_bytes((void *)temp,canonical_sizes[PTR],1,timeout_in_ms) == -1)
	return -1;
    if (uncanonize(temp,canonical_sizes[PTR],v_p,sizeof(void*)) == -1) return -1;
    return 0;
}

extern "C" int IPC_accept_PTR(void *ch,void **v_p)
{
    return ((DA_SocketChan *)ch)->accept(v_p);
}

extern "C" int IPC_accept_PTR_with_timeout(void *ch,void **v_p,
    int timeout_in_ms)
{
    return ((DA_SocketChan *)ch)->accept(v_p,timeout_in_ms);
}

int DA_RChan::accept(char **s_p,int timeout_in_ms)
{
    int length;
    char *s;

    if (accept(&length,1,timeout_in_ms,1) == -1)
	return -1;
    if (length == -1) *s_p = NULL;
    else {
	s = new char[(u_int)length+1];
	if (accept_bytes((void *)s,(u_int)length,1,timeout_in_ms) == -1)
	    return -1;
	*(s+length) = '\0';
	*s_p = s;
    }
    return 0;
}

extern "C" int IPC_accept_STRING(void *ch,char **s_p)
{
    return ((DA_SocketChan *)ch)->accept(s_p);
}

extern "C" int IPC_accept_STRING_with_timeout(void *ch,void **s_p,
    int timeout_in_ms)
{
    return ((DA_SocketChan *)ch)->accept(s_p,timeout_in_ms);
}

int DA_RChan::accept(char **s_p,const char *term,int timeout_in_ms)
{
    char *s;
    static char buffer[1024];
    int len,termlen,too_long;

    too_long = 0;
    termlen = (int)strlen(term);
    if (use_udp) {
	int bytes_read = accept_bytes(buffer,sizeof(buffer),0,timeout_in_ms);
	if (bytes_read == -1) return -1;
	for (len=0;len <= bytes_read-termlen;len++)
	    if (strncmp(buffer+len,term,(u_int)termlen) == 0)
		break;
	if (len > bytes_read-termlen) too_long = 1;
    }
    else /* TCP */ {
	for (len=0;;) {
	    char c;
	    if (accept(&c,1,timeout_in_ms,1) == -1) return -1;
	    if (len < (int)sizeof(buffer)-1)
		buffer[len++] = c;
	    else too_long = 1;
	    if (len >= termlen && strncmp(buffer+len-termlen,term,
		    (u_int)termlen) == 0) {
		len -= termlen;
		break;
	    }
	}
    }
    if (too_long) return -1;
    buffer[len] = '\0';
    s = new char[(u_int)len+1];
    strcpy(s, buffer);
    *s_p = s;
    return 0;
}

extern "C" int IPC_accept_TERMSTRING(void *ch,char **s_p,const char *term)
{
    return ((DA_SocketChan *)ch)->accept(s_p,term);
}

extern "C" int IPC_accept_TERMSTRING_with_timeout(void *ch,char **s_p,
    const char *term,int timeout_in_ms)
{
    return ((DA_SocketChan *)ch)->accept(s_p,term,timeout_in_ms);
}

int DA_RChan::accept(char ***array_p,int timeout_in_ms)
{
    u_int length,i;

    if (accept(&length,1,timeout_in_ms,1) == -1)
	return -1;
    if (!length) *array_p = NULL;
    else *array_p = new char *[length];
    for (i=0;i < length;i++)
	if (accept((*array_p)+i) == -1) return -1;
    return 0;
}

extern "C" int IPC_accept_STRING_ARRAY(void *ch,char ***array_p)
{
    return ((DA_SocketChan *)ch)->accept(array_p);
}

extern "C" int IPC_accept_STRING_ARRAY_with_timeout(void *ch,char ***array_p,
    int timeout_in_ms)
{
    return ((DA_SocketChan *)ch)->accept(array_p,timeout_in_ms);
}

int DA_RChan::accept(void **ptr,u_int nbytes,int timeout_in_ms,
    int read_everything)
{
    *ptr = new char[nbytes];
    return accept_bytes((void *)*ptr,nbytes,read_everything,timeout_in_ms);
}

extern "C" int IPC_accept_N_BYTES(void *ch,void **ptr,u_int nbytes)
{
    return ((DA_SocketChan *)ch)->accept(ptr,nbytes,-1);
}

extern "C" int IPC_accept_N_BYTES_with_timeout(void *ch,void **ptr,u_int nbytes,
    int timeout_in_ms)
{
    return ((DA_SocketChan *)ch)->accept(ptr,nbytes,timeout_in_ms);
}

int DA_RChan::accept(int indent,const char *descr,void **v_p,int timeout_in_ms)
{
    if (accept(v_p,1,timeout_in_ms,1) == -1)
	return -1;
    if (trace_fp)
	(void)fprintf(trace_fp,"IN  %*s%s: 0x%lx\n",4*indent,"",descr,
	    (unsigned long)*v_p);
    return 0;
}

int DA_RChan::accept(int indent,const char *descr,char **s_p,int timeout_in_ms)
{
    int length;
    char *s;

    if (accept(&length,1,timeout_in_ms,1) == -1)
	return -1;
    if (length == -1) {
	*s_p = NULL;
	if (trace_fp)
	    (void)fprintf(trace_fp,"IN  %*s%s: NULL\n",4*indent,"", descr);
    }
    else {
	s = new char[(u_int)length+1];
	if (accept_bytes((void *)s,(u_int)length,1,timeout_in_ms) == -1)
	    return -1;
	*(s+length) = '\0';
	*s_p = s;
	if (trace_fp)
	    (void)fprintf(trace_fp,"IN  %*s%s: %s\n",4*indent,"",descr,s);
    }
    return 0;
}

int DA_RChan::accept(int indent,const char *descr,char **s_p,const char *term,
    int timeout_in_ms)
{
    char *s;
    static char buffer[1024];
    int len,termlen,too_long;

    too_long = 0;
    termlen = (int)strlen(term);
    if (use_udp) {
	int bytes_read = accept_bytes(buffer,sizeof(buffer),0,timeout_in_ms);
	if (bytes_read == -1) return -1;
	for (len=0;len <= bytes_read-termlen;len++)
	    if (strncmp(buffer+len,term,(u_int)termlen) == 0)
		break;
	if (len > bytes_read-termlen) too_long = 1;
    }
    else /* TCP */ {
	for (len=0;;) {
	    char c;
	    if (accept(&c,1,timeout_in_ms,1) == -1) return -1;
	    if (len < (int)sizeof(buffer)-1)
		buffer[len++] = c;
	    else too_long = 1;
	    if (len >= termlen && strcmp(buffer+len-termlen, term) == 0)
		break;
	}
    }
    if (too_long) return -1;
    buffer[len] = '\0';
    s = new char[(u_int)len+1];
    strcpy(s, buffer);
    *s_p = s;
    if (trace_fp)
	(void)fprintf(trace_fp,"IN  %*s%s: %s\n",4*indent,"",descr,s);
    return 0;
}

int DA_RChan::accept(int indent,const char *descr,char ***array_p,
    int timeout_in_ms)
{
    u_int length,i;

    if (accept(&length,1,timeout_in_ms,1) == -1)
	return -1;
    if (!length) *array_p = NULL;
    else *array_p = new char *[length];
    if (trace_fp)
	(void)fprintf(trace_fp,"IN  %*s%s: %d strings\n",4*indent,"",
	    descr,(int)length);
    for (i=0;i < length;i++)
	if (accept((*array_p)+i) == -1) return -1;
    return 0;
}

int DA_RChan::accept(int indent,const char *descr,void **ptr,u_int nbytes,
    int timeout_in_ms,int read_everything)
{
    if (trace_fp)
	(void)fprintf(trace_fp,"IN  %*s%s: %d bytes\n",4*indent,"",descr,
	    (int)nbytes);
    *ptr = new char[nbytes];
    return accept_bytes((void *)*ptr,nbytes,read_everything,timeout_in_ms);
}


int DA_RChan::accept_bytes(void *,u_int,int,int)
{
    (void)sprintf(last_message,"accept_bytes: Internal error\n");
    (void)fprintf(stderr,"%s",last_message);
    FATAL_COMM_ERROR
}


DA_RChan::~DA_RChan() { }


    // DA_RChan -- support routines

int DA_RChan::message_ready(int)
{
    (void)sprintf(last_message,
	"message_ready: Internal error (no channel type)\n");
    (void)fprintf(stderr,"%s",last_message);
    FATAL_COMM_ERROR
}
