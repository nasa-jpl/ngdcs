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


#include <assert.h>
#include <dirent.h>
#include <math.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/param.h>
#include <gtkmm.h>
#include "lists.h"
#include "main.h"
#include "framebuf.h"
#include "plotting.h"
#include "appFrame.h"
#include "displayTab.h"
#include "recordingsTab.h"
#include "settingsBlock.h"
#include "settingsTab.h"

extern const char *const recordingsTabDate = "$Date: 2015/08/19 16:02:52 $";


RecordingsTab::RecordingsTab(SettingsBlock *sb, DisplayTab *dt)
{
    int i, nRecordings;
    recording_t *recordings;

	/* init member variables */

    m_block = sb;
    m_displayTab = dt;
    m_numFlightLines = 0;

        /* scan the data directories */

    scanDirs(m_block->currentProductRootDir(), &recordings, &nRecordings);

	/* construct the GUI */

    set_border_width(10);
    set_spacing(25);

	/* add columns to the record -- some columns are for internal
	   use and won't be displayed */

    m_record.add(m_typeColumn);
    m_record.add(m_imagepathColumn);
    m_record.add(m_imagefileColumn);
    m_record.add(m_imagehdrpathColumn);
    m_record.add(m_imagehdrfileColumn);
    m_record.add(m_gpspathColumn);
    m_record.add(m_gpsfileColumn);
    m_record.add(m_ppspathColumn);
    m_record.add(m_ppsfileColumn);
    m_record.add(m_diagspathColumn);
    m_record.add(m_diagsfileColumn);
    m_record.add(m_logpathColumn);
    m_record.add(m_logfileColumn);
    m_record.add(m_startTimeColumn);
    m_record.add(m_endTimeColumn);
    m_record.add(m_durationColumn);
    m_record.add(m_durationsecsColumn);
    m_record.add(m_recordingTypeColumn);

	/* create model around the record */

    m_model = Gtk::ListStore::create(m_record);
    m_list.set_model(m_model);

	/* fill in the columns */

    for (i=0;i < nRecordings;i++)
	addRecording(&recordings[i]);

	/* free up recordings list -- this is tricky to maintain, with
	   additions also made through Record button, so I'm going to keep
	   it simple and not try to keep it updated */

    delete[] recordings;

	/* add columns to the list */

    (void)m_list.append_column("Type", m_typeColumn);
    (void)m_list.append_column("Start time", m_startTimeColumn);
    (void)m_list.append_column("End time", m_endTimeColumn);
    (void)m_list.append_column("Duration", m_durationColumn);

    Gtk::TreeViewColumn *col = m_list.get_column(0);
    col->set_min_width(80);
    col = m_list.get_column(1);
    col->set_min_width(170);
    col = m_list.get_column(2);
    col->set_min_width(170);

	/* catch selections */

    m_selection = m_list.get_selection();
    m_selection->set_mode(Gtk::SELECTION_MULTIPLE);
    (void)m_selection->signal_changed().connect(sigc::mem_fun(*this,
	&RecordingsTab::onSelection));

	/* put the list in a scrolling window and add the window */

    m_window.add(m_list);
    m_window.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    pack_start(m_window);

	/* add the delete button */

    m_deleteButton.set_label("Delete");
    m_deleteButton.set_sensitive(false);
    (void)m_deleteButton.signal_clicked().connect(sigc::mem_fun(*this,
	&RecordingsTab::onDeleteButton));
    m_buttonBox.set_layout(Gtk::BUTTONBOX_START);
    m_buttonBox.add(m_deleteButton);
    pack_start(m_buttonBox, Gtk::PACK_SHRINK);
}


