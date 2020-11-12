#include <ctype.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/param.h>

int server_ctor_init(int port);
int client_ctor_init(const char *name,int port,const struct timeval *timeout);

int send_bytes_client(int handle,const void *ptr,u_int nbytes);
int send_bytes_server(int handle,const void *ptr,u_int nbytes,
    struct sockaddr *udp_sender);
int send_bytes_work(int handle,const void *ptr,u_int nbytes,
    struct sockaddr *udp_sender);

int accept_bytes_client(int handle,void *ptr,u_int nbytes,
    int read_everything,int timeout_in_ms);
int accept_bytes_server(int handle,void *ptr,u_int nbytes,
    int read_everything,int timeout_in_ms,struct sockaddr *udp_sender);
int accept_bytes_work(int handle,void *ptr,u_int nbytes,
    int read_everything,int timeout_in_ms,struct sockaddr *udp_sender);

int message_ready(int handle,int timeout_in_ms);


int main(int argc, char *argv[])
{
    int handle, port;
    int value1 = 0, value2 = 0, sum;
    struct sockaddr udp_sender;

	/* if acting as server... */

    if (argc == 3 && strcmp(argv[1], "-s") == 0) {

	    /* init server side */

	port = atoi(argv[2]);
	fprintf(stderr, "Server mode, listening on port %d\n", port);
	if ((handle=server_ctor_init(port)) == -1) {
	    fprintf(stderr, "Server can't init\n");
	    exit(1);
	}

	    /* forever... */

	for (;;) {

		/* read values from client */

	    if (accept_bytes_server(handle, &value1, sizeof(value1), 1, -1,
		    &udp_sender) <= 0)
		exit(1);
	    if (accept_bytes_server(handle, &value2, sizeof(value2), 1, -1,
		    &udp_sender) <= 0)
		exit(1);

		/* return sum to client */

	    sum = value1 + value2;
	    if (send_bytes_server(handle, &sum, sizeof(sum), &udp_sender) <= 0)
		exit(1);
	}
    }

	/* if client... */

    else if (argc == 4 && strcmp(argv[1], "-c") == 0) {

	    /* init client side */

	port = atoi(argv[3]);
	fprintf(stderr, "Client mode, sending to %s:%d\n", argv[2], port);
	if ((handle=client_ctor_init(argv[2],port,NULL)) == -1) {
	    fprintf(stderr, "Client can't connect to server\n");
	    exit(1);
	}

	    /* forever... */

	for (;;) {

		/* send values */

	    if (send_bytes_client(handle, &value1, sizeof(value1)) <= 0)
		exit(1);
	    if (send_bytes_client(handle, &value2, sizeof(value2)) <= 0)
		exit(1);
	    if (accept_bytes_client(handle, &sum, sizeof(sum), 1, -1) <= 0)
		exit(1);
	    printf("server says %d + %d = %d\n", value1, value2, sum);
	    value1++;
	    value2 += 2;
	    sleep(1);
	}
    }

	/* if neither, show usage */

    else fprintf(stderr, "Usage: demo [-s portnum | -c hostname portnum]\n");
    exit(0);
}


int accept_bytes_client(int handle,void *ptr,u_int nbytes,
    int read_everything,int timeout_in_ms)
{
    return accept_bytes_work(handle,ptr,nbytes,read_everything,timeout_in_ms,
	NULL);
}


int accept_bytes_server(int handle,void *ptr,u_int nbytes,
    int read_everything,int timeout_in_ms,struct sockaddr *udp_sender)
{
    return accept_bytes_work(handle,ptr,nbytes,read_everything,timeout_in_ms,
	udp_sender);
}


int accept_bytes_work(int handle,void *ptr,u_int nbytes,
    int read_everything,int timeout_in_ms,struct sockaddr *udp_sender)
{
    int status;
    u_int total_read,chunk;
    socklen_t sa_length;

	/* while we haven't yet read everything... */

    total_read = 0;
    while (total_read < nbytes) {

	    /* read up to the amount requested */

	chunk = nbytes-total_read;
	if (message_ready(handle,timeout_in_ms) != 1) {
	    fprintf(stderr, "Timeout!\n");
	    return -1;
	}
	sa_length = sizeof(*udp_sender);
	if (udp_sender == NULL)
	    status = recv(handle,(char *)ptr+total_read,chunk,0);
	else status = recvfrom(handle,(char *)ptr+total_read,chunk,0,udp_sender,
	    &sa_length);

	    /* if we had an error... */

	if (status == -1) {

		/* if we just got a signal, trust that it's been handled
		   and keep going */

	    if (errno == EINTR) continue;

		/* otherwise bail out */

	    fprintf(stderr, "Read failed\n");
	    exit(1);
	}

	    /* update amount read with whatever we read just now */

	total_read += (u_int)status;

	    /* if we read nothing, sender must have died */

	if (status == 0) {
	    fprintf(stderr, "Read got 0 bytes\n");
	    exit(1);
	}

	    /* if we don't care about getting everything (perhaps we weren't
	       sure how many bytes would be there so we requested 1000000)
	       be happy and break out of loop. */

	if (!read_everything) break;
    }

	/* return total number of bytes read, in case we didn't know how
	   many to expect */

    return (int)total_read;
}


