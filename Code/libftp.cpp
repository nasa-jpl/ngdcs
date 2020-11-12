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


#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/param.h>
#include "libftp.h"

extern const char *const libftpDate = "$Date: 2014/01/17 23:51:50 $";


FTPConnection::FTPConnection(const char *host, int port, const char *user,
    const char *password, int timeoutInMs, int (*eh)(void *, const char *msg),
    void (*lh)(void *, const char *msg), void *clientData)
{
    struct timeval timeout;
    char ftpCmd[MAXPATHLEN];
    char warningMsg[MAXERRORLEN];

	/* init */

    failed = 0;
    m_timeoutInMs = timeoutInMs;
    m_FTPClientData = clientData;
    m_FTPErrorHandler = eh;
    m_FTPLogHandler = lh;

	/* try to connect */

    timeout.tv_sec = m_timeoutInMs / MS_PER_SECOND;
    timeout.tv_usec = (m_timeoutInMs % MS_PER_SECOND) * USECS_PER_MS;
    DA_ChanBase::connect_fails_silently = 1;
    m_ftpConn = new DA_SocketChan(host, port, 0, 0, &timeout);
    if (m_ftpConn->failed) {
	delete m_ftpConn;
	m_ftpConn = NULL;
	sprintf(warningMsg,
	    "Can't connect to %s, port %d, within %.1f sec timeout", host, port,
	    m_timeoutInMs / (double)MS_PER_SECOND);
	(void)(*m_FTPErrorHandler)(m_FTPClientData, warningMsg);
	failed = 1;
	return;
    }
    m_ftpConn->suppress_messages();

    if (FTPValidateResponse("login", FTP_CMD_OKAY, NULL) == -1) {
	failed = 1;
	return;
    }

	/* pass in the user name */

    (void)sprintf(ftpCmd, "user %s", user);
    if (FTPSendCommand(ftpCmd, "login") == -1 ||
	    FTPValidateResponse("login", FTP_LOGIN_OKAY, NULL) == -1) {
	failed = 1;
	return;
    }

	/* pass in the password */

    (void)sprintf(ftpCmd, "pass %s", password);
    if (FTPSendCommand(ftpCmd, "login") == -1 ||
	    FTPValidateResponse("login", FTP_CMD_OKAY, NULL) == -1) {
	failed = 1;
	return;
    }
}


FTPConnection::~FTPConnection()
{
    if (m_ftpConn != NULL) {
	delete m_ftpConn; /*lint !e1551 exception thrown */
	m_ftpConn = NULL;
    }
    m_FTPClientData = NULL;
}


int FTPConnection::FTPChdir(const char *dir)
{
    char *cmd;
    int status;

    cmd = new char[strlen(dir)+10];
    sprintf(cmd, "cwd %s", dir);
    status = FTPSendCommand(cmd, "directory change");
    if (status != -1)
	status = FTPValidateResponse("directory change", FTP_CMD_OKAY, NULL);
    delete[] cmd;
    return status;
}