void RecordingsTab::scanDirs(char *productRootDir, recording_t **recordings_p,
    int *nRecordings_p) const
{
    int i, j, n, maxRecordings, len, recordingType;
    recording_t temp;
    DIR *rootDir, *subDir;
    char errorMsg[MAX_ERROR_LEN], *msg, *ct, *p, *path;
    recording_t *newRecordings;
    struct dirent *rootEntry, *subdirEntry;
    char subdirPath[MAXPATHLEN];
    struct stat statbuf;
    char gpsname[MAXPATHLEN], ppsname[MAXPATHLEN], diagsname[MAXPATHLEN],
	logname[MAXPATHLEN];
    recording_t *recordings;
    SL_List<char *> imageOrphans, otherOrphans;
    struct tm tm_struct;

	/* allocate initial storage.  we'll grow this if we have to */

    maxRecordings = 10;
    recordings = new recording_t[(unsigned)maxRecordings];
    n = 0;

	/* open the root directory */

    if ((rootDir=opendir(productRootDir)) == NULL) {
	sprintf(errorMsg, "Can't access data directory \"%s\".",
	    productRootDir);
	m_displayTab->warn(errorMsg, 
	    "Is directory correct on Settings tab?");
	*recordings_p = recordings;
	*nRecordings_p = n;
	return;
    }

	/* for each subdir... */

    while ((rootEntry=readdir(rootDir))) { /*lint --e{661,662} */

	    /* if this isn't a subdirectory, move on to the next entry */

	if (rootEntry->d_type != DT_DIR)
	    continue;

	    /* if it's a lost+found directory, ignore it */

	if (!strcmp(rootEntry->d_name, "lost+found"))
	    continue;

	    /* ignore entries for current and parent directories */

	if (!strcmp(rootEntry->d_name, ".") || !strcmp(rootEntry->d_name, ".."))
	    continue;

	    /* open the subdirectory */

	(void)sprintf(subdirPath, "%s/%s", productRootDir, rootEntry->d_name);
	if ((subDir=opendir(subdirPath)) == NULL) {
	    sprintf(errorMsg, "Can't access data directory \"%s\".",
		subdirPath);
	    m_displayTab->warn(errorMsg,
		"Recordings list may be incomplete.");
	    continue;
	}

	    /* add each image file and end-to-end file in directory into our
               arrays, expanding storage if necessary */

	while ((subdirEntry=readdir(subDir))) { /*lint --e{661,662} */

		/* if entry isn't image or end-to-end file, skip ahead for
		   now */

	    len = strlen(subdirEntry->d_name);
	    if (len >= (int)strlen("_raw") &&
		    strcmp(subdirEntry->d_name+len-strlen("_raw"),
			"_raw") == 0)
		recordingType = RECORDING_FLIGHTLINE;
	    else if (len >= (int)strlen("_allgps") &&
		    strcmp(subdirEntry->d_name+len-strlen("_allgps"),
			"_allgps") == 0)
		recordingType = RECORDING_ALLGPS;
	    else if (len >= (int)strlen("_allpps") &&
		    strcmp(subdirEntry->d_name+len-strlen("_allpps"),
			"_allpps") == 0)
		recordingType = RECORDING_ALLPPS;
	    else if (len >= (int)strlen("_diags.csv") &&
		    strcmp(subdirEntry->d_name+len-strlen("_diags.csv"),
			"_diags.csv") == 0)
		recordingType = RECORDING_DIAGS;
	    else if (len >= (int)strlen("_log.txt") &&
		    strcmp(subdirEntry->d_name+len-strlen("_log.txt"),
			"_log.txt") == 0)
		recordingType = RECORDING_LOG;
	    else continue;

		/* expand internal storage, if we don't have enough room for
   		   this */

	    if (n == maxRecordings) {
		maxRecordings += 10;

		newRecordings = new recording_t[(unsigned)maxRecordings];
		memcpy(newRecordings, recordings,
		    (u_int)n * sizeof(recording_t));
		delete[] recordings;
		recordings = newRecordings;
	    }

		/* save filenames */

	    memset(&recordings[n], 0, sizeof(recording_t));
	    if (recordingType == RECORDING_FLIGHTLINE) {
		strcpy(recordings[n].imagefile, subdirEntry->d_name);

		strcpy(recordings[n].imagehdrfile, subdirEntry->d_name);
		strcat(recordings[n].imagehdrfile, ".hdr");

		strcpy(gpsname, subdirEntry->d_name);
		strcpy(gpsname + strlen(gpsname) - strlen("_raw"), "_gps");
		strcpy(recordings[n].gpsfile, gpsname);

		strcpy(ppsname, subdirEntry->d_name);
		strcpy(ppsname + strlen(ppsname) - strlen("_raw"), "_pps");
		strcpy(recordings[n].ppsfile, ppsname);
	    }
	    else if (recordingType == RECORDING_ALLGPS) {
		strcpy(gpsname, subdirEntry->d_name);
		strcpy(recordings[n].gpsfile, gpsname);
	    }
	    else if (recordingType == RECORDING_ALLPPS) {
		strcpy(ppsname, subdirEntry->d_name);
		strcpy(recordings[n].ppsfile, ppsname);
	    }
	    else if (recordingType == RECORDING_DIAGS) {
		strcpy(diagsname, subdirEntry->d_name);
		strcpy(recordings[n].diagsfile, diagsname);
	    }
	    else /* recordingType == RECORDING_LOG */ {
		strcpy(logname, subdirEntry->d_name);
		strcpy(recordings[n].logfile, logname);
	    }

		/* save full paths */

	    if (recordingType == RECORDING_FLIGHTLINE) {
		path = recordings[n].imagepath;
		(void)sprintf(recordings[n].imagepath, "%s/%s/%s",
		    productRootDir, rootEntry->d_name,
		    subdirEntry->d_name);
		(void)sprintf(recordings[n].imagehdrpath, "%s.hdr",
		    recordings[n].imagepath);
		(void)sprintf(recordings[n].gpspath, "%s/%s/%s", productRootDir,
		    rootEntry->d_name, gpsname);
		(void)sprintf(recordings[n].ppspath, "%s/%s/%s", productRootDir,
		    rootEntry->d_name, ppsname);
	    }
	    else if (recordingType == RECORDING_ALLGPS) {
		path = recordings[n].gpspath;
		(void)sprintf(recordings[n].gpspath, "%s/%s/%s", productRootDir,
		    rootEntry->d_name, gpsname);
	    }
	    else if (recordingType == RECORDING_ALLPPS) {
		path = recordings[n].ppspath;
		(void)sprintf(recordings[n].ppspath, "%s/%s/%s", productRootDir,
		    rootEntry->d_name, ppsname);
	    }
	    else if (recordingType == RECORDING_DIAGS) {
		path = recordings[n].diagspath;
		(void)sprintf(recordings[n].diagspath, "%s/%s/%s",
		    productRootDir, rootEntry->d_name,
		    diagsname);
	    }
	    else /* recordingType == RECORDING_LOG */ {
		path = recordings[n].logpath;
		(void)sprintf(recordings[n].logpath, "%s/%s/%s", productRootDir,
		    rootEntry->d_name, logname);
	    }

		/* note recording type */

	    recordings[n].recordingType = recordingType;

		/* get start and end times, and calculate duration.  on OS X
		   this could be easy because the filesystem includes a
		   birthtime but for Linux we need to get the start time from
		   the filename.  looking at timetags on the frames is
		   awkward, as is assuming that no frames are dropped and 
		   counting backward from the endtime given by stat (which 
		   isn't even possible with end-to-end files) */

	    p = subdirEntry->d_name + strlen(subdirEntry->d_name) - 1;
	    while (*p != '_') p--;
	    p -= strlen("yyyymmddthhmmss");
	    if (p < subdirEntry->d_name) {
		sprintf(errorMsg,
		    "Can't decode start time for recorded file \"%s\".", path);
		m_displayTab->warn(errorMsg,
		    "Giving up directory scan.  "
		    "Recordings list is incomplete.");
		break;
	    }
	    (void)strptime(p, "%Y%m%dt%H%M%S", &tm_struct);
	    recordings[n].startTime = timegm(&tm_struct);

	    if (stat(path, &statbuf) == -1) {
		sprintf(errorMsg, "Can't access recorded file \"%s\".", path);
		m_displayTab->warn(errorMsg,
		    "Giving up directory scan.  "
		    "Recordings list is incomplete.");
		break;
	    }
	    recordings[n].endTime = statbuf.st_mtime;

	    recordings[n].duration = recordings[n].endTime -
		recordings[n].startTime;

		/* if image file, make sure corresponding GPS and PPS data
 		   exists, and if not, put the image on the orphans list */

	    if (recordingType == RECORDING_FLIGHTLINE) {
		if (stat(recordings[n].gpspath, &statbuf) == -1 ||
			stat(recordings[n].ppspath, &statbuf) == -1) {
		    ct = new char[strlen(recordings[n].imagepath)+1];
		    strcpy(ct, recordings[n].imagepath);
		    imageOrphans.add(ct);
		}
	    }

		/* note that we've found another recording */

	    n++;
	}

	    /* restart the search */

	rewinddir(subDir);

	    /* see if we have any GPS or PPS files without imagery */

	while ((subdirEntry=readdir(subDir))) { /*lint --e{661,662} */
	    len = strlen(subdirEntry->d_name);
	    if (len >= (int)strlen("_gps") &&
		    strcmp(subdirEntry->d_name+len-strlen("_gps"),
			"_gps") == 0) {

		    /* determine whether we've seen it already */

		for (i=0;i < n;i++)
		    if (recordings[i].recordingType == RECORDING_FLIGHTLINE &&
			    strcmp(subdirEntry->d_name,
				recordings[i].gpsfile) == 0)
			break;

		    /* if we haven't, add it to the orphans list */

		if (i == n) {
		    ct = new char[MAXPATHLEN+10];
		    (void)sprintf(ct, "%s/%s/%s", productRootDir,
			rootEntry->d_name, subdirEntry->d_name);
		    otherOrphans.add(ct);
		}
	    }
	    else if (len >= (int)strlen("_pps") &&
		    strcmp(subdirEntry->d_name+len-strlen("_pps"),
			"_pps") == 0) {

		    /* determine whether we've seen it already */

		for (i=0;i < n;i++)
		    if (recordings[i].recordingType == RECORDING_FLIGHTLINE &&
			    strcmp(subdirEntry->d_name,
				recordings[i].ppsfile) == 0)
			break;

		    /* if we haven't, add it to the orphans list */

		if (i == n) {
		    ct = new char[MAXPATHLEN+10];
		    (void)sprintf(ct, "%s/%s/%s", productRootDir,
			rootEntry->d_name, subdirEntry->d_name);
		    otherOrphans.add(ct);
		}
	    }
	}

	    /* close the subdirectory */

	(void)closedir(subDir);
    }

	/* close the root directory */

    (void)closedir(rootDir);

	/* make sure entries are sorted, based on start time */

    for (i=0;i < n-1;i++)
	for (j=i+1;j < n;j++)
	    if (recordings[i].startTime > recordings[j].startTime ||
		    (recordings[i].startTime == recordings[j].startTime &&
			recordings[i].recordingType >
			    recordings[j].recordingType)) {
		temp = recordings[i];
		recordings[i] = recordings[j];
		recordings[j] = temp;
	    }

	/* show orphans if we have any */

    if (imageOrphans.get_length() != 0) {
	msg = new char[imageOrphans.get_length() * MAXPATHLEN + 100];
	strcpy(msg, "\n");

	SL_ListWatch<char *> w1;
	imageOrphans.init_scan(&w1);
	while ((ct=imageOrphans.next()) != NULL) {
	    sprintf(msg+strlen(msg), "    %s\n", ct);
	    delete[] ct;
	}
	imageOrphans.clear();

	strcat(msg, "\nAre data directories correct on Settings tab?");
	m_displayTab->warn(
	    "The following image files are missing corresponding GPS or PPS "
		"data.",
	    msg);
    }
    if (otherOrphans.get_length() != 0) {
	msg = new char[otherOrphans.get_length() * MAXPATHLEN + 100];
	strcpy(msg, "\n");

	SL_ListWatch<char *> w1;
	otherOrphans.init_scan(&w1);
	while ((ct=otherOrphans.next()) != NULL) {
	    sprintf(msg+strlen(msg), "    %s\n", ct);
	    delete[] ct;
	}
	otherOrphans.clear();

	strcat(msg, "\nAre data directories correct on Settings tab?");
	m_displayTab->warn(
	    "The following GPS or PPS files are missing corresponding imagery.",
	    msg);
    }

	/* return our list of recordings to caller */

    *recordings_p = recordings;
    *nRecordings_p = n;
}


