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


#define MAX_NAMESTR_LEN    (128 + 1)
#define MAX_DATESTR_LEN    (1024 + 1)
#define MAX_ERROR_LEN      (100 + 1)
#define MAX_TIME_LEN	   (30+1)

#define OBC_AUTO           0  /* OBC positions and frame tags */
#define OBC_DARK1          1
#define OBC_SCIENCE        2
#define OBC_DARK2          3
#define OBC_MEDIUM         4
#define OBC_BRIGHT         5
#define OBC_LASER          6
#define OBC_IDLE           7

#define MOUNT_AUTO	   0  /* mount modes */
#define MOUNT_IDLE	   1
#define MOUNT_ACTIVE   	   2

#define SHUTTER_POS_UNKNOWN	0
#define SHUTTER_POS_CLOSED	1
#define SHUTTER_POS_OPEN	2

#define SECONDS_PER_MINUTE	60
#define SECONDS_PER_HOUR	3600

#define BYTES_PER_MB		(1024 * 1024)

#define NOMINAL_FRAME_HEIGHT    481

#define PI			3.1415926

#define LOCK_FILE		    "/tmp/ngdcs-lock"

#define RECORDING_FLIGHTLINE	0
#define RECORDING_ALLGPS	1
#define RECORDING_ALLPPS	2
#define RECORDING_DIAGS		3
#define RECORDING_LOG		4

typedef struct {
    int recordingType;
    char imagefile[MAXPATHLEN];
    char imagepath[MAXPATHLEN];
    char imagehdrfile[MAXPATHLEN];
    char imagehdrpath[MAXPATHLEN];
    char gpsfile[MAXPATHLEN];
    char gpspath[MAXPATHLEN];
    char ppsfile[MAXPATHLEN];
    char ppspath[MAXPATHLEN];
    char diagsfile[MAXPATHLEN];
    char diagspath[MAXPATHLEN];
    char logfile[MAXPATHLEN];
    char logpath[MAXPATHLEN];
    time_t startTime;
    time_t endTime;
    time_t duration;
} recording_t;
