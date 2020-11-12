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

   Please do not redistribute this file separate from the NGDCS source
   distribution.  For questions regarding this software, please contact the
   author, Alan Mazer, alan.s.mazer@jpl.nasa.gov */


#include <ipc.h>

#define MS_PER_SECOND		1000
#define USECS_PER_MS		1000

#define FTP_CMD_OKAY		200
#define FTP_LOGIN_OKAY		300

#define MAXERRORLEN		256


class FTPConnection
{
public:
    typedef struct {
	char name[MAXPATHLEN];
	struct tm time;
	int size;
    } ftpDirEntry_t;
protected:

	/* FTP variables */

    DA_SocketChan *m_ftpConn;
    int m_timeoutInMs;
    void *m_FTPClientData;
    int (*m_FTPErrorHandler)(void *clientData, const char *msg);
    void (*m_FTPLogHandler)(void *clientData, const char *msg);

	/* support routines */

    int FTPSendCommand(const char *command, const char *description);
    int FTPValidateResponse(const char *description, int expectedResponse,
	char **actualResponse_p);
    int FTPConvertResponseToHostAndPort(const char *response, char **hostname_p,
	int *port_p);
public:
    int failed;
    FTPConnection(const char *host, int port, const char *user,
	const char *password, int timeoutInMs,
	int (*errorHandler)(void *, const char *msg),
	void (*logHandler)(void *, const char *msg), void *clientData);
    int FTPChdir(const char *dir);
    int FTPList(ftpDirEntry_t **datasets_p, int *nDatasets_p);
    int FTPGetFile(const char *remoteName, const char *localName);
    int FTPDeleteFile(const char *remoteName);
    ~FTPConnection();
};