void RecordingsTab::addRecording(const recording_t *r)
{
    char *buf, *temp;
    struct tm tm_struct;

    Gtk::TreeModel::Row row = *(m_model->append());
    
    buf = new char[20];
    if (r->recordingType == RECORDING_FLIGHTLINE)
	sprintf(buf, "Line %d", ++m_numFlightLines);
    else if (r->recordingType == RECORDING_ALLGPS)
	sprintf(buf, "All-GPS");
    else if (r->recordingType == RECORDING_ALLPPS)
	sprintf(buf, "All-PPS");
    else if (r->recordingType == RECORDING_DIAGS)
	sprintf(buf, "Diags");
    else /* r->recordingType == RECORDING_LOG */
	sprintf(buf, "Log");
    row[m_typeColumn] = buf;

    temp = new char[strlen(r->imagepath)+1];
    strcpy(temp, r->imagepath);
    row[m_imagepathColumn] = temp;

    temp = new char[strlen(r->imagefile)+1]; /*lint !e423 memory leak */
    strcpy(temp, r->imagefile);
    row[m_imagefileColumn] = temp;

    temp = new char[strlen(r->imagehdrpath)+1]; /*lint !e423 memory leak */
    strcpy(temp, r->imagehdrpath);
    row[m_imagehdrpathColumn] = temp;

    temp = new char[strlen(r->imagehdrfile)+1]; /*lint !e423 memory leak */
    strcpy(temp, r->imagehdrfile);
    row[m_imagehdrfileColumn] = temp;

    temp = new char[strlen(r->gpspath)+1]; /*lint !e423 memory leak */
    strcpy(temp, r->gpspath);
    row[m_gpspathColumn] = temp;

    temp = new char[strlen(r->gpsfile)+1]; /*lint !e423 memory leak */
    strcpy(temp, r->gpsfile);
    row[m_gpsfileColumn] = temp;

    temp = new char[strlen(r->ppspath)+1]; /*lint !e423 memory leak */
    strcpy(temp, r->ppspath);
    row[m_ppspathColumn] = temp;

    temp = new char[strlen(r->ppsfile)+1]; /*lint !e423 memory leak */
    strcpy(temp, r->ppsfile);
    row[m_ppsfileColumn] = temp;

    temp = new char[strlen(r->diagspath)+1]; /*lint !e423 memory leak */
    strcpy(temp, r->diagspath);
    row[m_diagspathColumn] = temp;

    temp = new char[strlen(r->diagsfile)+1]; /*lint !e423 memory leak */
    strcpy(temp, r->diagsfile);
    row[m_diagsfileColumn] = temp;

    temp = new char[strlen(r->logpath)+1]; /*lint !e423 memory leak */
    strcpy(temp, r->logpath);
    row[m_logpathColumn] = temp;

    temp = new char[strlen(r->logfile)+1]; /*lint !e423 memory leak */
    strcpy(temp, r->logfile);
    row[m_logfileColumn] = temp;
    
    buf = new char[MAX_TIME_LEN]; /*lint !e423 memory leak */
    (void)gmtime_r(&r->startTime, &tm_struct);
    (void)strftime(buf, MAX_TIME_LEN, "%Y/%m/%d %H:%M:%S", &tm_struct);
    row[m_startTimeColumn] = buf;

    buf = new char[MAX_TIME_LEN]; /*lint !e423 memory leak */
    (void)gmtime_r(&r->endTime, &tm_struct);
    (void)strftime(buf, MAX_TIME_LEN, "%Y/%m/%d %H:%M:%S", &tm_struct);
    row[m_endTimeColumn] = buf;

    buf = new char[MAX_TIME_LEN]; /*lint !e423 memory leak */
    (void)gmtime_r(&r->duration, &tm_struct);
    (void)strftime(buf, MAX_TIME_LEN, "%H:%M:%S", &tm_struct);
    row[m_durationColumn] = buf;
    row[m_durationsecsColumn] = r->duration;

    row[m_recordingTypeColumn] = r->recordingType;
}