int FTPConnection::FTPList(ftpDirEntry_t **datasets_p, int *nDatasets_p)
{
    int i, j, n;
    ftpDirEntry_t temp;
    u_int maxRecordings;
    ftpDirEntry_t *datasets;
    char *response;
    char *datahost;
    int dataport;
    ftpDirEntry_t *newDatasets;
    char ftpCmd[MAXPATHLEN*3];
    char *subresponse, *timeparse;
    int size;
    struct tm t;
    char msg[MAXERRORLEN];
    DA_SocketChan *ch;

	/* allocate initial storage.  we'll grow this if we have to */

    maxRecordings = 10;
    datasets = new ftpDirEntry_t[maxRecordings];
    n = 0;

	/* set up empty return in case we fail midway through */

    *datasets_p = NULL;
    *nDatasets_p = 0;

	/* force ascii data transfers */

    if (FTPSendCommand("type a", "type command") == -1) return -1;
    if (FTPValidateResponse("type command", FTP_CMD_OKAY, NULL) == -1)
	return -1;

	/* enter passive mode */

    if (FTPSendCommand("pasv", "passive-mode entry") == -1) return -1;
    if (FTPValidateResponse("passive-mode entry", FTP_CMD_OKAY,
	    &response) == -1)
	return -1;

	/* get host and port */

    if (FTPConvertResponseToHostAndPort(response, &datahost, &dataport) == -1) {
	delete m_ftpConn;
	m_ftpConn = NULL;
	return -1;
    }

	/* open socket on specified port -- timeout isn't necessary here
	   since we know the host is responsive */

    ch = new DA_SocketChan(datahost, dataport, 0);
    if (ch->failed) {
	delete ch;
	sprintf(msg, "Can't make FTP data connection");
	(void)(*m_FTPErrorHandler)(m_FTPClientData, msg);
	return -1;
    }
    ch->suppress_messages();

	/* request a directory */

    if (FTPSendCommand("nlst", "list command") == -1) {
	delete ch;
	return -1;
    }

	/* process each response */

    while (ch->accept(&response, "\r\n", m_timeoutInMs) != -1) {

	    /* show what we've got */

	(*m_FTPLogHandler)(m_FTPClientData, response);

	    /* get the size and creation date */

	(void)sprintf(ftpCmd, "size %s", response);
	if (FTPSendCommand(ftpCmd, "size command") == -1 ||
		FTPValidateResponse("size command", FTP_CMD_OKAY,
		    &subresponse) == -1) {
	    delete[] response;
	    delete ch;
	    return -1;
	}
	size = strtol(subresponse+4, NULL, 0);
	delete[] subresponse;

	(void)sprintf(ftpCmd, "mdtm %s", response);
	if (FTPSendCommand(ftpCmd, "date check") == -1 ||
		FTPValidateResponse("date check", FTP_CMD_OKAY,
		    &subresponse) == -1) {
	    delete[] response;
	    delete ch;
	    return -1;
	}
	timeparse = strptime(subresponse+4, "%Y%m%d%H%M%S", &t);
	delete[] subresponse;
	if (timeparse == NULL || *timeparse != '\0') {
	    (void)sprintf(msg,
		"Date attached to file \"%s\" is of odd form and may be "
		    "incorrect in listing",
		response);
	    (void)(*m_FTPErrorHandler)(m_FTPClientData, msg);
	}

	    /* expand internal storage, if we don't have enough room for this */

	if ((unsigned)n == maxRecordings) {
	    maxRecordings += 10;

	    newDatasets = new ftpDirEntry_t[maxRecordings];
	    memcpy(newDatasets, datasets, (unsigned)n * sizeof(ftpDirEntry_t));
	    delete[] datasets;
	    datasets = newDatasets;
	}

	    /* fill in descriptor */

	strcpy(datasets[n].name, response); /*lint !e661 !e662 out-of-bounds */
	datasets[n].time = t;		    /*lint !e661 !e662 out-of-bounds */
	datasets[n].size = size;	    /*lint !e661 !e662 out-of-bounds */

	    /* note that we've found another recording */

	n++;
	
	    /* free temp space */

	delete[] response;
    }

	/* clean up */

    delete ch;

	/* make sure entries are sorted */

    for (i=0;i < n-1;i++)
	for (j=i+1;j < n;j++)
	    if (strcmp(datasets[i].name, datasets[j].name) > 0) {
		temp = datasets[i];
		datasets[i] = datasets[j];
		datasets[j] = temp;
	    }

	/* return our list of flightlines to caller */

    *datasets_p = datasets;
    *nDatasets_p = n;
    return 0;
}


int FTPConnection::FTPGetFile(const char *remoteName, const char *localName)
{
    char *response, *datahost;
    int dataport;
    char warningMsg[MAXPATHLEN+MAXERRORLEN];
    char ftpCmd[MAXPATHLEN*3];
    FILE *fp;
    u_char bytes[1024];
    int nbytes;
    char msg[MAXERRORLEN];
    DA_SocketChan *ch;

	/* force binary data transfers */

    if (FTPSendCommand("type i", "type command") == -1) return -1;
    if (FTPValidateResponse("type command", FTP_CMD_OKAY, NULL) == -1)
	return -1;

	/* enter passive mode */

    if (FTPSendCommand("pasv", "passive-mode entry") == -1) return -1;
    if (FTPValidateResponse("passive-mode entry", FTP_CMD_OKAY,
	    &response) == -1)
	return -1;

	/* get host and port */

    if (FTPConvertResponseToHostAndPort(response, &datahost,
	    &dataport) == -1)
	return -1;

	/* open socket on specified port -- timeout isn't necessary
	   here since we know the host is responsive */

    ch = new DA_SocketChan(datahost, dataport, 0);
    if (ch->failed) {
	delete ch;
	sprintf(warningMsg, "Can't make FTP data connection");
	(void)(*m_FTPErrorHandler)(m_FTPClientData, warningMsg);
	return -1;
    }
    ch->suppress_messages();

	/* request file */

    (void)sprintf(ftpCmd, "retr %s", remoteName);
    if (FTPSendCommand(ftpCmd, "file copy") == -1 ||
	    FTPValidateResponse("file copy", FTP_CMD_OKAY, &response) == -1) {
	delete ch;
	return -1;
    }

	/* open local copy */

    fp = fopen(localName, "wb");
    if (fp == NULL) {
	sprintf(msg, "Can't create local file \"%s\".", localName);
	(void)(*m_FTPErrorHandler)(m_FTPClientData, msg);
	delete ch;
	return -1;
    }

	/* save each response */

    while ((nbytes=ch->accept_bytes(bytes, sizeof(bytes), 0,
	    m_timeoutInMs)) > 0) {
	if ((int)fwrite(bytes, 1, (unsigned)nbytes, fp) != nbytes) {
	    sprintf(msg, "Write to local file failed.");
	    (void)(*m_FTPErrorHandler)(m_FTPClientData, msg);
	    fclose(fp);
	    (void)unlink(localName);
	    delete ch;
	    return -1;
	}
    }

	/* close local file */

    (void)fclose(fp);

	/* we're done */

    delete ch;
    return 0;
}


