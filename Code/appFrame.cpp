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


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <assert.h>
#include <math.h>
#include <pwd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <gtkmm.h>
#include "main.h"
#include "framebuf.h"
#include "plotting.h"
#include "appFrame.h"
#include "displayTab.h"
#include "diagTab.h"
#include "plotsTab.h"
#include "ecsDataTab.h"
#include "recordingsTab.h"
#include "settingsBlock.h"
#include "settingsTab.h"

static const char *const appFrameDate = "$Date: 2015/12/03 21:39:53 $";

FrameBuffer *appFrame::fb = NULL;
int appFrame::headless = 0;
char appFrame::dailyDir[MAXPATHLEN];


    /* appFrame construction and destruction */

appFrame::appFrame(int h)
{
    char errorMsg[MAX_ERROR_LEN + MAXPATHLEN];
    struct tm *tm_struct;
    char filename[MAXPATHLEN];
    struct passwd *pw;

        /* note that we're successful so far */

    failed = 0;

	/* note whether or not we're headless */

    headless = h;

	/* init everything first, in case we leave this early */

    m_settingsBlock = NULL;
    m_displayTab = NULL;
    m_diagTab = NULL;
    m_plotsTab = NULL;
    m_recordingsTab = NULL;
    m_ecsDataTab = NULL;
    m_settingsTab = NULL;

    m_version = 0;
    m_pendingFrameHeight = 0;
    m_pendingFrameWidth = 0;
    m_pendingCompressionOption = 0;
    m_mountDevFd = -1;

	/* determine software version as date */

    registerSourceFile(appFrameDate);
    registerSourceFile(diagTabDate);
    registerSourceFile(plotsTabDate);
    registerSourceFile(displayTabDate);
    registerSourceFile(ecsDataTabDate);
    registerSourceFile(framebufDate);
    registerSourceFile(libftpDate);
    registerSourceFile(mainDate);
    registerSourceFile(maxonDate);
    registerSourceFile(recordingsTabDate);
    registerSourceFile(settingsBlockDate);
    registerSourceFile(settingsTabDate);
    registerSourceFile(plottingDate);

        /* load in settings from .ngdcs */

    m_settingsBlock = new SettingsBlock(headless);

    if (getenv("HOME") != NULL)
        sprintf(filename, "%s/.ngdcs", getenv("HOME"));
    else {
        pw = getpwuid(getuid());
        sprintf(filename, "%s/.ngdcs", pw->pw_dir);
    }

    m_settingsBlock->loadSettings(filename);

	/* init pending settings */

    m_pendingFrameHeight = m_settingsBlock->currentFrameHeight();
    m_pendingFrameWidth = m_settingsBlock->currentFrameWidth();
    m_pendingCompressionOption = m_settingsBlock->currentCompressionOption();

	/* make sure root directory exists */

    if (ensureDirectoryExists(m_settingsBlock->currentProductRootDir()) == -1) {
	sprintf(errorMsg, "Can't access top-level product directory \"%s\".\n"
	    "Fix permissions or change directory name in ~/.ngdcs.",
	    m_settingsBlock->currentProductRootDir());
	if (!headless) {
	    Gtk::MessageDialog d(errorMsg, false /* no markup */,
		Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
	    d.set_secondary_text("This message is unloggable.");
	    (void)d.run();
	}
	else {
	    (void)fputs(errorMsg, stderr);
	    (void)fputc('\n', stderr);
	}
	failed = 1;
	return;
    }

	/* make sure we have subdirectory representing today's date */

    if (makeDailyDir(m_settingsBlock->currentProductRootDir(), 
	    dailyDir) == -1) {
	(void)sprintf(errorMsg,
	    "Can't find or create daily-data dir under \"%s\".",
	    m_settingsBlock->currentProductRootDir());
	if (!headless) {
	    Gtk::MessageDialog d(errorMsg,
		false /* no markup */, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
	    d.set_secondary_text("This message is unloggable.");
	    (void)d.run();
	}
	else {
	    (void)fputs(errorMsg, stderr);
	    (void)fputc('\n', stderr);
	}
	failed = 1;
	return;
    }

	/* assume worst-possible OBC state for setOBC below */

    lastOBCState = OBC_BRIGHT;

	/* connect to frame buffer, and reset OBC control lines in case it
 	   was previously left in odd position */

    fb = new AlphaDataFrameBuffer(m_settingsBlock->currentFrameHeightLines(),
	m_settingsBlock->currentFrameWidthSamples(), NULL);
    if (fb->failed) {
	if (!headless) {
	    Gtk::MessageDialog d(fb->last_error, false /* no markup */,
		Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
	    d.set_secondary_text("This message is unloggable.");
	    (void)d.run();
	}
	else {
	    (void)fputs(fb->last_error, stderr);
	    (void)fputc('\n', stderr);
	}
	failed = 1;
	return;
    }
    if (m_settingsBlock->currentOBCInterface() != OBCINTERFACE_NONE)
	setOBC(OBC_SCIENCE);

	/* set up GPS serial connection */

    fb->setExtSerialBaud(m_settingsBlock->currentGPSSerialBaudBPS());
    switch (m_settingsBlock->currentGPSSerialParity()) {
	case GPSSERIALPARITY_NONE:
	    fb->setExtSerialParityNone();
	    break;
	case GPSSERIALPARITY_EVEN:
	    fb->setExtSerialParityEven();
	    break;
	case GPSSERIALPARITY_ODD:
	    fb->setExtSerialParityOdd();
	    break;
	default: /* shouldn't happen */
	    (void)sprintf(errorMsg,
		"Saved GPS serial parity isn't available.  Using odd.");
	    if (!headless) {
		Gtk::MessageDialog d(errorMsg, false /* no markup */,
		    Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK);
		d.set_secondary_text("This message is unloggable.");
		(void)d.run();
	    }
	    else {
		(void)fputs(errorMsg, stderr);
		(void)fputc('\n', stderr);
	    }
	    fb->setExtSerialParityOdd();
	    break;
    }
    switch (m_settingsBlock->currentGPSSerialTXInvert()) {
	case GPSSERIALTXINVERT_OFF:
	    fb->setExtSerialTXInvertOff();
	    break;
	case GPSSERIALTXINVERT_ON:
	    fb->setExtSerialTXInvertOn();
	    break;
	default: /* shouldn't happen */
	    (void)sprintf(errorMsg,
		"Saved GPS serial TX-invert isn't available.  "
		"Using no inversion.");
	    if (!headless) {
		Gtk::MessageDialog d(errorMsg, false /* no markup */,
		    Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK);
		d.set_secondary_text("This message is unloggable.");
		(void)d.run();
	    }
	    else {
		(void)fputs(errorMsg, stderr);
		(void)fputc('\n', stderr);
	    }
	    fb->setExtSerialTXInvertOff();
	    break;
    }

    if (fb->setFrameRate(m_settingsBlock->currentFrameRate()) == -1) {
	(void)sprintf(errorMsg,
	    "Saved frame rate isn't available.  Using default.");
	if (!headless) {
	    Gtk::MessageDialog d(errorMsg, false /* no markup */,
		Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK);
	    d.set_secondary_text("This message is unloggable.");
	    (void)d.run();
	}
	else {
	    (void)fputs(errorMsg, stderr);
	    (void)fputc('\n', stderr);
	}
    }

	/* create version string */

    strcpy(versionString, "NGDCS software was last revised ");
    tm_struct = gmtime(&m_version);
    (void)strftime(versionString+strlen(versionString),
	sizeof(versionString) - strlen(versionString),
	"%m/%d/%Y at %H:%M:%S GMT.\n", tm_struct);
    (void)sprintf(versionString+strlen(versionString),
	"FPGA is version hx%x.\n", fb->fpgaVersion());
}


void appFrame::registerSourceFile(const char *date)
{
    const char *p;
    struct tm tm_struct;
    char *timeparse;
    time_t t;

	/* if file's check-in date is of expected format... */

    if ((p=strchr(date, ' ')) != NULL) {

	    /* convert the string to a number of seconds relative to the epoch,
	       and if this time is later than the check-in time for other files
	       we've seen, update the version's build date */

	(void)memset(&tm_struct, 0, sizeof(tm_struct));
	timeparse = strptime(p+1, "%Y/%m/%d %H:%M:%S", &tm_struct);
	if (timeparse != NULL && strcmp(timeparse, " $") == 0) {
	    t = timegm(&tm_struct);
	    if (t > m_version)
		m_version = t;
	}
    }
}


appFrame::~appFrame()
{
}


int appFrame::handleExitRequest(void)
{
    struct passwd *pw;
    char filename[MAXPATHLEN];
    char msg[MAXPATHLEN+100];

	/* log it */

    m_displayTab->log("User requested exit.");

        /* don't allow the user to exit if we're recording */

    if (m_displayTab->recording()) {
	m_displayTab->error(
            "NGDCS software is recording.  You must stop recording before "
		"exiting the program.");
        return -1;
    }

	/* stop display processing -- this probably isn't necessary but seems
	   potentially cleaner */

    m_displayTab->stopThreadsAndTimers();

        /* leave OBC in science mode */

    if (m_settingsBlock->currentOBCInterface() != OBCINTERFACE_NONE)
	setOBC(OBC_SCIENCE);

        /* save any user-modified settings in the registry */

    if (getenv("HOME") != NULL)
        sprintf(filename, "%s/.ngdcs", getenv("HOME"));
    else {
        pw = getpwuid(getuid());
        sprintf(filename, "%s/.ngdcs", pw->pw_dir);
    }
    m_settingsBlock->setFrameHeight(m_pendingFrameHeight);
    m_settingsBlock->setFrameWidth(m_pendingFrameWidth);
    m_settingsBlock->setCompressionOption(m_pendingCompressionOption);
    if (m_settingsBlock->saveSettings(filename) == -1) {
        sprintf(msg, "Can't open settings file, \"%s\".", filename);
	m_displayTab->warn(msg,
	    "Values on Settings tab can't be saved.");
    }

	/* close end-to-end files */

    m_displayTab->cleanupForExit();
    return 0;
}


void appFrame::setOBC(int newState)
{
    struct timespec ts;
    char buf[2];

	/* if we're transitioning from bright to something other than medium,
 	   do it slowly to avoid damaging the OBC */

    if (lastOBCState == OBC_BRIGHT && newState != OBC_MEDIUM) {
	setOBCWork(OBC_OPER_A_LOW, OBC_OPER_B_LOW, OBC_SHUTTER_HIGH,
	    OBC_TARGET_HIGH, OBC_LASER_HIGH);
	ts.tv_sec = 0;
	ts.tv_nsec = 60 * 1000000;  /* i.e., 60 ms */
	(void)nanosleep(&ts, NULL);
    }

	/* command the new TTL signals */

    switch (newState) {
        case OBC_DARK1:
            setOBCWork(OBC_OPER_A_LOW, OBC_OPER_B_LOW, OBC_SHUTTER_LOW,
		OBC_TARGET_HIGH, OBC_LASER_LOW);
            break;
        case OBC_SCIENCE:
            setOBCWork(OBC_OPER_A_LOW, OBC_OPER_B_LOW, OBC_SHUTTER_LOW,
		OBC_TARGET_LOW, OBC_LASER_LOW);
            break;
        case OBC_DARK2:
            setOBCWork(OBC_OPER_A_LOW, OBC_OPER_B_HIGH, OBC_SHUTTER_LOW,
		OBC_TARGET_HIGH, OBC_LASER_LOW);
            break;
        case OBC_MEDIUM:
            setOBCWork(OBC_OPER_A_LOW, OBC_OPER_B_HIGH, OBC_SHUTTER_HIGH,
		OBC_TARGET_HIGH, OBC_LASER_LOW);
            break;
        case OBC_BRIGHT:
            setOBCWork(OBC_OPER_A_HIGH, OBC_OPER_B_HIGH, OBC_SHUTTER_HIGH,
		OBC_TARGET_HIGH, OBC_LASER_LOW);
            break;
        case OBC_LASER:
            setOBCWork(OBC_OPER_A_LOW, OBC_OPER_B_LOW, OBC_SHUTTER_LOW,
		OBC_TARGET_HIGH, OBC_LASER_HIGH);
            break;
        case OBC_IDLE:
            setOBCWork(OBC_OPER_A_LOW, OBC_OPER_B_LOW, OBC_SHUTTER_HIGH,
		OBC_TARGET_HIGH, OBC_LASER_LOW);
            break;
	default:
	    m_displayTab->error("Internal error in appFrame::setOBC");
    }

	/* note the new state */

    lastOBCState = newState;

	/* tell the FPIE the state so that it can mark images for the PXI */

    buf[0] = 'A' + lastOBCState;
    buf[1] = '\0';
    fb->writeCLSerial(buf);
}


void appFrame::setOBCWork(u_char ttlOut1, u_char ttlOut2, u_char ttlOut3,
    u_char ttlOut4, u_char ttlOut5) const
{
    unsigned char obcControlByte;

        /* this routine returns without doing anything if the OBC interface is
           set to OBCINTERFACE_NONE, although that really shouldn't happen based
           on checks in the recording code */

    if (m_settingsBlock->currentOBCInterface() == OBCINTERFACE_FPGA) {

            /* make sure bits passed in are really just one bit wide */

        ttlOut1 &= 0x1;
        ttlOut2 &= 0x1;
        ttlOut3 &= 0x1;
        ttlOut4 &= 0x1;
        ttlOut5 &= 0x1;

            /* set new control byte */

        obcControlByte = ttlOut1 | (ttlOut2 << 1) | (ttlOut3 << 2) |
	    (ttlOut4 << 3) | (ttlOut5 << 4); /*lint !e734 ignore potential overflow */
	fb->setGPIO(OBC_GPIO_BITMASK, obcControlByte);
    }
}


int appFrame::openMountConnection(int *alreadyOpenFlag_p, int quiet)
{
    	/* connect if necessary */

    if (m_mountDevFd == -1) {
	*alreadyOpenFlag_p = 0;
	m_mountDevFd = openPCSerial(m_settingsBlock->currentMountDev(), 19200);
	if (m_mountDevFd == -1) {
	    if (quiet)
		return -1;
	    else return showMountError(
		"Can't open serial connection to mount.  Is port available?");
	}
    }
    else *alreadyOpenFlag_p = 1;
    return 0;
}


int appFrame::sendMountCmd(const char *cmd, int quiet)
{
	/* send the new state to the mount */

    if (writePCSerial(m_mountDevFd, cmd, strlen(cmd)) != (int)strlen(cmd)) {
	if (quiet)
	    return -1;
	else return showMountError(
	    "Command write to mount failed.  Is cable connected?");
    }
    return 0;
}


int appFrame::getMountResponse(char *response, int maxResponseSize,
    int *actualResponseSize_p, int quiet)
{
    int count;

	/* get the response */

    if ((count=readPCSerial(m_mountDevFd, response, maxResponseSize,
	    10000)) <= 0) {
	if (quiet)
	    return -1;
	else return showMountError("Can't read serial response from mount.");
    }
    if (response != NULL && maxResponseSize > 0 && *response != '+') {
	if (quiet)
	    return -1;
	else return showMountError("Mount command failed.");
    }

	/* pass response size back to the user */

    *actualResponseSize_p = count;
    return 0;
}


int appFrame::showMountError(const char *string)
{
    if (m_displayTab == NULL) {
	if (!headless) {
	    Gtk::MessageDialog d(string, false /* no markup */,
		Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK);
	    d.set_secondary_text("This message is unloggable.");
	    (void)d.run();
	}
	else {
	    (void)fputs(string, stderr);
	    (void)fputc('\n', stderr);
	}
    }
    else m_displayTab->error(string);
    return -1;
}


void appFrame::setPXIRecord(int recording) const
{
    if (recording)
	fb->setGPIO(PXI_GPIO_BITMASK, PXI_START_RECORD);
    else fb->setGPIO(PXI_GPIO_BITMASK, PXI_STOP_RECORD);
}


void appFrame::comboEntryToIndex(const Gtk::ComboBoxText *combo,
    const char *const strings[], int *setting_p, const char *description)
{
    m_settingsBlock->stringToIndex(combo->get_active_text().c_str(),
	strings, setting_p, description);
}


int appFrame::ensureDirectoryExists(const char *dir)
{
    struct stat statbuf;
    char *copy, *slash, *last_slash;
    mode_t desired_mode;

	/* if the directory exists and is writable, we're done */

    if (stat(dir, &statbuf) != -1 && (statbuf.st_mode & 0300) == 0300)
	return 0;

	/* use the permissions of the current directory by default */

    if (stat(".", &statbuf) == -1)
	return -1;
    desired_mode = statbuf.st_mode & 0x1ff;

	/* otherwise, make a copy of the path that we can manipulate, trimming
	   any trailing slashes */

    copy = new char[strlen(dir)+1];
    strcpy(copy, dir);
    while (strlen(copy) > 1 && *(copy+strlen(copy)-1) == '/')
	*(copy+strlen(copy)-1) = '\0';

	/* init */

    last_slash = copy;

	/* indefinitely... */

    for (;;) {

	    /* find the next slash (other than the starting character) and
	       remove it */

	slash = strchr(last_slash+1, '/');
	if (slash != NULL) {
	    *slash = '\0';
	    last_slash = slash;
	}

	    /* if the resulting directory doesn't exist, try to create it using
	       the permissions of the parent */

	if (stat(copy, &statbuf) == -1) {
	    if (mkdir(copy, desired_mode) == -1)
		return -1;
	}
	else desired_mode = statbuf.st_mode & 0x1ff;

	    /* if there was no slash, we're done.  otherwise, replace the slash
	       and continue */

	if (slash == NULL)
	    break;
	else *slash = '/';
    }
    return 0;
}


int appFrame::makeDailyDir(const char *root, char *dailyDirPath)
{
    struct timeval timebuffer;
    struct tm *tm_struct;
    char temp[MAXPATHLEN];

    (void)gettimeofday(&timebuffer, NULL);
    tm_struct = gmtime(&timebuffer.tv_sec);
    (void)sprintf(temp, "%s/", root);
    (void)strftime(temp+strlen(temp), MAX_TIME_LEN, "%Y%m%d", tm_struct);
    if (appFrame::ensureDirectoryExists(temp) == -1)
	return -1;
    else {
	if (dailyDirPath != NULL)
	    (void)strcpy(dailyDirPath, temp);
	return 0;
    }
}


int appFrame::openPCSerial(const char *devName, int baud)
{
    int fd;
    struct termios options;

    fd = open(devName, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd != -1) {
	fcntl(fd, F_SETFL, 0);

	tcgetattr(fd, &options);
	switch (baud) {
	    case 1200: baud = B1200; break;
	    case 2400: baud = B2400; break;
	    case 4800: baud = B4800; break;
	    case 9600: baud = B9600; break;
	    case 19200: baud = B19200; break;
	    case 38400: baud = B38400; break;
	    case 57600: baud = B57600; break;
	    case 115200: baud = B115200; break;
	    default:
		close(fd);
		return -1;
	}
	cfsetispeed(&options, baud);
	cfsetospeed(&options, baud);
	options.c_cflag |= CLOCAL | CREAD;

	options.c_cflag &= ~PARENB;
	options.c_cflag &= ~CSTOPB;
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;

	options.c_cflag &= ~CRTSCTS;
	options.c_iflag &= ~(IXON | IXOFF | IXANY);

	tcsetattr(fd, TCSANOW, &options);
    }
    sleep(1); /* otherwise Keyspan-mount connection sometimes fails */
    return fd;
}


int appFrame::writePCSerial(int fd, const char *buf, int nbytes)
{
    return write(fd, buf, nbytes);
}


int appFrame::readPCSerial(int fd, char *buf, int bufSize, int timeoutUs)
{
    int bytesRead, chunk, readStatus;
    fd_set fds;
    struct timeval timeout;

    bytesRead = 0;
    timeout.tv_sec = 0;
    timeout.tv_usec = timeoutUs;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    while (select(fd+1, &fds, NULL, NULL, &timeout) == 1 &&
	    bytesRead < bufSize) {
	chunk = 256;
	if (bytesRead + chunk > bufSize)
	    chunk = bufSize - bytesRead;
	readStatus = read(fd, buf+bytesRead, chunk);
	bytesRead += readStatus;
	timeout.tv_sec = 0;
	timeout.tv_usec = 10000;
    }
    return bytesRead;
}


void appFrame::closePCSerial(int fd)
{
    (void)close(fd);
}


appFrameHeadless::appFrameHeadless() : appFrame(1)
{
	/* if we've already failed, bail out */

    if (failed)
	return;

        /* create virtual "tab" for data acquisition and control */

    m_displayTab = new DisplayTab(m_settingsBlock, this);

	/* start the data-gathering thread */

    m_displayTab->startDataThreadAndBlock();
}


appFrameHeadless::~appFrameHeadless()
{
    delete fb;
    delete m_displayTab;
    delete m_settingsBlock;
}


appFrameWin::appFrameWin() : appFrame(0)
{
	/* if we've already failed, bail out */

    if (failed)
	return;

	/* make sure user can't CTRL-C out and lose stuff.  signal handlers
	   don't seem to work reliably here: sometimes the app hangs, 
	   perhaps because of some sort of issue with gtkmm */

    (void)signal(SIGINT, SIG_IGN);
    (void)signal(SIGHUP, SIG_IGN);

	/* set window title and border width */

    set_title("NGDCS");
    set_border_width(10);

	/* add box to hold tabs, version label and exit button */

    Gtk::Window::add(m_mainBox);

	/* add the tabs */

    m_notebook.set_border_width(10);
    m_mainBox.pack_start(m_notebook);

        /* create GUI tabs for each component of the user interface */

    m_displayTab = new DisplayTab(m_settingsBlock, this);
    (void)m_notebook.append_page(*m_displayTab, "Display");

    m_diagTab = new DiagTab(m_settingsBlock, this);
    (void)m_notebook.append_page(*m_diagTab, "Diagnostics");

    m_plotsTab = new PlotsTab(m_settingsBlock);
    (void)m_notebook.append_page(*m_plotsTab, "Plots");

    m_recordingsTab = new RecordingsTab(m_settingsBlock, m_displayTab);
    (void)m_notebook.append_page(*m_recordingsTab, "Recordings");

    m_ecsDataTab = new ECSDataTab(m_settingsBlock, m_displayTab);
    (void)m_notebook.append_page(*m_ecsDataTab, "ECS Data");

    m_settingsTab = new SettingsTab(m_settingsBlock, this, m_displayTab,
	m_diagTab, m_plotsTab, m_ecsDataTab, fb);
    m_displayTab->setDiagTab(m_diagTab);
    m_displayTab->setPlotsTab(m_plotsTab);
    m_displayTab->setRecordingsTab(m_recordingsTab);
    m_displayTab->setECSDataTab(m_ecsDataTab);
    m_displayTab->setSettingsTab(m_settingsTab);
    (void)m_notebook.append_page(*m_settingsTab, "Settings");

	/* create version label */

    m_versionLabel.set_text(versionString);
    m_versionAndExit.set_border_width(10);
    m_versionAndExit.pack_start(m_versionLabel, Gtk::PACK_SHRINK);

	/* add the exit button */

    m_buttonBox.set_layout(Gtk::BUTTONBOX_END);
    m_buttonBox.pack_start(m_exitButton);
    m_exitButton.set_label("Exit");
    (void)m_exitButton.signal_clicked().connect(sigc::mem_fun(*this,
	&appFrameWin::on_exit_button));
    m_versionAndExit.pack_start(m_buttonBox);
    m_mainBox.pack_start(m_versionAndExit, Gtk::PACK_SHRINK);

	/* finish up */

    show_all_children();

	/* start the thread that gathers data */

    m_displayTab->startDataThread();

	/* if first time, warn the user to check the settings page */

    if (m_settingsBlock->currentFirstTime()) {
	Gtk::MessageDialog d(
            "Please review the Settings tab, particularly the system\n"
		"configuration settings and data directories, before starting\n"
		"to record data for the first time.",
	    false /* no markup */,
	    Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK);
	(void)d.run();
    }
}


appFrameWin::~appFrameWin()
{
    delete fb;
    delete m_settingsTab;
    delete m_ecsDataTab;
    delete m_recordingsTab;
    delete m_diagTab;
    delete m_plotsTab;
    delete m_displayTab;
    delete m_settingsBlock;
}


bool appFrameWin::on_delete_event(GdkEventAny *)
{
    if (handleExitRequest() == -1)
	return true;
    else {
	hide();
	return false;
    }
}


void appFrameWin::on_exit_button(void)
{
    (void)handleExitRequest();

    hide();
}