RecordingsTab::~RecordingsTab()
{
    m_block = NULL;
    m_displayTab = NULL;
}


void RecordingsTab::disableRecordingsChanges(void)
{
    m_list.set_sensitive(false);
    m_selection->unselect_all(); /* otherwise selected lines are invisible */
    m_deleteButton.set_sensitive(false);
}


void RecordingsTab::enableRecordingsChanges(void)
{
    m_list.set_sensitive(true);
    m_deleteButton.set_sensitive(false);
}


void RecordingsTab::onSelection(void)
{
    m_deleteButton.set_sensitive(true);
}


void RecordingsTab::onDeleteButton(void)
{
    int numFlightLines, numEndtoendFiles;
    time_t duration, totalFlightLineDuration, totalEndtoendDuration;
    u_char flightLineHrs, flightLineMins, flightLineSecs;
    u_char endtoendHrs, endtoendMins, endtoendSecs;
    char warningMsg[200];
    const char *imagepath, *imagehdrpath, *gpspath, *ppspath, *diagspath,
	*logpath;
    Glib::ustring us;
    char logmsg[MAXPATHLEN+100];

	/* log */

    sprintf(logmsg, "User pressed recording Delete button.");
    m_displayTab->log(logmsg);

	/* init */

    typedef Gtk::TreeModel::Children children_t;
    children_t c = m_model->children();

    numFlightLines = numEndtoendFiles = 0;
    totalFlightLineDuration = totalEndtoendDuration = 0;

	/* for each row in our list... */

    for (children_t::iterator iter = c.begin(); iter != c.end(); ++iter) {

	    /* if the row is selected... */

	if (m_selection->is_selected(iter)) {

		/* figure out how much time is represented */

	    Gtk::TreeModel::Row row = *iter;

	    duration = row[m_durationsecsColumn];
	    if (row[m_recordingTypeColumn] == RECORDING_FLIGHTLINE) {
		numFlightLines++;
		totalFlightLineDuration += duration;
	    }
	    else {
		numEndtoendFiles++;
		totalEndtoendDuration += duration;
	    }
        }
    }
    if (numFlightLines == 0 && numEndtoendFiles == 0) {
	m_displayTab->error(
	    "Internal error: Can't determine selected recordings!");
        return;
    }
    flightLineHrs = (u_char)(totalFlightLineDuration / SECONDS_PER_HOUR);
    totalFlightLineDuration -= flightLineHrs * SECONDS_PER_HOUR;
    flightLineMins = (u_char)(totalFlightLineDuration / SECONDS_PER_MINUTE);
    totalFlightLineDuration -= flightLineMins * SECONDS_PER_MINUTE;
    flightLineSecs = (u_char)totalFlightLineDuration;

    endtoendHrs = (u_char)(totalEndtoendDuration / SECONDS_PER_HOUR);
    totalEndtoendDuration -= endtoendHrs * SECONDS_PER_HOUR;
    endtoendMins = (u_char)(totalEndtoendDuration / SECONDS_PER_MINUTE);
    totalEndtoendDuration -= endtoendMins * SECONDS_PER_MINUTE;
    endtoendSecs = (u_char)totalEndtoendDuration;

        /* give user chance to bail -- if user does decide to cancel, force
	   focus away from the delete button back to the list.  otherwise the
	   list looks insensitive, as if it's being acted on */

    if (numFlightLines != 0) {
	if (numEndtoendFiles == 0)
	    sprintf(warningMsg,
		"You're about to delete %d flight line(s), with a "
		    "duration of %u:%02u:%02u.",
		numFlightLines, flightLineHrs, flightLineMins, flightLineSecs);
	else sprintf(warningMsg,
	    "You're about to delete %d flight line(s), with a "
		"duration of %u:%02u:%02u, "
	    "and %d file(s) of end-to-end data, with a "
		"duration of %u:%02u:%02u.",
	    numFlightLines, flightLineHrs, flightLineMins, flightLineSecs,
	    numEndtoendFiles, endtoendHrs, endtoendMins, endtoendSecs);
    }
    else sprintf(warningMsg,
	"You're about to delete %d file(s) of end-to-end data, with a "
	    "duration of %u:%02u:%02u.",
	numEndtoendFiles, endtoendHrs, endtoendMins, endtoendSecs);
    Gtk::MessageDialog d(warningMsg, false /* no markup */,
	Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK_CANCEL);
    if ((Gtk::ResponseType)d.run() != Gtk::RESPONSE_OK) {
	m_list.grab_focus();
	m_displayTab->log("User cancelled delete.");
	return;
    }
    else m_displayTab->log("User confirmed delete.");

        /* go ahead and delete them */

    for (children_t::iterator iter = c.begin();iter != c.end();) {

	    /* if the row is selected... */

	if (m_selection->is_selected(iter)) {

	    Gtk::TreeModel::Row row = *iter;

	    if (row[m_recordingTypeColumn] == RECORDING_FLIGHTLINE) {
		us = row[m_imagepathColumn];
		imagepath = us.c_str();
		unlink(imagepath);
		sprintf(logmsg, "Deleted %s.", imagepath);
		m_displayTab->log(logmsg);

		us = row[m_imagehdrpathColumn];
		imagehdrpath = us.c_str();
		unlink(imagehdrpath);
		sprintf(logmsg, "Deleted %s.", imagehdrpath);
		m_displayTab->log(logmsg);

		us = row[m_gpspathColumn];
		gpspath = us.c_str();
		unlink(gpspath);
		sprintf(logmsg, "Deleted %s.", gpspath);
		m_displayTab->log(logmsg);

		us = row[m_ppspathColumn];
		ppspath = us.c_str();
		unlink(ppspath);
		sprintf(logmsg, "Deleted %s.", ppspath);
		m_displayTab->log(logmsg);

	    }
	    else if (row[m_recordingTypeColumn] == RECORDING_ALLGPS) {
		us = row[m_gpspathColumn];
		gpspath = us.c_str();
		unlink(gpspath);
		sprintf(logmsg, "Deleted %s.", gpspath);
		m_displayTab->log(logmsg);
	    }
	    else if (row[m_recordingTypeColumn] == RECORDING_ALLPPS) {
		us = row[m_ppspathColumn];
		ppspath = us.c_str();
		unlink(ppspath);
		sprintf(logmsg, "Deleted %s.", ppspath);
		m_displayTab->log(logmsg);
	    }
	    else if (row[m_recordingTypeColumn] == RECORDING_DIAGS) {
		us = row[m_diagspathColumn];
		diagspath = us.c_str();
		unlink(diagspath);
		sprintf(logmsg, "Deleted %s.", diagspath);
		m_displayTab->log(logmsg);
	    }
	    else /* row[m_recordingTypeColumn] == RECORDING_LOG */ {
		us = row[m_logpathColumn];
		logpath = us.c_str();
		unlink(logpath);
		sprintf(logmsg, "Deleted %s.", logpath);
		m_displayTab->log(logmsg);
	    }

	    iter = m_model->erase(iter);
        }

	    /* otherwise move on to the next */

	else ++iter;
    }

        /* disable the buttons since nothing is now selected */

    m_deleteButton.set_sensitive(false);

        /* update time left on display dialog */

    m_displayTab->updateResourcesDisplay();

	/* if we've got empty dirs, see if the user wants them pruned */

    if (checkForEmptyDirs(m_block->currentProductRootDir(), 0)) {
	Gtk::MessageDialog d(
	    "Removed specified files.\nPrune empty directories?",
	    false /* no markup */, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO);
	if ((Gtk::ResponseType)d.run() == Gtk::RESPONSE_YES) {
	    m_displayTab->log("User okayed directory pruning.");
	    (void)checkForEmptyDirs(m_block->currentProductRootDir(), 1);
	}
	else m_displayTab->log("User cancelled directory pruning.");
    }
}