int message_ready(int handle,int timeout_in_ms)
{
    fd_set readfds;
    struct timeval timeout_struct;
    int select_return;

	/* if timeout is -1, return 1 rather than blocking here */

    if (timeout_in_ms == -1) return 1;

	/* return 1 if something is available to be read, else return 0
	   or -1 as appropriate */

    FD_SET(handle,&readfds);

    timeout_struct.tv_sec = timeout_in_ms / 1000;
    timeout_struct.tv_usec = (timeout_in_ms % 1000) * 1000;

    select_return = select(32,&readfds,NULL,NULL,&timeout_struct);
    if (select_return > 0) return 1; /* there's something to be read */
    else if (select_return == -1) return -1; /* we got an error */
    else return 0; /* there's nothing to be read */
}


int send_bytes_client(int handle,const void *ptr,u_int nbytes)
{
    return send_bytes_work(handle,ptr,nbytes,NULL);
}


int send_bytes_server(int handle,const void *ptr,u_int nbytes,
    struct sockaddr *udp_sender)
{
    return send_bytes_work(handle,ptr,nbytes,udp_sender);
}


int send_bytes_work(int handle,const void *ptr,u_int nbytes,
    struct sockaddr *udp_sender)
{
    int status;
    int total_written,chunk;

	/* keep track of how much of message we've written so far */

    total_written = 0;

	/* while we still have stuff to write... */

    while (total_written < nbytes) {

	    /* try to send all remaining bytes now */

	chunk = nbytes-total_written;
	if (udp_sender == NULL)
	    status = send(handle,(const char *)ptr+total_written,chunk,0);
	else status = sendto(handle,(const char *)ptr+total_written,chunk,0,
	    udp_sender,sizeof(*udp_sender));

	    /* if we couldn't send any, receiver must have died */

	if (status == 0) {
	    static int wrote_message = 0;
	    fprintf(stderr, "Write returned 0\n");
	    exit(1);
	}

	    /* if we encountered an actual error, and it was a signal, assume
	       it's been handled and continue.  otherwise write a message */

	if (status == -1) {
	    if (errno == EINTR) continue;
	    static int wrote_message = 0;
	    fprintf(stderr, "Write failed\n");
	    exit(1);
	}

	    /* update number of bytes written, and loop back to top to see
	       if we're done */

	total_written += (u_int)status;
    }

	/* return normally */

    return total_written;
}


void handle_alarm(int v) { }


int client_ctor_init(const char *name,int port,const struct timeval *timeout)
{
    u_int len;
    struct sockaddr_in sin;
    struct hostent *hp;
    in_addr_t laddr;
    int sock_fd;
    char msg[132];
    struct itimerval timerval;
    struct sigaction sa;

    /* create the socket */
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
	fprintf(stderr, "Can't connect to server\n");
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
	hp = gethostbyname(name);
	if (hp == NULL) {
	    fprintf(stderr,"Host \"%s\" is unknown\n",name);
	    return -1;
        }
	if (hp->h_length != (int)sizeof(in_addr_t)) {
	    fprintf(stderr,"Unexpected address format returned by "
		"gethostbyname() (%d bytes)\n",hp->h_length);
            return -1;
        }
	(void)memcpy(&laddr, hp->h_addr_list[0], sizeof(in_addr_t));
	sin.sin_addr.s_addr = laddr;
    }

    /* make the connection -- this is wrapped with an alarm to avoid long
       hangs if the connection can't be made */
    if (timeout) {
	memset(&timerval.it_interval, 0, sizeof(timerval.it_interval));
	timerval.it_value = *timeout;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handle_alarm;
	(void)sigaction(SIGALRM, &sa, NULL);
	(void)setitimer(ITIMER_REAL, &timerval, NULL);
    }
    if (connect(sock_fd,(struct sockaddr *)&sin,len) < 0) {
	if (timeout) {
	    memset(&timerval, 0, sizeof(timerval));
	    (void)setitimer(ITIMER_REAL, &timerval, NULL);
	}
	fprintf(stderr, "Can't connect to %s", name);
	close(sock_fd);
	return -1;
    }
    if (timeout) {
	memset(&timerval, 0, sizeof(timerval));
	(void)setitimer(ITIMER_REAL, &timerval, NULL);
    }

    return sock_fd;
}


int server_ctor_init(int port)
{
    struct sockaddr_in sin;
    u_int len;
    int sock;
    int on;

    /* create socket */
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
	fprintf(stderr, "Can't create server socket\n");
        return -1;
    }

    on = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
            (char *) &on, sizeof(on)) == -1) {
	fprintf(stderr, "setsockopt failed\n");
        return -1;
    }

    /* bind address */
    len = sizeof(sin);
    (void)memset(&sin,0,len);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_family = AF_INET;
    sin.sin_port = htons((u_short)port);
    if (bind(sock, (struct sockaddr *)&sin, len) == -1) {
	fprintf(stderr, "bind failed\n");
        return -1;
    }
    return sock;
}