int FTPConnection::FTPDeleteFile(const char *remoteName)
{
    char *response;
    char ftpCmd[MAXPATHLEN*3];

    (void)sprintf(ftpCmd, "dele %s", remoteName);
    if (FTPSendCommand(ftpCmd, "file delete") == -1)
	return -1;
    if (FTPValidateResponse("file delete", FTP_CMD_OKAY, &response) == -1)
	return -1;
    return 0;
}


int FTPConnection::FTPConvertResponseToHostAndPort(const char *response,
    char **hostname_p, int *port_p)
{
    char msg[MAXERRORLEN];
    char *temp;
    const char *lparenPos;
    char *rparenPos;
    char *commaPos;
    int i, port;

	/* find the ( in the response.  this is the start of the passive
	   IP we want to use */

    lparenPos = strchr(response, '(');
    if (lparenPos == NULL) {
	(void)sprintf(msg, 
	    "FTP server had odd response to pasv command (%s); can't continue",
	    response);
	(void)(*m_FTPErrorHandler)(m_FTPClientData, msg);
	return -1;
    }

	/* make a copy of everything after that paren */

    temp = new char[strlen(lparenPos+1)+1];
    strcpy(temp, lparenPos+1);

	/* replace each of the first three commas with periods.  replace the
	   last with a null byte, to mark the end of the hostname */

    commaPos = strchr(temp, ',');
    for (i=0;;i++) {
	if (commaPos == NULL) {
	    (void)sprintf(msg, 
		"FTP server had odd response to pasv command (%s); "
		    "can't continue",
		response);
	    (void)(*m_FTPErrorHandler)(m_FTPClientData, msg);
	    return -1;
	}
	if (i < 3) {
	    *commaPos = '.';
	    commaPos = strchr(commaPos+1, ',');
	}
	else {
	    *commaPos = '\0';
	    break;
	}
    }

	/* save the extracted IP address for the caller */

    *hostname_p = temp;

	/* we should now be at the start (almost) of our port number */

    port = strtol(commaPos+1, &commaPos, 0) * 256;
    if (*commaPos != ',') {
	(void)sprintf(msg, 
	    "FTP server had odd response to pasv command (%s); can't continue",
	    response);
	(void)(*m_FTPErrorHandler)(m_FTPClientData, msg);
	return -1;
    }
    port += strtol(commaPos+1, &rparenPos, 0);
    if (*rparenPos != ')') {
	(void)sprintf(msg, 
	    "FTP server had odd response to pasv command (%s); can't continue",
	    response);
	(void)(*m_FTPErrorHandler)(m_FTPClientData, msg);
	return -1;
    }
    *port_p = port;
    return 0;
}


int FTPConnection::FTPSendCommand(const char *command, const char *description)
{
    char msg[MAXERRORLEN];
    char *copy, *p;

	/* send specified command */

    if (strncmp(command, "pass ", 5) != 0)
	(*m_FTPLogHandler)(m_FTPClientData, command);
    else { /*lint --e{415,661} p out-of-bounds */
	copy = new char[strlen(command)+1];
	strcpy(copy, command);
	for (p=copy+5;*p != '\0';p++)
	    *p = '*';
	(*m_FTPLogHandler)(m_FTPClientData, copy);
	delete[] copy;
    }
    if (m_ftpConn->send(command, "\r\n") == -1) {
	(void)sprintf(msg, "FTP connection dropped (%s failed)", description);
	(void)(*m_FTPErrorHandler)(m_FTPClientData, msg);
	delete m_ftpConn;
	m_ftpConn = NULL; /* force reset */
	return -1;
    }
    return 0;
}


int FTPConnection::FTPValidateResponse(const char *description, 
    int expectedResponse, char **actualResponse_p)
{
    char msg[MAXERRORLEN];
    int success;
    char *response;

	/* init */

    success = 0;
    if (actualResponse_p)
	*actualResponse_p = NULL;

	/* look for success response */

    while (m_ftpConn->accept(&response, "\r\n", m_timeoutInMs) != -1) {
	(*m_FTPLogHandler)(m_FTPClientData, response);
	if (isdigit(*response) &&
		strtol(response, NULL, 0) / 100 == expectedResponse / 100) {
	    success = 1;
	    if (actualResponse_p)
		*actualResponse_p = response;
	    else delete[] response;
	}
	else delete[] response;
    }
    if (!success) {
	(void)sprintf(msg, 
	    "FTP server returned error response (%s failed)", description);
	(void)(*m_FTPErrorHandler)(m_FTPClientData, msg);
	delete m_ftpConn;
	m_ftpConn = NULL; /* force reset */
	return -1;
    }
    return 0;
}