int RecordingsTab::checkForEmptyDirs(char *productRootDir, int removeThem) const
{
    DIR *rootDir, *subDir;
    struct dirent *rootEntry, *subdirEntry;
    char subdirPath[MAXPATHLEN];
    int empty;

	/* open the root directory */

    if ((rootDir=opendir(productRootDir)) == NULL) 
	return 0; /* shouldn't happen and doesn't matter much if it does */

	/* for each subdir... */

    while ((rootEntry=readdir(rootDir))) { /*lint --e{661,662} */

	    /* if this isn't a subdirectory, move on to the next entry */

	if (rootEntry->d_type != DT_DIR)
	    continue;

	    /* if it's a lost+found directory, ignore it */

	if (!strcmp(rootEntry->d_name, "lost+found"))
	    continue;

	    /* determine full path of current directory, and skip it if it's
               the daily dir, since we need that even if empty */

	(void)sprintf(subdirPath, "%s/%s", productRootDir, rootEntry->d_name);
	if (!strcmp(subdirPath, appFrame::dailyDir)) continue;

	    /* open the dir so we can see if it's empty.  if we can't open it
 	       for some odd reason, just skip ahead */

	if ((subDir=opendir(subdirPath)) == NULL)
	    continue;

	    /* set flag if directory is empty */

	empty = 1;
	while ((subdirEntry=readdir(subDir))) { /*lint --e{661,662} */
	    if (strcmp(subdirEntry->d_name, ".") &&
		    strcmp(subdirEntry->d_name, "..")) {
		empty = 0;
		break;
	    }
	}

	    /* close dir -- if the directory is empty, and we're pruning, 
 	       remove it and continue.  if the directory is empty and we're
	       just checking, we're done.  if the directory is not empty,
	       we continue */

	(void)closedir(subDir);
	if (empty) {
	    if (removeThem) rmdir(subdirPath);
	    else {
		(void)closedir(rootDir);
		return 1;
	    }
	}
    }

	/* close the root dir */

    (void)closedir(rootDir);

	/* we're done */

    return 0;
}
