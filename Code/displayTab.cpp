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


#include <unistd.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/time.h>
#if !defined(__APPLE__)
#include <sys/vfs.h>
#endif
#include <math.h>
#include <gtkmm.h>
#include <aiousb.h>
#include <assert.h>
#include <libusb-1.0/libusb.h>
#include "main.h"
#include "framebuf.h"
#include "plotting.h"
#include "appFrame.h"
#include "displayTab.h"
#include "diagTab.h"
#include "plotsTab.h"
#include "recordingsTab.h"
#include "ecsDataTab.h"
#include "settingsBlock.h"
#include "settingsTab.h"
#include "maxon.h"

using namespace AIOUSB;

extern const char *const displayTabDate = "$Date: 2015/12/03 22:43:55 $";


    /* define OBC control options.  not all options make sense and some are
       dangerous (bright light with shutter closed will destroy the bulb), so
       these are enumerated rather than letting user set the OBC lines however
       they'd like.  target closed is high, but shutter closed is low */

const char *const DisplayTab::obcControlOptions[] = {
    "OBC Auto", "OBC Dark", "OBC Idle", "OBC Mid", "OBC High", "OBC Laser",
    NULL };
const u_char DisplayTab::obcControlCodes[
	sizeof(obcControlOptions)/sizeof(char *)-1] = {
    OBC_AUTO, OBC_DARK1, OBC_IDLE, OBC_MEDIUM, OBC_BRIGHT, OBC_LASER };

    /* define shutter options */

const char *const DisplayTab::shutterOptions[] = {
    "Shutter Pos Unknown", "Shutter Closed", "Shutter Open", NULL };

    /* define mount options */

const char *const DisplayTab::mountControlOptions[] = {
    "Mount Auto", "Mount Idle", "Mount Active", NULL };
const u_char DisplayTab::mountControlCodes[
	sizeof(mountControlOptions)/sizeof(char *)-1] = {
    MOUNT_AUTO, MOUNT_IDLE, MOUNT_ACTIVE };


DisplayTab::DisplayTab(SettingsBlock *sb, appFrame *app) :
    m_checksCol1Table(3,2), m_checksCol2Table(3,2), m_checksCol3Table(3,2),
    m_checksCol4Table(3,2),
    m_imageDisplay(sb->currentFrameHeightLines(),
	sb->currentFrameWidthSamples(), this)
{
    int i;
    static const Gdk::Color red("red");
    static const Gdk::Color green("green");
    static const Gdk::Color black("black");
    static const Gdk::Color white("white");
    static const Gdk::Color orange("orange");
    char msg[MAX_ERROR_LEN + 2 * MAXPATHLEN];
    struct timeval timebuffer;
    struct tm *tm_struct;

        /* save pointer to the settings block */

    m_block = sb;

	/* make local copies of frame dimensions.  these aren't allowed to
 	   change during program execution, so this won't cause problems, and
	   makes the code much more readable */

    m_frameHeightLines = sb->currentFrameHeightLines();
    m_frameWidthSamples = sb->currentFrameWidthSamples();

	/* save pointer to the app */

    m_app = app;

	/* open log file -- this must be done as soon as possible so that we
           can capture user events and program messages.  if running headless
	   we can (and must) write the file to its final location, since the
	   location can't change during the running of the program, and there's
	   no formal exit */

    (void)gettimeofday(&timebuffer, NULL);
    tm_struct = gmtime(&timebuffer.tv_sec);

    m_endtoendStartTime = new char[MAX_TIME_LEN];
    (void)strftime(m_endtoendStartTime, MAX_TIME_LEN, "%Y%m%dt%H%M%S",
	tm_struct);

    m_logName = new char[MAXPATHLEN+1];
    if (!appFrame::headless)
	(void)sprintf(m_logName, "%s/logtemp%d", P_tmpdir, getpid());
    else {
        strcpy(m_logName, appFrame::dailyDir);
        strcat(m_logName, "/");
        strcat(m_logName, m_block->currentPrefix());
        strcat(m_logName, m_endtoendStartTime);
        strcat(m_logName, "_log.txt");
    }
    m_logFP = fopen(m_logName, "w");
    if (!m_logFP) {
	(void)sprintf(msg, "Can't open log file \"%s\".", m_logName);
	if (!appFrame::headless) {
	    Gtk::MessageDialog d(msg, false /* no markup */,
		Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK);
	    d.set_secondary_text("This message is unloggable.");
	    (void)d.run();
	}
	else {
	    (void)fputs(msg, stderr);
	    (void)fputc('\n', stderr);
	}
    }

    if (m_logFP) {
	strftime(msg, sizeof(msg), 
	    "Log started %A, %B %d, %Y, at %H:%M:%S UTC.\n\n", tm_struct);
	strcat(msg, m_app->versionString);
	strcat(msg, "\n");
	fputs(msg, m_logFP);
	fprintf(m_logFP, "Settings at startup:\n");
	m_block->logSettings(m_logFP);
	fprintf(m_logFP, "\nEvents:\n");
	fflush(m_logFP);
    }

	/* if headless, open serial control line */

    if (appFrame::headless) {
	m_headlessDevFd = appFrame::openPCSerial(m_block->currentHeadlessDev(), 
	    9600);
	if (m_headlessDevFd == -1) {
	    sprintf(msg, "Can't open serial device, %s.",
		m_block->currentHeadlessDev());
	    warn(msg);
	}
    }
    else m_headlessDevFd = -1;

	/* init tab pointers */

    m_diagTab = NULL;
    m_plotsTab = NULL;
    m_settingsTab = NULL;
    m_recordingsTab = NULL;
    m_ecsDataTab = NULL;

	/* init state variables */

    m_recordState = RECORD_STATE_NOT_RECORDING;
    m_wait = 0;
    m_imageFP = m_imageHdrFP = m_gpsFP = m_ppsFP = NULL;
    m_framesWritten = 0;
    m_stopRequested = 0;
    m_stopIsAbort = 0;
    (void)memset(&m_recordStartTime, 0, sizeof(m_recordStartTime));
    m_outOfSpace = 0;
    m_skipLeft = 0;

	/* init GPS-init data */

    m_frameCountAtGPSInit = ~0;
    (void)memset(m_gpsInitCommand, 0, sizeof(m_gpsInitCommand));
    m_gpsInitBytesSent = ~0;

	/* init diagnostics */

    m_frameCount = 0;
    m_framesDropped = 0;
    m_frameBacklog = 0;
    m_maxFrameBacklog = 0;
    m_msg3Count = 0;
    m_gpsCount = 0;
    m_numSensors = 0;
    m_sensorNames = NULL;
    m_sensorValues = NULL;
    m_sensorRanges = NULL;
    m_fpgaFrameCount = 0;
    m_fpgaPPSCount = 0;
    m_fpga81ffCount = 0;
    m_fpgaFramesDropped = 0;
    m_fpgaSerialErrors = 0;
    m_fpgaSerialPortCtl = 0;
    m_localFrameCount = 0;
    m_fpgaBufferDepth = 0;
    m_fpgaMaxBuffersUsed = 0;

	/* determine max consecutive frames (10 for AVng frames, more for
           smaller frames) */

    m_maxConsecutiveFrames = 10 * NOMINAL_FRAME_HEIGHT / m_frameHeightLines;

	/* init indicator colors */

    m_idleColor = StatusDisplay::COLOR_NONE;
    m_startingCalColor = StatusDisplay::COLOR_NONE;
    m_activeColor = StatusDisplay::COLOR_NONE;
    m_endingCalColor = StatusDisplay::COLOR_NONE;

    m_imagerCheckColor = StatusDisplay::COLOR_YELLOW;
    m_gpsCommCheckColor = StatusDisplay::COLOR_YELLOW;
    m_fpiePPSCheckColor = StatusDisplay::COLOR_YELLOW;
    m_msg3CheckColor = StatusDisplay::COLOR_YELLOW;
    m_cpuCheckColor = StatusDisplay::COLOR_YELLOW;
    m_tempCheckColor = StatusDisplay::COLOR_YELLOW;
    m_airNavCheckColor = StatusDisplay::COLOR_YELLOW;
    m_gpsValidCheckColor = StatusDisplay::COLOR_YELLOW;
    m_fpgaPPSCheckColor = StatusDisplay::COLOR_YELLOW;
    m_mountCheckColor = StatusDisplay::COLOR_YELLOW;
    m_fmsCheckColor = StatusDisplay::COLOR_YELLOW;

	/* init image buffer */

    m_imageBuffer =
	new unsigned short[m_frameWidthSamples * m_frameHeightLines];
    (void)memset(m_imageBuffer, 0,
	m_frameWidthSamples * m_frameHeightLines * sizeof(short));

    m_incr = 1;

    m_framePixels = new unsigned char[m_frameWidthSamples *
	(m_frameHeightLines-1) * 3];
    (void)memset(m_framePixels, 0,
	m_frameWidthSamples * (m_frameHeightLines-1) * 3);

    m_waterfallPixels = new unsigned char[m_frameWidthSamples * 3];
    (void)memset(m_waterfallPixels, 0, m_frameWidthSamples * 3);

	/* init image correction */

    m_framesUntilDark = -1;
    m_darkFramesAccumulated = 0;
    m_darkFrame = new u_short[m_frameHeightLines * m_frameWidthSamples];
    m_darkFrameAccumulation =
	new u_int[m_frameHeightLines * m_frameWidthSamples];
    m_darkFrameBuffered = 0;

    readDarkFrame();

	/* init for stretch -- init all the way to 65536 in case we somehow
   	   get 16-bit data, so that we don't run off the end of the array.
           we have one primary stretch LUT, and then two more that take green
           and blue gain settings into account */

    m_stretchLUT = new unsigned char[65536];
    (void)memset(m_stretchLUT, 255, 65536);
    m_stretchLUTGreen = new unsigned char[65536];
    (void)memset(m_stretchLUTGreen, 255, 65536);
    m_stretchLUTBlue = new unsigned char[65536];
    (void)memset(m_stretchLUTBlue, 255, 65536);
    updateStretch();

	/* init end-to-end files, created in a temp directory so that
           user changes to the data directory on the Settings tab can be
	   accommodated */

    m_allgpsName = new char[MAXPATHLEN+1];
    if (!appFrame::headless)
	(void)sprintf(m_allgpsName, "%s/allgpstemp%d", P_tmpdir, getpid());
    else {
        strcpy(m_allgpsName, appFrame::dailyDir);
        strcat(m_allgpsName, "/");
        strcat(m_allgpsName, m_block->currentPrefix());
        strcat(m_allgpsName, m_endtoendStartTime);
        strcat(m_allgpsName, "_allgps");
    }
    m_allgpsFP = fopen(m_allgpsName, "wb");
    if (!m_allgpsFP) {
	(void)sprintf(msg,
	    "Can't open end-to-end GPS file \"%s\".", m_allgpsName);
	warn(msg);
    }

    m_allppsName = new char[MAXPATHLEN+1];
    if (!appFrame::headless)
	(void)sprintf(m_allppsName, "%s/allppstemp%d", P_tmpdir, getpid());
    else {
        strcpy(m_allppsName, appFrame::dailyDir);
        strcat(m_allppsName, "/");
        strcat(m_allppsName, m_block->currentPrefix());
        strcat(m_allppsName, m_endtoendStartTime);
        strcat(m_allppsName, "_allpps");
    }
    m_allppsFP = fopen(m_allppsName, "wb");
    if (!m_allppsFP) {
	(void)sprintf(msg,
	    "Can't open end-to-end PPS file \"%s\".", m_allppsName);
	warn(msg);
    }

    m_diagsName = new char[MAXPATHLEN+1];
    if (!appFrame::headless)
	(void)sprintf(m_diagsName, "%s/diagstemp%d", P_tmpdir, getpid());
    else {
        strcpy(m_diagsName, appFrame::dailyDir);
        strcat(m_diagsName, "/");
        strcat(m_diagsName, m_block->currentPrefix());
        strcat(m_diagsName, m_endtoendStartTime);
        strcat(m_diagsName, "_diags.csv");
    }
    m_diagsFP = fopen(m_diagsName, "w");
    if (!m_diagsFP) {
	(void)sprintf(msg,
	    "Can't open end-to-end diagnostics file \"%s\".", m_diagsName);
	warn(msg);
    }

	/* if not headless... */

    if (!appFrame::headless) {

	    /* init */

	set_border_width(10);
	set_spacing(15);

	    /* create record and stop buttons in horizontal button box */

	m_buttonHBox.set_spacing(20);

	m_recordButton.set_label("Record");
	m_recordButton.modify_bg(Gtk::STATE_NORMAL, green);
	m_recordButton.modify_bg(Gtk::STATE_PRELIGHT, green);
	m_recordButton.set_size_request(RECORD_BUTTON_WIDTH,
	    RECORD_BUTTON_HEIGHT);
	(void)m_recordButton.signal_clicked().connect(sigc::mem_fun(*this,
	    &DisplayTab::onRecordButton));
	m_buttonHBox.pack_start(m_recordButton, Gtk::PACK_SHRINK);

	m_stopButton.set_label("Stop");
	Gtk::Widget *label = m_stopButton.get_child();
	label->modify_fg(Gtk::STATE_NORMAL, white);
	label->modify_fg(Gtk::STATE_PRELIGHT, white);
	m_stopButton.modify_bg(Gtk::STATE_NORMAL, black);
	m_stopButton.modify_bg(Gtk::STATE_PRELIGHT, black);
	m_stopButton.set_size_request(STOP_BUTTON_WIDTH, STOP_BUTTON_HEIGHT);
	(void)m_stopButton.signal_clicked().connect(sigc::mem_fun(*this,
	    &DisplayTab::onStopButton));
	m_buttonHBox.pack_start(m_stopButton, Gtk::PACK_SHRINK);

	m_fpsLabel.set_label("Frame rate: TBD fps");
	m_buttonHBox.pack_start(m_fpsLabel, Gtk::PACK_SHRINK);

	    /* create OBC-state display in horizontal state box */

	m_obcStateHBox.set_spacing(15);

	m_idleHBox.pack_start(m_idleDisplay, Gtk::PACK_SHRINK);
	m_idleLabel.set_text("Idle");
	m_idleHBox.pack_start(m_idleLabel, Gtk::PACK_SHRINK);
	m_obcStateHBox.pack_start(m_idleHBox, Gtk::PACK_SHRINK);

	m_startingCalHBox.pack_start(m_startingCalDisplay, Gtk::PACK_SHRINK);
	m_startingCalLabel.set_text("Starting Cal");
	m_startingCalHBox.pack_start(m_startingCalLabel, Gtk::PACK_SHRINK);
	m_obcStateHBox.pack_start(m_startingCalHBox, Gtk::PACK_SHRINK);

	m_activeHBox.pack_start(m_activeDisplay, Gtk::PACK_SHRINK);
	m_activeLabel.set_text("Active");
	m_activeHBox.pack_start(m_activeLabel, Gtk::PACK_SHRINK);
	m_obcStateHBox.pack_start(m_activeHBox, Gtk::PACK_SHRINK);

	m_endingCalHBox.pack_start(m_endingCalDisplay, Gtk::PACK_SHRINK);
	m_endingCalLabel.set_text("Ending Cal");
	m_endingCalHBox.pack_start(m_endingCalLabel, Gtk::PACK_SHRINK);
	m_obcStateHBox.pack_start(m_endingCalHBox, Gtk::PACK_SHRINK);

	showIdleMode();

	    /* create misc controls area in horizontal box */

	m_miscControlsHBox.set_spacing(20);

	m_shutterCombo = new Gtk::ComboBoxText();
	for (i=0;shutterOptions[i] != NULL;i++)
	    m_shutterCombo->append_text(shutterOptions[i]);
	m_shutterCombo->set_active_text(shutterOptions[0]);
	(void)m_shutterCombo->signal_changed().connect(sigc::mem_fun(*this,
	    &DisplayTab::onShutterCombo));
	m_miscControlsHBox.pack_start(*m_shutterCombo, Gtk::PACK_SHRINK);

	m_obcControlCombo = new Gtk::ComboBoxText();
	for (i=0;obcControlOptions[i] != NULL;i++)
	    m_obcControlCombo->append_text(obcControlOptions[i]);
	m_obcControlCombo->set_active_text(obcControlOptions[0]);
	(void)m_obcControlCombo->signal_changed().connect(sigc::mem_fun(*this,
	    &DisplayTab::onOBCControlCombo));
	m_miscControlsHBox.pack_start(*m_obcControlCombo, Gtk::PACK_SHRINK);
	m_obcControlComboForced = false;
    }

	/* init OBC mode */

    m_mostRecentOBCMode = 0;

	/* init FMS control */

    fmsControlChanged();

	/* set up mount control */

    if (!appFrame::headless) {
	m_mountControlCombo = new Gtk::ComboBoxText();
	for (i=0;mountControlOptions[i] != NULL;i++)
	    m_mountControlCombo->append_text(mountControlOptions[i]);
	m_mountControlCombo->set_active_text(mountControlOptions[0]);
	(void)m_mountControlCombo->signal_changed().connect(sigc::mem_fun(*this,
	    &DisplayTab::onMountControlCombo));
	m_miscControlsHBox.pack_start(*m_mountControlCombo, Gtk::PACK_SHRINK);
	m_mountControlComboForced = false;
    }
    if (m_block->currentMountUsed() == MOUNTUSED_NO)
	m_mountCheckColor = StatusDisplay::COLOR_UNUSED;
    m_mostRecentMountMode = 0;

	/* if not headless... */

    if (!appFrame::headless) {

	    /* set up INS control */

	m_gpsInitButton.set_label("Init INS");
	(void)m_gpsInitButton.signal_clicked().connect(sigc::mem_fun(*this,
	    &DisplayTab::onGPSInitButton));
	m_miscControlsHBox.pack_start(m_gpsInitButton, Gtk::PACK_SHRINK);

	    /* combine horizontal boxes in controls vertical box */

	m_controlsVBox.set_spacing(15);
	m_controlsVBox.pack_start(m_buttonHBox, Gtk::PACK_SHRINK);
	m_controlsVBox.pack_start(m_obcStateHBox, Gtk::PACK_SHRINK);
	m_controlsVBox.pack_start(m_miscControlsHBox, Gtk::PACK_SHRINK);

	    /* create resources display in vertical health box, and tuck
	       abort button in here too, where it's out of the way */

	m_spaceAvailableLabel.set_label("Recording space");
	m_spaceAvailableHBox.set_spacing(10);
	m_spaceAvailableHBox.pack_start(m_spaceAvailableLabel,
	    Gtk::PACK_SHRINK);
	m_spaceAvailableHBox.pack_start(m_spaceAvailableBar, Gtk::PACK_SHRINK);

	m_abortButton.set_label("Abort");
	m_abortButton.modify_bg(Gtk::STATE_NORMAL, orange);
	m_abortButton.modify_bg(Gtk::STATE_PRELIGHT, orange);
	m_abortButton.set_size_request(ABORT_BUTTON_WIDTH, ABORT_BUTTON_HEIGHT);
	(void)m_abortButton.signal_clicked().connect(sigc::mem_fun(*this,
	    &DisplayTab::onAbortButton));
	static Gtk::Alignment a0(Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, (float)0.0,
	    (float)0.0);
	a0.set_size_request(30, 10);
	m_spaceAvailableHBox.pack_start(a0, Gtk::PACK_SHRINK);
	m_spaceAvailableHBox.pack_start(m_abortButton, Gtk::PACK_SHRINK);

	m_healthVBox.set_spacing(5);
	m_healthVBox.pack_start(m_spaceAvailableHBox, Gtk::PACK_SHRINK);
	m_timeLeftLabel.set_label("Time left: TBD");
	m_healthVBox.pack_start(m_timeLeftLabel, Gtk::PACK_SHRINK);
    }

	/* measure resources */

    updateResourcesDisplay();

 	/* if not headless... */

    if (!appFrame::headless) {

	    /* init spacing */

	Gtk::Label *spacer = new Gtk::Label("");
	m_healthVBox.pack_start(*spacer, Gtk::PACK_SHRINK);

	    /* add indicators box to vertical health box */

	m_checksCol1Table.set_col_spacings(2);
	m_checksCol2Table.set_col_spacings(2);
	m_checksCol3Table.set_col_spacings(2);
	m_checksCol4Table.set_col_spacings(2);
	m_checksBox.set_spacing(30);

	m_imagerCheckLabel.set_label("Imager");
	static Gtk::Alignment a1(Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, (float)0.0,
	    (float)0.0);
	attachIndicator(m_checksCol1Table, 0, a1, m_imagerCheckLabel,
	    m_imagerCheckDisplay);

	m_gpsCommCheckLabel.set_label("GPS Comm");
	static Gtk::Alignment a2(Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, (float)0.0,
	    (float)0.0);
	attachIndicator(m_checksCol1Table, 1, a2, m_gpsCommCheckLabel,
	    m_gpsCommCheckDisplay);

	m_fpiePPSCheckLabel.set_label("FPIE PPS");
	static Gtk::Alignment a3(Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, (float)0.0,
	    (float)0.0);
	attachIndicator(m_checksCol1Table, 2, a3, m_fpiePPSCheckLabel,
	    m_fpiePPSCheckDisplay);

	m_msg3CheckLabel.set_label("Msg 3");
	static Gtk::Alignment a4(Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, (float)0.0,
	    (float)0.0);
	attachIndicator(m_checksCol2Table, 0, a4, m_msg3CheckLabel,
	    m_msg3CheckDisplay);

	m_cpuCheckLabel.set_label("CPU");
	static Gtk::Alignment a5(Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, (float)0.0,
	    (float)0.0);
	attachIndicator(m_checksCol2Table, 1, a5, m_cpuCheckLabel,
	    m_cpuCheckDisplay);

	m_tempCheckLabel.set_label("Temp");
	static Gtk::Alignment a6(Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, (float)0.0,
	    (float)0.0);
	attachIndicator(m_checksCol2Table, 2, a6, m_tempCheckLabel,
	    m_tempCheckDisplay);

	m_airNavCheckLabel.set_label("Air Nav");
	static Gtk::Alignment a7(Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, (float)0.0,
	    (float)0.0);
	attachIndicator(m_checksCol3Table, 0, a7, m_airNavCheckLabel,
	    m_airNavCheckDisplay);

	m_gpsValidCheckLabel.set_label("GPS Valid");
	static Gtk::Alignment a8(Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, (float)0.0,
	    (float)0.0);
	attachIndicator(m_checksCol3Table, 1, a8, m_gpsValidCheckLabel,
	    m_gpsValidCheckDisplay);

	m_fpgaPPSCheckLabel.set_label("FPGA PPS");
	static Gtk::Alignment a9(Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, (float)0.0,
	    (float)0.0);
	attachIndicator(m_checksCol3Table, 2, a9, m_fpgaPPSCheckLabel,
	    m_fpgaPPSCheckDisplay);

	m_mountCheckLabel.set_label("Mount");
	static Gtk::Alignment a10(Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER,
	    (float)0.0, (float)0.0);
	attachIndicator(m_checksCol4Table, 0, a10, m_mountCheckLabel,
	    m_mountCheckDisplay);
	if (m_block->currentMountUsed() != MOUNTUSED_YES)
	    m_mountCheckLabel.set_sensitive(false);

	m_fmsCheckLabel.set_label("FMS Control");
	static Gtk::Alignment a11(Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER,
	    (float)0.0, (float)0.0);
	attachIndicator(m_checksCol4Table, 1, a11, m_fmsCheckLabel,
	    m_fmsCheckDisplay);

	m_checksBox.pack_start(m_checksCol1Table, Gtk::PACK_SHRINK);
	m_checksBox.pack_start(m_checksCol2Table, Gtk::PACK_SHRINK);
	m_checksBox.pack_start(m_checksCol3Table, Gtk::PACK_SHRINK);
	m_checksBox.pack_start(m_checksCol4Table, Gtk::PACK_SHRINK);
	m_healthVBox.pack_start(m_checksBox, Gtk::PACK_SHRINK);

	    /* combine vertical boxes into app-wide box */

	m_aboveImageDisplayHBox.set_spacing(80);
	m_aboveImageDisplayHBox.pack_start(m_controlsVBox, Gtk::PACK_SHRINK);
	m_aboveImageDisplayHBox.pack_start(m_healthVBox, Gtk::PACK_SHRINK);

	    /* add app-wide box to the dialog */
    
	pack_start(m_aboveImageDisplayHBox, Gtk::PACK_SHRINK);

	    /* add display to images-plots horizontal box */

	m_imageDisplayVBox.set_spacing(5);

	m_imageDisplay.set_size_request(
	    m_frameWidthSamples + 2*NON_COMBO_MARGIN, m_frameHeightLines-1);
	m_imageDisplay.add_events(Gdk::BUTTON_PRESS_MASK);
	(void)m_imageDisplay.signal_button_press_event().connect(
	    sigc::mem_fun(*this, &DisplayTab::onRightMouseClick));
	m_imageDisplay.show();
	m_imageDisplayVBox.pack_start(m_imageDisplay, Gtk::PACK_SHRINK);

	m_slidersPromptLabel.set_text(
	    "Right-click over the image area for sliders.");
	static Gtk::Alignment a90(Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER,
	    (float)0.0, (float)0.0);
	a90.add(m_slidersPromptLabel);
	m_imageDisplayVBox.pack_start(a90, Gtk::PACK_SHRINK);

	m_imageAndPlotsHBox.pack_start(m_imageDisplayVBox, Gtk::PACK_SHRINK);

	    /* construct nav-plots area */

	const char *pitchAndRollNames[2] = { "Pitch", "Roll" };
	const char *headingName[1] = { "Heading" };

	m_pitchAndRollPlots = new PlotDisplay("Pitch and Roll", 2,
	    pitchAndRollNames, -10.0, 10.0, m_block->currentNavDurationSecs(),
	    30);
	m_pitchAndRollPlots->setColor(0,PlotDisplay::COLOR_RED);
	m_pitchAndRollPlots->setColor(1,PlotDisplay::COLOR_BLACK);

	m_headingPlot = new PlotDisplay("Heading", 1,
	    headingName, -180.0, 180.0, m_block->currentNavDurationSecs(), 30);
	m_headingPlot->setColor(0, PlotDisplay::COLOR_BLUE);

	m_navVBox.set_border_width(10);
	m_navVBox.set_spacing(15);

	m_navVBox.pack_start(*m_pitchAndRollPlots, Gtk::PACK_SHRINK);
	m_navVBox.pack_start(*m_headingPlot, Gtk::PACK_SHRINK);

	    /* add nav plots to images-plots box and add that to dialog */

	m_imageAndPlotsHBox.pack_start(m_navVBox, Gtk::PACK_SHRINK);
	pack_start(m_imageAndPlotsHBox, Gtk::PACK_SHRINK);

	    /* add log entry to dialog */

	m_logEntryHBox.set_spacing(10);

	m_logEntryLabel.set_text("Log Entry");
	m_logEntryHBox.pack_start(m_logEntryLabel, Gtk::PACK_SHRINK);

	m_logEntry = new Gtk::Entry();
	m_logEntry->set_width_chars(80);
	(void)m_logEntry->signal_activate().connect(
	    sigc::mem_fun(*this, &DisplayTab::onLogEntryChange));
	m_logEntryHBox.pack_start(*m_logEntry, Gtk::PACK_SHRINK);

	pack_start(m_logEntryHBox, Gtk::PACK_SHRINK);
    }

        /* set controls to initial state */

    m_dataThreadEnabled = 0;
    if (!appFrame::headless) {
	enableControlChanges();
	m_stopButton.set_sensitive(false);
	m_abortButton.set_sensitive(false);
    }

    updateResourcesDisplay();

    if (!appFrame::headless) {
	showIdleMode();
	if (m_block->currentShutterInterface() == SHUTTERINTERFACE_NONE)
	    m_shutterCombo->set_sensitive(false);
	if (m_block->currentOBCInterface() == OBCINTERFACE_NONE)
	    m_obcControlCombo->set_sensitive(false);
	if (m_block->currentMountUsed() == MOUNTUSED_NO)
	    m_mountControlCombo->set_sensitive(false);
    }

	/* set up dispatchers */

    if (!appFrame::headless) {
	m_refreshIndicators.connect(sigc::mem_fun(*this,
	    &DisplayTab::indicatorUpdateWork));
	m_refreshFrameDisplay.connect(sigc::mem_fun(*this,
	    &DisplayTab::frameUpdateWork));
	m_refreshWaterfallDisplay.connect(sigc::mem_fun(*this,
	    &DisplayTab::waterfallUpdateWork));
	m_refreshNavDisplay.connect(sigc::mem_fun(*this,
	    &DisplayTab::navDisplayUpdateWork));
	m_refreshRecordControls.connect(sigc::mem_fun(*this,
	    &DisplayTab::recordControlsUpdateWork));
	m_refreshDiags.connect(sigc::mem_fun(*this,
	    &DisplayTab::diagsUpdateWork));
	m_refreshStopButton.connect(sigc::mem_fun(*this,
	    &DisplayTab::stopButtonUpdateWork));
	m_pressRecordButton.connect(sigc::mem_fun(*this,
	    &DisplayTab::handleFMSRecord));
	m_pressStopButton.connect(sigc::mem_fun(*this,
	    &DisplayTab::handleFMSStop));
    }

	/* set up popup menu */

    if (!appFrame::headless) {
	m_actionGroup = Gtk::ActionGroup::create();
	m_actionGroup->add(
	    Gtk::Action::create("ContextMenu", "Context Menu"));
	m_actionGroup->add(
	    Gtk::Action::create("ContextShowSliders", "Show Sliders"),
	    sigc::mem_fun(*this, &DisplayTab::showSliderDialog));

	m_uiManager = Gtk::UIManager::create();
	m_uiManager->insert_action_group(m_actionGroup);

	Glib::ustring ui_info =
	    "<ui>"
	    "    <popup name='PopupMenu'>"
	    "        <menuitem action='ContextShowSliders'/>"
	    "    </popup>"
	    "</ui>";
	try {
	    m_uiManager->add_ui_from_string(ui_info);
	}
	catch(const Glib::Error &ex)
	{
	    fprintf(stderr, "menu build failed\n");
	}

	m_popupMenu = dynamic_cast<Gtk::Menu *>(
	    m_uiManager->get_widget("/PopupMenu"));
	if (!m_popupMenu)
	    fprintf(stderr, "Can't find menu\n");
	show_all_children();
    }

	/* set up thread to grab and save data -- this must be separate from 
	   any display because the windowing event engine hangs temporarily if
	   the user grabs the window, e.g., to move it */

    m_dataThreadEnabled = 1;
    m_dataThreadMutex.lock();
    m_dataThread = Glib::Thread::create(sigc::mem_fun(*this,
	&DisplayTab::dataThread), true);
    if (m_dataThread == NULL) {
	error("Linux error: Can't create data-reading thread.");
	m_dataThreadEnabled = 0;
	if (!appFrame::headless)
	    m_recordButton.set_sensitive(false);
    }

	/* set up resources check every minute */

    m_resourcesCheckTimer = Glib::signal_timeout().connect(sigc::mem_fun(*this,
	&DisplayTab::performResourcesCheck), 60000);

	/* handle the mount as we're able, making sure to start it in idle
           mode */

    if (m_block->currentMountUsed() == MOUNTUSED_YES)
	m_mountHandlerState = MOUNTHANDLER_S0_MAKE_CONNECTION;
    else m_mountHandlerState = MOUNTHANDLER_IDLE;
    m_mountHandler = Glib::signal_idle().connect(sigc::mem_fun(*this,
	&DisplayTab::handleMount));
}


void DisplayTab::attachIndicator(Gtk::Table& table, int row,
    Gtk::Alignment& alignment, Gtk::Label& label, StatusDisplay& indicator)
{
    alignment.add(label);
    table.attach(indicator, 0, 1, row, row+1,
	(Gtk::AttachOptions)Gtk::FILL, (Gtk::AttachOptions)0);
    table.attach(alignment, 1, 2, row, row+1,
	(Gtk::AttachOptions)Gtk::FILL, (Gtk::AttachOptions)0);
}


void DisplayTab::onLogEntryChange(void)
{
    static char *buffer = NULL;
    static int bufferSize = 0;
    int len;

    len = strlen(m_logEntry->get_text().c_str());
    if (buffer == NULL || len+20 > bufferSize) {
	if (buffer != NULL)
	    delete[] buffer;
	bufferSize = len+20;
	buffer = new char[bufferSize];
    }
    (void)sprintf(buffer, "User typed \"%s\"", m_logEntry->get_text().c_str());
    log(buffer);
    m_logEntry->set_text("");
}


DisplayTab::~DisplayTab()
{
	/* files are already known to be closed */

    m_imageFP = NULL;
    m_imageHdrFP = NULL;
    m_gpsFP = NULL;
    m_ppsFP = NULL;

	/* timers are stopped in the exit routine, so we should be able to
	   safely free up storage */

    delete[] m_imageBuffer;
    delete[] m_darkFrame;
    delete[] m_darkFrameAccumulation;
    delete[] m_framePixels;
    delete[] m_waterfallPixels;
    delete[] m_stretchLUT;
    delete[] m_stretchLUTGreen;
    delete[] m_stretchLUTBlue;

	/* end-to-end stuff is cleaned up through the exit routine */

    m_endtoendStartTime = NULL; /*lint !e423 memory leak */

    m_allgpsName = NULL;
    m_allgpsFP = NULL;
    m_allppsName = NULL;
    m_allppsFP = NULL;
    m_diagsName = NULL;
    m_diagsFP = NULL;
    m_logName = NULL;
    m_logFP = NULL;

	/* free up our plot objects */

    if (!appFrame::headless) {
	delete m_pitchAndRollPlots;
	delete m_headingPlot;
    }

	/* misc stuff */

    m_block = NULL;
    m_app = NULL;

	/* tabs are destroyed elsewhere */

    m_diagTab = NULL;
    m_plotsTab = NULL;
    m_settingsTab = NULL;
    m_recordingsTab = NULL;
    m_ecsDataTab = NULL;
}


bool DisplayTab::onRightMouseClick(GdkEventButton *event) const
{
    if (event->button == 3) {
	if (m_popupMenu)
	    m_popupMenu->popup(event->button, event->time);
	return true;
    }
    else return false;
}


void DisplayTab::showSliderDialog(void)
{
    double value;

    SliderDialog d(this);
    (void)d.run();

    if (m_settingsTab) {
	if (d.m_redBandSliderChanged) {
	    value = d.m_redBandSlider.get_value();
	    m_settingsTab->updateRedBandSelection((int)value, true);
	}
	if (d.m_greenBandSliderChanged) {
	    value = d.m_greenBandSlider.get_value();
	    m_settingsTab->updateGreenBandSelection((int)value, true);
	}
	if (d.m_blueBandSliderChanged) {
	    value = d.m_blueBandSlider.get_value();
	    m_settingsTab->updateBlueBandSelection((int)value, true);
	}
	if (d.m_downtrackSkipSliderChanged) {
	    value = d.m_downtrackSkipSlider.get_value();
	    m_settingsTab->updateDowntrackSkipSelection((int)value, true);
	}
	if (d.m_stretchMinSliderChanged) {
	    value = d.m_stretchMinSlider.get_value();
	    m_settingsTab->updateStretchMinSelection((int)value, true);
	}
	if (d.m_stretchMaxSliderChanged) {
	    value = d.m_stretchMaxSlider.get_value();
	    m_settingsTab->updateStretchMaxSelection((int)value, true);
	}
    }
}


SliderDialog::SliderDialog(DisplayTab *parent) :
    m_viewerCtlsTable(4, 4),
    m_redBandAdj(1.0, 1.0, 480.0, 1.0, 100.0, 0.0),
    m_greenBandAdj(1.0, 1.0, 480.0, 1.0, 100.0, 0.0),
    m_blueBandAdj(1.0, 1.0, 480.0, 1.0, 100.0, 0.0),
    m_downtrackSkipAdj(0.0, 0.0, 9.0, 1.0, 1.0, 0.0),
    m_stretchMinAdj(0.0, 0.0, (double)MAX_DN, 1.0, 10000.0, 0.0),
    m_stretchMaxAdj((double)MAX_DN, 0.0, (double)MAX_DN, 1.0, 10000.0, 0.0),
    m_redBandSlider(m_redBandAdj),
    m_greenBandSlider(m_greenBandAdj),
    m_blueBandSlider(m_blueBandAdj),
    m_downtrackSkipSlider(m_downtrackSkipAdj),
    m_stretchMinSlider(m_stretchMinAdj),
    m_stretchMaxSlider(m_stretchMaxAdj)
{
    Gtk::VBox *box = get_vbox();

	/* init variables */

    m_redBandSliderChanged = false;
    m_greenBandSliderChanged = false;
    m_blueBandSliderChanged = false;
    m_downtrackSkipSliderChanged = false;
    m_stretchMinSliderChanged = false;
    m_stretchMaxSliderChanged = false;

    m_parent = parent;

	/* configure viewer controls */

    m_redBandAdj.set_upper(
	(double)m_parent->m_block->currentFrameHeightLines()-1);
    m_redBandAdj.set_value((double)m_parent->m_block->currentRedBand());
    m_redBandSlider.set_digits(0);
    m_redBandSlider.set_size_request(200, 40);
    (void)m_redBandSlider.signal_value_changed().connect(sigc::mem_fun(*this,
	&SliderDialog::onRedBandSliderChange));

    m_greenBandAdj.set_upper(
	(double)m_parent->m_block->currentFrameHeightLines()-1);
    m_greenBandAdj.set_value((double)m_parent->m_block->currentGreenBand());
    m_greenBandSlider.set_digits(0);
    m_greenBandSlider.set_size_request(200, 40);
    (void)m_greenBandSlider.signal_value_changed().connect(sigc::mem_fun(*this,
	&SliderDialog::onGreenBandSliderChange));

    m_blueBandAdj.set_upper(
	(double)m_parent->m_block->currentFrameHeightLines()-1);
    m_blueBandAdj.set_value((double)m_parent->m_block->currentBlueBand());
    m_blueBandSlider.set_digits(0);
    m_blueBandSlider.set_size_request(200, 40);
    (void)m_blueBandSlider.signal_value_changed().connect(sigc::mem_fun(*this,
	&SliderDialog::onBlueBandSliderChange));

    m_downtrackSkipAdj.set_value(
	(double)m_parent->m_block->currentDowntrackSkip());
    m_downtrackSkipSlider.set_digits(0);
    m_downtrackSkipSlider.set_size_request(200, 40);
    (void)m_downtrackSkipSlider.signal_value_changed().connect(
	sigc::mem_fun(*this, &SliderDialog::onDowntrackSkipSliderChange));

    m_stretchMinAdj.set_value((double)m_parent->m_block->currentStretchMin());
    m_stretchMinSlider.set_digits(0);
    m_stretchMinSlider.set_size_request(200, 40);
    (void)m_stretchMinSlider.signal_value_changed().connect(sigc::mem_fun(*this,
	&SliderDialog::onStretchMinSliderChange));

    m_stretchMaxAdj.set_value((double)m_parent->m_block->currentStretchMax());
    m_stretchMaxSlider.set_digits(0);
    m_stretchMaxSlider.set_size_request(200, 40);
    (void)m_stretchMaxSlider.signal_value_changed().connect(sigc::mem_fun(*this,
	&SliderDialog::onStretchMaxSliderChange));

	/* add viewer controls to dialog */

    set_border_width(15);
    box->set_spacing(10);

    m_viewerCtlsTable.set_col_spacings(15);

    m_viewerCtlsTable.attach(*Gtk::manage(new Gtk::Label("Red Band", false)),
	0, 1, 0, 1, (Gtk::AttachOptions)Gtk::FILL, (Gtk::AttachOptions)0);
    m_viewerCtlsTable.attach(m_redBandSlider,
	1, 2, 0, 1, (Gtk::AttachOptions)Gtk::FILL, (Gtk::AttachOptions)0);
    if (m_parent->m_block->currentViewerType() != VIEWERTYPE_WATERFALL &&
	    m_parent->m_block->currentViewerType() != VIEWERTYPE_COMBO)
	m_redBandSlider.set_sensitive(false);

    m_viewerCtlsTable.attach(*Gtk::manage(new Gtk::Label("Green Band", false)),
	0, 1, 1, 2, (Gtk::AttachOptions)Gtk::FILL, (Gtk::AttachOptions)0);
    m_viewerCtlsTable.attach(m_greenBandSlider,
	1, 2, 1, 2, (Gtk::AttachOptions)Gtk::FILL, (Gtk::AttachOptions)0);
    if (m_parent->m_block->currentViewerType() != VIEWERTYPE_WATERFALL &&
	    m_parent->m_block->currentViewerType() != VIEWERTYPE_COMBO)
	m_greenBandSlider.set_sensitive(false);

    m_viewerCtlsTable.attach(*Gtk::manage(new Gtk::Label("Blue Band", false)),
	0, 1, 2, 3, (Gtk::AttachOptions)Gtk::FILL, (Gtk::AttachOptions)0);
    m_viewerCtlsTable.attach(m_blueBandSlider,
	1, 2, 2, 3, (Gtk::AttachOptions)Gtk::FILL, (Gtk::AttachOptions)0);
    if (m_parent->m_block->currentViewerType() != VIEWERTYPE_WATERFALL &&
	    m_parent->m_block->currentViewerType() != VIEWERTYPE_COMBO)
	m_blueBandSlider.set_sensitive(false);

    m_viewerCtlsTable.attach(
	*Gtk::manage(new Gtk::Label("Downtrack Skip", false)),
	0, 1, 3, 4, (Gtk::AttachOptions)Gtk::FILL, (Gtk::AttachOptions)0);
    m_viewerCtlsTable.attach(m_downtrackSkipSlider,
	1, 2, 3, 4, (Gtk::AttachOptions)Gtk::FILL, (Gtk::AttachOptions)0);
    if (m_parent->m_block->currentViewerType() != VIEWERTYPE_WATERFALL &&
	    m_parent->m_block->currentViewerType() != VIEWERTYPE_COMBO)
	m_downtrackSkipSlider.set_sensitive(false);

    m_viewerCtlsTable.attach(*Gtk::manage(new Gtk::Label("Stretch Min", false)),
	2, 3, 0, 1, (Gtk::AttachOptions)Gtk::FILL, (Gtk::AttachOptions)0);
    m_viewerCtlsTable.attach(m_stretchMinSlider,
	3, 4, 0, 1, (Gtk::AttachOptions)Gtk::FILL, (Gtk::AttachOptions)0);

    m_viewerCtlsTable.attach(*Gtk::manage(new Gtk::Label("Stretch Max", false)),
	2, 3, 1, 2, (Gtk::AttachOptions)Gtk::FILL, (Gtk::AttachOptions)0);
    m_viewerCtlsTable.attach(m_stretchMaxSlider,
	3, 4, 1, 2, (Gtk::AttachOptions)Gtk::FILL, (Gtk::AttachOptions)0);

    box->add(m_viewerCtlsTable);

    add_button("Close", 0);

    show_all_children();
}


void DisplayTab::readDarkFrame(void)
{
    char filename[MAXPATHLEN], msg[MAXPATHLEN+100];
    FILE *fp;
    int pixelsRead;

	/* zero out dark frame -- this isn't strictly necessary but might 
           help make bugs clearer */

    (void)memset(m_darkFrame, 0, m_frameHeightLines * m_frameWidthSamples *
	sizeof(short));

	/* construct filename from path and frame size */

    sprintf(filename, "%s/ngdcs-dark.%dhx%dw", m_block->currentDarkSubDir(),
	m_frameHeightLines, m_frameWidthSamples);

	/* open the file, and if it doesn't exist, warn the user */

    fp = fopen(filename, "rb");
    if (fp == NULL) {
	sprintf(msg, "Dark frame \"%s\" doesn't yet exist.", filename);
	info(msg);
    }

	/* otherwise read the file in and we're done */

    else {
	pixelsRead = fread(m_darkFrame, sizeof(short),
	    m_frameHeightLines * m_frameWidthSamples, fp);
	if (pixelsRead != m_frameHeightLines * m_frameWidthSamples) {
	    sprintf(msg, "Dark frame \"%s\" is too short.  Ignoring.",
		filename);
	    warn(msg);
	    (void)memset(m_darkFrame, 0,
		m_frameHeightLines * m_frameWidthSamples * sizeof(short));
	}
	(void)fclose(fp);

	m_darkFrameBuffered = 1;
    }
}


void DisplayTab::writeDarkFrame(void)
{
    char filename[MAXPATHLEN], msg[MAXPATHLEN+100];
    FILE *fp;
    int pixelsWritten;

	/* if we have a dark frame buffered... */

    if (m_darkFrameBuffered) {

	    /* construct filename from path and frame size */

	sprintf(filename, "%s/ngdcs-dark.%dhx%dw",
	    m_block->currentDarkSubDir(), m_frameHeightLines,
	    m_frameWidthSamples);

	    /* open the file, and if we can't, alert the user */

	fp = fopen(filename, "wb");
	if (fp == NULL) {
	    sprintf(msg, "Dark frame can't be saved to \"%s\".",
		m_block->currentDarkSubDir());
	    warn(msg);
	}

	    /* otherwise write the file out and we're done */

	else {
	    pixelsWritten = fwrite(m_darkFrame, sizeof(short),
		m_frameHeightLines * m_frameWidthSamples, fp);
	    if (pixelsWritten != m_frameHeightLines * m_frameWidthSamples) {
		sprintf(msg, "Dark frame can't be written to \"%s\".",
		    filename);
		warn(msg);
		(void)fclose(fp);
		(void)unlink(filename);
	    }
	    else (void)fclose(fp);
	}
    }
}


void DisplayTab::cleanupForExit()
{
    char newname[MAXPATHLEN];

	/* if we've been logging to an all-GPS file... */

    if (m_allgpsFP) {

	    /* close the temp file */

	(void)fclose(m_allgpsFP);
	m_allgpsFP = NULL;

	    /* determine filename for product */

	strcpy(newname, appFrame::dailyDir);
	strcat(newname, "/");
	strcat(newname, m_block->currentPrefix());
	strcat(newname, m_endtoendStartTime);
	strcat(newname, "_allgps");

	    /* copy our temp files to the correct names.  rename(2)
	       doesn't work here if directories are on different
	       filesystems */

	copyFile(newname, m_allgpsName);
    }

	/* if we've been logging to an all-PPS file... */

    if (m_allppsFP) {

	    /* close the temp file */

	(void)fclose(m_allppsFP);
	m_allppsFP = NULL;

	    /* determine filename for product */

	strcpy(newname, appFrame::dailyDir);
	strcat(newname, "/");
	strcat(newname, m_block->currentPrefix());
	strcat(newname, m_endtoendStartTime);
	strcat(newname, "_allpps");

	    /* copy our temp files to the correct names */

	copyFile(newname, m_allppsName);
    }

	/* if we've been logging to a diags file... */

    if (m_diagsFP) {

	    /* close the temp file */

	(void)fclose(m_diagsFP);
	m_diagsFP = NULL;

	    /* determine filename for product */

	strcpy(newname, appFrame::dailyDir);
	strcat(newname, "/");
	strcat(newname, m_block->currentPrefix());
	strcat(newname, m_endtoendStartTime);
	strcat(newname, "_diags.csv");

	    /* copy our temp files to the correct names */

	copyFile(newname, m_diagsName);
    }

	/* if we've been logging to a log file... */

    if (m_logFP) {

	    /* close the temp file */

	(void)fclose(m_logFP);
	m_logFP = NULL;

	    /* determine filename for product */

	strcpy(newname, appFrame::dailyDir);
	strcat(newname, "/");
	strcat(newname, m_block->currentPrefix());
	strcat(newname, m_endtoendStartTime);
	strcat(newname, "_log.txt");

	    /* copy our temp files to the correct names */

	copyFile(newname, m_logName);
    }

	/* we're done with m_endtoendStartTime */

    delete[] m_endtoendStartTime;

	/* save our dark frame, if we have one */

    writeDarkFrame();

	/* close serial lines */

    if (m_headlessDevFd != -1)
	appFrame::closePCSerial(m_headlessDevFd);
}


void DisplayTab::copyFile(const char *to, const char *from)
{
    char msg[MAX_ERROR_LEN];
    FILE *fpIn, *fpOut;
    size_t nbytes;
    char buffer[4096];
    int writeFailed;

    *msg = '\0';
    if ((fpIn=fopen(from, "rb")) == NULL)
	(void)sprintf(msg, "Can't reopen end-to-end file \"%s\".", from);
    else if ((fpOut=fopen(to, "wb")) == NULL) {
	(void)sprintf(msg,
	    "Can't create end-to-end file \"%s\".  Data is in \"%s\".", to,
	        from);
	fclose(fpIn);
    }
    else {
	writeFailed = 0;
	while ((nbytes=fread(buffer, 1, sizeof(buffer), fpIn)) > 0)
	    if (fwrite(buffer, 1, nbytes, fpOut) != nbytes) {
		writeFailed = 1;
		break;
	    }
	(void)fclose(fpOut);
	if (writeFailed) {
	    (void)sprintf(msg,
		"Can't create end-to-end file \"%s\".  Data is in \"%s\".",
		to, from);
	    (void)fclose(fpIn);
	    (void)unlink(to);
	}
	else {
	    if (feof(fpIn)) {
		(void)fclose(fpIn);
		(void)unlink(from);
	    }
	    else /* error */ {
		(void)fclose(fpIn);
		(void)sprintf(msg,
		    "Can't create end-to-end file \"%s\".  Data is in \"%s\".",
		    to, from);
		(void)unlink(to);
	    }
	}
    }

	/* if we had an error, show the message */

    if (*msg != '\0')
	warn(msg);
}


ImageDisplay::ImageDisplay(int fh, int fw, DisplayTab *dt)
{
	/* save arguments passed */

    m_displayTab = dt;
    m_frameHeightLines = fh;
    m_frameWidthSamples = fw;

	/* if not headless, initialize Cairo elements */

    if (!appFrame::headless) {
	m_surface = Cairo::ImageSurface::create(Cairo::FORMAT_RGB24,
	    fw + 2*NON_COMBO_MARGIN, fh-1);
	m_context = Cairo::Context::create(m_surface);
    }

	/* initialize clipping dimensions.  we'll set these to the proper
	   values as we do the different displays (frame or waterfall). */

    m_currentWaterfallHeight = 0;

	/* note that we haven't yet seen any data */

    m_dataSeen = false;

	/* note that we don't yet have RGB bands selected */

    m_redBand = m_greenBand = m_blueBand = -1;
}


ImageDisplay::~ImageDisplay()
{
    m_displayTab = NULL; /* freed elsewhere */
}


void ImageDisplay::reset(void)
{
	/* cut waterfall area down -- code will increment this with each
	   line until it reaches the full window height */

    m_currentWaterfallHeight = 0;
}


void ImageDisplay::displayFrame(const u_char *image, int height, int width,
    int redBand, int greenBand, int blueBand)
{
    Glib::RefPtr<Gdk::Window> win = get_window();
    int xImageOffset, reflect;

	/* note that we've now seen data */

    m_dataSeen = true;

	/* note current RGB markers */

    m_redBand = redBand;
    m_greenBand = greenBand;
    m_blueBand = blueBand;

	/* if we're not fully initialized yet, ignore this.  it's not
	   necessary */

    if (!win)
	return;

	/* create a pixbuf with image contents.  this is a very low-cost
	   operation, and it needs to be done whenever we change image
	   size, so I'm just doing it for every frame */

    m_pixbuf = Gdk::Pixbuf::create_from_data(image,
	Gdk::COLORSPACE_RGB, false, 8, width, height, width*3);

	/* copy the pixbuf onto the surface.  this is the equivalent of
	   writing it to a server-side Pixbuf.  we could write the
	   the image directly in the on-expose handler but this doesn't
	   seem to cost much and might make display smoother or the 
	   images more coherent.  after the copy, force redraw. */

    reflect = (m_displayTab->m_block->currentReflectionOption() ==
	REFLECTIONOPTION_ON? 1:0);
    if (m_displayTab->m_block->currentViewerType() == VIEWERTYPE_FRAME) {
	if (reflect) { /* from cairographics.org/matrix_transform */
	    Cairo::Matrix reflection(-1, 0, 0, 1, 2*NON_COMBO_MARGIN+width, 0);
	    m_context->set_identity_matrix();
	    m_context->transform(reflection);
	}
	else m_context->set_identity_matrix();
	Gdk::Cairo::set_source_pixbuf(m_context, m_pixbuf,
	    (double)NON_COMBO_MARGIN, 0.0);
	m_context->paint();
	Gdk::Rectangle r(NON_COMBO_MARGIN, 0, width, height);
	win->invalidate_rect(r, false);
    }
    else /* VIEWERTYPE_COMBO */ {
	xImageOffset = m_frameWidthSamples/2 + 2 * NON_COMBO_MARGIN;
	if (reflect) { /* from cairographics.org/matrix_transform */
	    Cairo::Matrix reflection(-1, 0, 0, 1, 2*xImageOffset+width, 0);
	    m_context->set_identity_matrix();
	    m_context->transform(reflection);
	}
	else m_context->set_identity_matrix();
	Gdk::Cairo::set_source_pixbuf(m_context, m_pixbuf,
	    (double)xImageOffset, 0.0);
	m_context->rectangle((double)xImageOffset, (double)0, (double)width,
	    (double)height);
	m_context->fill();
	Gdk::Rectangle r(xImageOffset, 0, width, height);
	win->invalidate_rect(r, false);
    }
}


void ImageDisplay::displayLine(const u_char *image, u_short width)
{
    Glib::RefPtr<Gdk::Window> win = get_window();
    const u_char *dataPtr;
    u_char *surfacePtr;
    int i, margin, bytesPerLine, reflect;
    const u_char rgbSize = 4;

	/* note that we've now seen data */

    m_dataSeen = true;

	/* if we're not fully initialized yet, ignore this.  it's not
	   necessary */

    if (!win)
	return;

	/* determine whether or not we have a margin on our surface. */

    if (m_displayTab->m_block->currentViewerType() == VIEWERTYPE_COMBO)
	margin = 0;
    else margin = NON_COMBO_MARGIN;

	/* determine reflection */

    reflect = (m_displayTab->m_block->currentReflectionOption() ==
	REFLECTIONOPTION_ON? 1:0);

	/* shift the image down one line.  I couldn't get this to work with
	   the surface primitives so I'm brute-forcing it using get_data().
	   according to the cairomm doc, this is acceptable. */

    bytesPerLine = (m_frameWidthSamples + 2 * NON_COMBO_MARGIN) * rgbSize;
    for (i=m_frameHeightLines-2;i > 0;i--)
	memmove(m_surface->get_data() + margin * rgbSize + i * bytesPerLine,
	    m_surface->get_data() + margin * rgbSize + (i-1) * bytesPerLine,
	    width * rgbSize);

	/* add a new top line.  this isn't entirely portable because it
	   assumes a particular RGB ordering within the surface pixels, i.e., 
	   a little-endian ordering where the blue value is the first byte
	   in the surface pixel and the last byte is 0.  but seeing as how I
	   couldn't get it working using surface primitives and we're
	   targeting Linux systems, this seems okay.  it might not really
	   be necessary to initialize the 4th padding byte, but since a
	   fourth byte is sometimes used for a transparency value, I'm
	   initializing to ensure the results are predictable. */

    dataPtr = image + (reflect? (width-1)*3:0);
    surfacePtr = m_surface->get_data() + margin * rgbSize;

    for (i=0;i < width;i++) {
	*(surfacePtr+2) = *dataPtr;     /* red   */
	*(surfacePtr+1) = *(dataPtr+1); /* green */
	*surfacePtr = *(dataPtr+2);     /* blue  */
	*(surfacePtr+3) = 0;	        /* pad   */
	surfacePtr += rgbSize;
	if (reflect)
	    dataPtr -= 3;
	else dataPtr += 3;
    }

	/* update waterfall height.  this code helps make sure that
	   only the waterfall plot is showing. */

    if (m_currentWaterfallHeight < m_frameHeightLines-1)
	m_currentWaterfallHeight++;

	/* force redraw */

    Gdk::Rectangle r(0, 0, get_allocation().get_width(),
	get_allocation().get_height());
    win->invalidate_rect(r, false);
}


bool ImageDisplay::on_expose_event(GdkEventExpose *e)
{
    int frameBottom, xImageOffset, y;
    Glib::RefPtr<Gdk::Window> win = get_window();

	/* if we haven't seen any data yet, ignore this and come back later */

    if (!m_dataSeen)
	return true;

	/* otherwise, if we've initialized our window... */

    if (win) {

	    /* associate a Cairo context with our DrawingArea */

	Cairo::RefPtr<Cairo::Context> cr = win->create_cairo_context();

	    /* clip to the exposed/invalidated area.  this isn't necessary
	       technically but is low-cost and seems to make things a little
	       cleaner */

	cr->rectangle((double)e->area.x, (double)e->area.y,
	    (double)e->area.width, (double)e->area.height);
	cr->clip();

	    /* clip to the current area and paint */

	if (m_displayTab->m_block->currentViewerType() == VIEWERTYPE_FRAME) {

		/* draw frame */

	    cr->save();
	    cr->rectangle((double)NON_COMBO_MARGIN, 0.0,
		(double)m_frameWidthSamples, (double)m_frameHeightLines-1);
	    cr->clip();
	    cr->set_source(m_surface, 0.0, 0.0);
	    cr->paint();
	    cr->restore();
	}
	else if (m_displayTab->m_block->currentViewerType() ==
		VIEWERTYPE_WATERFALL) {

		/* draw waterfall line */

	    cr->save();
	    cr->rectangle((double)NON_COMBO_MARGIN, 0.0,
		(double)m_frameWidthSamples, (double)m_currentWaterfallHeight);
	    cr->clip();
	    cr->set_source(m_surface, 0.0, 0.0);
	    cr->paint();
	    cr->restore();
	}
	else /* VIEWERTYPE_COMBO */ {

		/* draw waterfall line */

	    cr->save();
	    cr->rectangle(0.0, 0.0, (double)m_frameWidthSamples/2,
		(double)m_currentWaterfallHeight);
	    cr->clip();
	    cr->set_source(m_surface, 0.0, 0.0);
	    cr->paint();
	    cr->restore();

		/* draw frame */

	    frameBottom = (m_frameHeightLines-1) / 2;
	    xImageOffset = m_frameWidthSamples/2 + 2 * NON_COMBO_MARGIN;

	    cr->save();
	    cr->rectangle((double)xImageOffset, 0.0,
		(double)m_frameWidthSamples/2, (double)frameBottom);
	    cr->clip();
	    cr->set_source(m_surface, 0.0, 0.0);
	    cr->paint();
	    cr->restore();

		/* draw RGB markers */

	    cr->save();
	    cr->set_line_width(1.0);
	    cr->set_line_join(Cairo::LINE_JOIN_ROUND);
	    cr->rectangle((double)e->area.x, (double)e->area.y,
		(double)e->area.width, (double)e->area.height);
	    cr->clip();

	    y = m_redBand;
	    if (y == 0) y = 1;
	    cr->set_source_rgb(1.0, 0.0, 0.0);
	    cr->move_to(xImageOffset-10, y);
	    cr->line_to(xImageOffset, y);
	    cr->stroke();

	    y = m_greenBand;
	    if (y == 0) y = 1;
	    cr->set_source_rgb(0.0, 1.0, 0.0);
	    cr->move_to(xImageOffset-10, y);
	    cr->line_to(xImageOffset, y);
	    cr->stroke();

	    y = m_blueBand;
	    if (y == 0) y = 1;
	    cr->set_source_rgb(0.0, 0.0, 1.0);
	    cr->move_to(xImageOffset-10, y);
	    cr->line_to(xImageOffset, y);
	    cr->stroke();

	    cr->restore();
	}
    }

    return true;
}


StatusDisplay::StatusDisplay()
{
    set_size_request(20, 20);

    m_color = COLOR_NONE;
}


bool StatusDisplay::on_expose_event(GdkEventExpose *e)
{
    double x, y;

    Glib::RefPtr<Gdk::Window> win = get_window();
    if (win) {
	Cairo::RefPtr<Cairo::Context> cr = win->create_cairo_context();

	cr->save();
	cr->set_line_width(1.0);
	cr->set_line_join(Cairo::LINE_JOIN_ROUND);
	cr->rectangle((double)e->area.x, (double)e->area.y,
	    (double)e->area.width, (double)e->area.height);
	cr->clip();

        if (m_color == COLOR_UNUSED)
	    cr->set_source_rgb(0.7, 0.7, 0.7);
	else cr->set_source_rgb(0.0, 0.0, 0.0);

	x = y = 5.0;

	cr->move_to(x,y);
	cr->line_to(x+10,y);
	cr->line_to(x+10,y+10);
	cr->line_to(x,y+10);
	cr->line_to(x,y);
	cr->stroke();

	if (m_color != COLOR_NONE) {
	    if (m_color == COLOR_RED)
		cr->set_source_rgb(1.0, 0.0, 0.0);
	    else if (m_color == COLOR_GREEN)
		cr->set_source_rgb(0.0, 1.0, 0.0);
	    else if (m_color == COLOR_YELLOW)
		cr->set_source_rgb(1.0, 1.0, 0.0);
	    else if (m_color == COLOR_GRAY)
		cr->set_source_rgb(0.4, 0.4, 0.4);
	    else /* m_color == COLOR_UNUSED */
		cr->set_source_rgb(0.9, 0.9, 0.9);
	    cr->move_to(x+1,y+1);
	    cr->line_to(x+9,y+1);
	    cr->line_to(x+9,y+9);
	    cr->line_to(x+1,y+9);
	    cr->line_to(x+1,y+1);
	    cr->fill();
	    cr->stroke();
	}

	cr->restore();
    }
    return true;
}


void StatusDisplay::setColor(color_t color)
{
    if (m_color != color) {
	m_color = color;

	if (!appFrame::headless) {
	    Glib::RefPtr<Gdk::Window> win = get_window();
	    if (win) {
		Gdk::Rectangle r(0, 0, get_allocation().get_width(),
		    get_allocation().get_height());
		win->invalidate_rect(r, false);
	    }
	}
    }
}


char *StatusDisplay::color(void)
{
    switch (m_color) {
	case COLOR_RED:
	    return (char *)"RED";
	case COLOR_GREEN:
	    return (char *)"GREEN";
	case COLOR_GRAY:
	    return (char *)"GRAY";
	case COLOR_YELLOW:
	    return (char *)"YELLOW";
	case COLOR_UNUSED:
	    return (char *)"unused";
	case COLOR_NONE:
	default:
	    return (char *)"UNKNOWN";
    }
}


void DisplayTab::updateStretch(void)
{
    int i, dn, min, max;
    double gain;

    min = m_block->currentStretchMin();
    max = m_block->currentStretchMax();
    for (i=0;i < min && i <= MAX_DN;i++)
	m_stretchLUT[i] = 0;
    if (min != max)
	for (;i <= max && i <= MAX_DN;i++) {
	    dn = 256 * (i-min)/(max-min);
	    if (dn == 256)
		dn = 255;
	    m_stretchLUT[i] = (unsigned char)dn;
	}
    for (;i <= MAX_DN;i++)
	m_stretchLUT[i] = 255;

    gain = m_block->currentGreenGain();
    for (i=0;i <= MAX_DN;i++) {
	dn = (int)(m_stretchLUT[i] * gain + 0.5);
	if (dn > 255)
	    dn = 255;
	m_stretchLUTGreen[i] = dn;
    }

    gain = m_block->currentBlueGain();
    for (i=0;i <= MAX_DN;i++) {
	dn = (int)(m_stretchLUT[i] * gain + 0.5);
	if (dn > 255)
	    dn = 255;
	m_stretchLUTBlue[i] = dn;
    }
}


void DisplayTab::disableControlChanges(int disableRecord,
    int disableShutterControl, int disableOBCControl, int disableMountControl)
{
    if (disableRecord)
        disableRecordButton();
    if (disableShutterControl)
        disableShutterCombo();
    if (disableOBCControl)
        disableOBCControlCombo();
    if (disableMountControl)
        disableMountControlCombo();
}


void DisplayTab::enableControlChanges(void)
{
    if (m_dataThreadEnabled &&
	    m_block->currentFMSControlOption() == FMSCONTROL_NO)
	enableRecordButton();
    if (m_block->currentShutterInterface() != SHUTTERINTERFACE_NONE)
        enableShutterCombo();
    if (m_block->currentOBCInterface() != OBCINTERFACE_NONE)
        enableOBCControlCombo();
    if (m_block->currentMountUsed() != MOUNTUSED_NO)
        enableMountControlCombo();
}


void DisplayTab::enableRecordButton(void)
{
    if (!appFrame::headless)
	m_recordButton.set_sensitive(true);
}


void DisplayTab::disableRecordButton(void)
{
    if (!appFrame::headless)
	m_recordButton.set_sensitive(false);
}


void DisplayTab::enableStopButton(void)
{
    if (!appFrame::headless)
	m_stopButton.set_sensitive(true);
}


void DisplayTab::disableStopButton(void)
{
    if (!appFrame::headless)
	m_stopButton.set_sensitive(false);
}


void DisplayTab::enableGPSInitButton(void)
{
    if (!appFrame::headless)
	m_gpsInitButton.set_sensitive(true);
}


void DisplayTab::disableGPSInitButton(void)
{
    if (!appFrame::headless)
	m_gpsInitButton.set_sensitive(false);
}


void DisplayTab::enableAbortButton(void)
{
    if (!appFrame::headless)
	m_abortButton.set_sensitive(true);
}


void DisplayTab::disableAbortButton(void)
{
    if (!appFrame::headless)
	m_abortButton.set_sensitive(false);
}


void DisplayTab::enableShutterCombo(void)
{
    if (!appFrame::headless &&
	    m_block->currentShutterInterface() != SHUTTERINTERFACE_NONE)
	m_shutterCombo->set_sensitive(true);
}


void DisplayTab::disableShutterCombo(void)
{
    if (!appFrame::headless)
	m_shutterCombo->set_sensitive(false);
}


void DisplayTab::enableOBCControlCombo(void)
{
    if (!appFrame::headless &&
	    m_block->currentOBCInterface() != OBCINTERFACE_NONE)
	m_obcControlCombo->set_sensitive(true);
}


void DisplayTab::disableOBCControlCombo(void)
{
    if (!appFrame::headless)
	m_obcControlCombo->set_sensitive(false);
}


void DisplayTab::enableMountControlCombo(void)
{
    if (!appFrame::headless &&
	    m_block->currentMountUsed() != MOUNTUSED_NO)
	m_mountControlCombo->set_sensitive(true);
}


void DisplayTab::disableMountControlCombo(void)
{
    if (!appFrame::headless)
	m_mountControlCombo->set_sensitive(false);
}


void DisplayTab::makeMountUsed(void)
{
    int alreadyOpen;

	/* make sure indicator is visible with status undefined */

    if (!appFrame::headless)
	m_mountCheckLabel.set_sensitive(true);
    m_mountCheckColor = StatusDisplay::COLOR_YELLOW;

	/* make the connection.  this interrupts display briefly, so it's
	   best to do it when the display isn't visible, i.e., when the
	   settings tab is exposed */

    if (m_app->openMountConnection(&alreadyOpen, 0) != -1)
	m_mountHandlerState = MOUNTHANDLER_S0_MAKE_CONNECTION;
    else m_mountCheckColor = StatusDisplay::COLOR_RED;
}


void DisplayTab::makeMountUnused(void)
{
	/* make indicator "unused" */

    if (!appFrame::headless)
	m_mountCheckLabel.set_sensitive(false);
    m_mountHandlerState = MOUNTHANDLER_IDLE;
    m_mountCheckColor = StatusDisplay::COLOR_UNUSED;
}


void DisplayTab::showIdleMode(void)
{
    m_idleColor = StatusDisplay::COLOR_GRAY;
    m_startingCalColor = StatusDisplay::COLOR_NONE;
    m_activeColor = StatusDisplay::COLOR_NONE;
    m_endingCalColor = StatusDisplay::COLOR_NONE;
}


void DisplayTab::showStartingCalMode(void)
{
    m_idleColor = StatusDisplay::COLOR_NONE;
    m_startingCalColor = StatusDisplay::COLOR_GRAY;
    m_activeColor = StatusDisplay::COLOR_NONE;
    m_endingCalColor = StatusDisplay::COLOR_NONE;
}


void DisplayTab::showActiveMode(void)
{
    m_idleColor = StatusDisplay::COLOR_NONE;
    m_startingCalColor = StatusDisplay::COLOR_NONE;
    m_activeColor = StatusDisplay::COLOR_GRAY;
    m_endingCalColor = StatusDisplay::COLOR_NONE;
}


void DisplayTab::showEndingCalMode(void)
{
    m_idleColor = StatusDisplay::COLOR_NONE;
    m_startingCalColor = StatusDisplay::COLOR_NONE;
    m_activeColor = StatusDisplay::COLOR_NONE;
    m_endingCalColor = StatusDisplay::COLOR_GRAY;
}


#if defined(DEBUG)
#define TRACE_SPEED(x) { struct timeval t; \
    (void)gettimeofday(&t, NULL); printf("%c: %d\n", x, (int)t.tv_usec); }
#else
#define TRACE_SPEED(x)
#endif

void DisplayTab::dataThread(void)
{
	/* wait for parent's enable */

    m_dataThreadMutex.lock();

	/* this thread is calibrated to run every 10 ms with the -O3 compile
           option.  data/discrete reading takes 3 ms in that case.  with -g and
	   without -O3, the data lookup takes 8 ms.  waits aren't critical
	   though: they just improve GUI response time */

    while (m_dataThreadEnabled) {
	TRACE_SPEED('1')
	lookForImageData();
	TRACE_SPEED('2')
	if (!appFrame::headless) {
	    if (m_recordToggleAttached)
		testDigitalLineTransition(); // We used 1 ms to read the USB
	    Glib::usleep(6000);
	}
	else {
	    if (m_headlessDevFd != -1)
		checkHeadlessInterface();
	    Glib::usleep(4000);  //  We allow 3 ms to read the serial line
	}
	TRACE_SPEED('3')
    }

	/* clean up */

    if (m_recordToggleAttached)
        detachRecordToggleSwitch();

    m_dataThreadMutex.unlock();
}


void DisplayTab::startDataThread(void)
{
    m_dataThreadMutex.unlock();
}


void DisplayTab::startDataThreadAndBlock(void)
{
    m_dataThreadMutex.unlock();

    Glib::usleep(1000000); /* i.e., 1 second to allow thread to start */
    m_dataThread->join();
}


void DisplayTab::lookForImageData(void)
{
    int i, j, framesExpected, dataAvailable;
    static u_int diagsUpdateIter = 0;
    unsigned short *usp;
    unsigned int *uip;
    unsigned char *ucp;
    static int imageAcquisitionFailures = 0;
    struct timeval recordEndTime;
    double secs;
    int consecutiveFrames;
    int maxSensorRange;

	/* if simulating, set initial dataAvailable to keep up with expected
	   data rate.  otherwise just ask framebuffer whether or not something
	   is ready */

    if (m_block->currentImageSim() == IMAGESIM_NONE)
	dataAvailable = appFrame::fb->frameIsAvailable();
    else if (m_recordState == RECORD_STATE_NOT_RECORDING || m_outOfSpace)
	dataAvailable = 1;
    else /* simulated recording */ {
	(void)gettimeofday(&recordEndTime, NULL);
	secs = (recordEndTime.tv_sec - m_recordStartTime.tv_sec) +
	    ((int)recordEndTime.tv_usec - (int)m_recordStartTime.tv_usec) /
		1.0e6;
	framesExpected = (int)(secs * appFrame::fb->getFrameRateHz());
	if (m_framesWritten >= framesExpected)
	    dataAvailable = 0;
	else dataAvailable = 1;
    }
    consecutiveFrames = 0;

	/* if no data is available... */

    if (!dataAvailable) {

	    /* check to make sure stop wasn't requested.  stop is otherwise
 	       only checked when displaying data */

	if (m_stopRequested) {
	    m_stopRequested = 0;

	    if (m_mostRecentOBCMode != OBC_AUTO || m_stopIsAbort ||
		    m_block->currentOBCInterface() == OBCINTERFACE_NONE)
		enterIdleState();
	    else if (m_block->currentDark2CalPeriodInSecs() > 0)
		enterDark2CalState();
	    else if (m_block->currentMediumCalPeriodInSecs() > 0)
		enterMediumCalState();
	    else if (m_block->currentBrightCalPeriodInSecs() > 0)
		enterBrightCalState();
	    else if (m_block->currentLaserCalPeriodInSecs() > 0)
		enterLaserCalState();
	    else enterIdleState();
	}

	    /* if it looks like we have a problem, update indicator */

	imageAcquisitionFailures++;
	if (imageAcquisitionFailures > 40) {
	    m_imagerCheckColor = StatusDisplay::COLOR_RED;
	    m_fpiePPSCheckColor = StatusDisplay::COLOR_YELLOW;
	    m_gpsCommCheckColor = StatusDisplay::COLOR_YELLOW;
	    m_msg3CheckColor = StatusDisplay::COLOR_YELLOW;
	    m_airNavCheckColor = StatusDisplay::COLOR_YELLOW;
	    m_gpsValidCheckColor = StatusDisplay::COLOR_YELLOW;
	}
    }

	/* otherwise, while data is available... */

    else while (dataAvailable) {

	    /* make sure indicator reflects availability */

	imageAcquisitionFailures = 0;
	m_imagerCheckColor = StatusDisplay::COLOR_GREEN;
	
	    /* get data from FPGA or simulation */

	m_frameCount++;
	consecutiveFrames++;
	if (m_block->currentImageSim() == IMAGESIM_NONE)
	    getImageFrame();
	else simImageFrame();

	    /* if simulating GPS data, modify image -- this goes a little more
 	       often than the clock rate if simulating and not recording,
	       because of the way the data-available flag is set above */

	if (m_block->currentGPSSim() != GPSSIM_NONE)
	    addGPSSimToFrame();

	    /* update dark frame, if applicable.  accumulate dark frames only
 	       through first second.  any longer and the dark-frame period may
	       be over, depending on user settings */

	if (m_framesUntilDark > 0)
	    m_framesUntilDark--;
	if (m_framesUntilDark == 0) {
	    if (m_darkFramesAccumulated < appFrame::fb->getFrameRateHz() *
		    DARK_ACCUM_TIME_SECS) {
		usp = m_imageBuffer + m_frameWidthSamples;
		uip = m_darkFrameAccumulation + m_frameWidthSamples;
		for (i = m_frameHeightLines - 1;i > 0;i--)
		    for (j = m_frameWidthSamples;j > 0;j--)
			*uip++ += *usp++;
		m_darkFramesAccumulated++;
	    }
	    else {
		usp = m_darkFrame + m_frameWidthSamples;
		uip = m_darkFrameAccumulation + m_frameWidthSamples;
		for (i = m_frameHeightLines - 1;i > 0;i--)
		    for (j = m_frameWidthSamples;j > 0;j--)
			*usp++ = *uip++ /
			    (appFrame::fb->getFrameRateHz() *
				DARK_ACCUM_TIME_SECS);
		m_framesUntilDark = -1;
		m_darkFrameBuffered = 1;
	    }
	}

	    /* update indicators for FPIE PPS and Msg3 from imagery */

	processImageTimingData();

	    /* note local frame count */

	ucp = (u_char *)m_imageBuffer + LOCAL_FRAME_COUNT_IMAGE_OFFSET;
	m_localFrameCount = *(ucp+1) << 24;
	m_localFrameCount += *ucp << 16;
	m_localFrameCount += *(ucp+3) << 8;
	m_localFrameCount += *(ucp+2);

	    /* if we've got a waterfall display we need to display the new
	       data for continuity in the display.  this is an easy enough
	       task that we can afford to do this.  we defer full-frame
	       displays to after we catch up with the data rate.  note that
	       the downtrack-skip option lets up skip some lines to preserve
	       the correct aspect ratio in the display */

	if (!appFrame::headless)
	    if (m_block->currentViewerType() == VIEWERTYPE_WATERFALL ||
		    m_block->currentViewerType() == VIEWERTYPE_COMBO)
		displayWaterfallLine();

	    /* process GPS-data area of image */

	processImageGPSData();

	    /* handle according to current record state */

	if (m_outOfSpace) {
	    if (m_recordState != RECORD_STATE_NOT_RECORDING) {
		log("Out of storage space.  Stopping record.");
		enterIdleState();
	    }
	}
	else if (m_recordState == RECORD_STATE_NOT_RECORDING) {
	    (void)writeAllgpsData();
	    (void)writeAllppsData();
	}
	else {
	    (void)writeAllgpsData();
	    (void)writeAllppsData();

	    if (writeFlightlineGPSData() == -1 || writePPSData() == -1 ||
		    writeImageFrame() == -1) {
		enterIdleState();
		m_outOfSpace = 1;
	    }
	    else if (m_stopRequested && m_stopIsAbort) {
		m_stopRequested = 0;
		enterIdleState();
	    }
	    else switch (m_recordState) {
		case RECORD_STATE_DARK1CAL:
		    if (--m_wait == 0)
			enterScienceState();
		    break;
		case RECORD_STATE_SCIENCE:
		    if (m_stopRequested) {
			m_stopRequested = 0;
			if (m_mostRecentOBCMode != OBC_AUTO ||
				m_block->currentOBCInterface() ==
				    OBCINTERFACE_NONE)
			    enterIdleState();
			else if (m_block->currentDark2CalPeriodInSecs() > 0)
			    enterDark2CalState();
			else if (m_block->currentMediumCalPeriodInSecs() > 0)
			    enterMediumCalState();
			else if (m_block->currentBrightCalPeriodInSecs() > 0)
			    enterBrightCalState();
			else if (m_block->currentLaserCalPeriodInSecs() > 0)
			    enterLaserCalState();
			else enterIdleState();
		    }
		    break;
		case RECORD_STATE_DARK2CAL:
		    if (--m_wait == 0) {
			if (m_block->currentMediumCalPeriodInSecs() > 0)
			    enterMediumCalState();
			else if (m_block->currentBrightCalPeriodInSecs() > 0)
			    enterBrightCalState();
			else if (m_block->currentLaserCalPeriodInSecs() > 0)
			    enterLaserCalState();
			else enterIdleState();
		    }
		    break;
		case RECORD_STATE_MEDIUMCAL:
		    if (--m_wait == 0) {
			if (m_block->currentBrightCalPeriodInSecs() > 0)
			    enterBrightCalState();
			else if (m_block->currentLaserCalPeriodInSecs() > 0)
			    enterLaserCalState();
			else enterIdleState();
		    }
		    break;
		case RECORD_STATE_BRIGHTCAL:
		    if (--m_wait == 0) {
			if (m_block->currentLaserCalPeriodInSecs() > 0)
			    enterLaserCalState();
			else enterIdleState();
		    }
		    break;
		case RECORD_STATE_LASERCAL:
		    if (--m_wait == 0)
			enterIdleState();
		    break;
		default:
		    error("Internal error in DisplayTab::lookForImageData");
		    break;
	    }
	}

	    /* see if we've got more data available, either real or simulated */

	dataAvailable = checkForMoreImageData(consecutiveFrames);

	    /* if we've caught up with the data and we're displaying whole
	       frames, show the most recent one */

	if (!dataAvailable &&
		(m_block->currentViewerType() == VIEWERTYPE_FRAME ||
		    m_block->currentViewerType() == VIEWERTYPE_COMBO))
	    if (!appFrame::headless)
		displayCurrentFrame();
    }
// printf("%d\n", consecutiveFrames);

	/* check temps */

    if (diagsUpdateIter % 100 == 0) {

	    /* read the sensors */

	appFrame::fb->readSensors(&m_numSensors, &m_sensorNames,
	    &m_sensorValues, &m_sensorRanges);

	    /* find most out-of-range sensor */

	maxSensorRange = SENSOR_NOMINAL;
	for (i=0;i < m_numSensors;i++)
	    if (m_sensorRanges[i] > maxSensorRange)
		maxSensorRange = m_sensorRanges[i];

	    /* update indicator to show overall temp status */

	switch (maxSensorRange) {
	    case SENSOR_NOMINAL: 
		m_tempCheckColor = StatusDisplay::COLOR_GREEN;
		break;
	    case SENSOR_WARM: 
		m_tempCheckColor = StatusDisplay::COLOR_YELLOW;
		break;
	    case SENSOR_HOT: 
		m_tempCheckColor = StatusDisplay::COLOR_RED;
		break;
	    default:
		break;
	}

	    /* get FPGA registers */

	appFrame::fb->readFPGARegs(&m_fpgaFrameCount, &m_fpgaPPSCount,
	    &m_fpga81ffCount, &m_fpgaFramesDropped, &m_fpgaSerialErrors,
	    &m_fpgaSerialPortCtl, &m_fpgaBufferDepth, &m_fpgaMaxBuffersUsed);

	    /* update diagnostics */

	if (!appFrame::headless)
	    m_refreshDiags();
    }

	/* update indicator display -- for non-headless mode we want to do
 	   this rapidly to capture transient events */

    if (!appFrame::headless)
	m_refreshIndicators();
    else if (diagsUpdateIter % 100 == 0) {
	indicatorUpdateWork();
	updateHeadlessIndicators();
    }

	/* update CSV */

    if (diagsUpdateIter % 100 == 0) {
	if (diagsUpdateIter == 0)
	    saveCSVHeader();
	saveStateToCSV();
    }

	/* if DIO is supposed to be in use but failed, and if we're not
 	   recording, attempt to reinit.  we don't do this if recording 
	   because it creates a popup which could block data storage if
	   ignored */

    if (diagsUpdateIter % 500 == 0) { /* i.e., every 5 seconds for AVIRISng */
	if (!m_recordToggleAttached &&
		m_block->currentFMSControlOption() == FMSCONTROL_YES &&
		m_recordState == RECORD_STATE_NOT_RECORDING) {
	    if (attachRecordToggleSwitch()) {
		m_recordToggleAttached = true;
		m_fmsCheckColor = StatusDisplay::COLOR_YELLOW;
	    }
	}
    }
    diagsUpdateIter++;

	/* if we're in the middle of sending out a serial command to the
           GPS, write some more now */

    if (m_gpsInitBytesSent < GPS_MSG_3510_WORDS * sizeof(short))
	m_gpsInitBytesSent += appFrame::fb->writeExtSerial(
	    (u_char *)m_gpsInitCommand + m_gpsInitBytesSent,
	    GPS_MSG_3510_WORDS * sizeof(short) - m_gpsInitBytesSent);
}


void DisplayTab::getImageFrame(void)
{
    u_char *ucp;
    u_short *usp;
    int i, j;

	/* get data from FPGA.  data should only be 14-bit but I'm not masking
  	   off because the first line will have larger values and the
	   electronics team doesn't want me to mask incorrect pixel values
	   from appearing in saved data files */

    ucp = (u_char *)appFrame::fb->getFrame();
    usp = m_imageBuffer;
    for (i = m_frameHeightLines;i > 0;i--) {
	for (j = m_frameWidthSamples;j > 0;j--) {
	    *usp = (*(ucp+1) << 8) | *ucp;
	    usp++;
	    ucp += sizeof(short);
	}
    }
}


void DisplayTab::simImageFrame(void)
{
    u_short *usp;
    int i, j, caldn;
    double pct;
    static u_short dn = 0;

    memset(m_imageBuffer, 0, m_frameWidthSamples * sizeof(short));
    usp = m_imageBuffer + m_frameWidthSamples;
    if (m_app->lastOBCState == OBC_DARK1 ||
	    m_app->lastOBCState == OBC_DARK2) {
	for (i = m_frameHeightLines - 1;i > 0;i--)
	    for (j = m_frameWidthSamples;j > 0;j--)
		*usp++ = 3 * (m_frameWidthSamples-j);
    }
    else if (m_app->lastOBCState == OBC_MEDIUM) {
	for (i = m_frameHeightLines - 1;i > 0;i--) {
	    pct = i / (double)m_frameHeightLines;
	    caldn = pow(sin(pct * PI),5.0) * (MAX_DN/2);
	    for (j = m_frameWidthSamples;j > 0;j--)
		*usp++ = caldn + 3 * (m_frameWidthSamples-j);
	}
    }
    else if (m_app->lastOBCState == OBC_BRIGHT) {
	for (i = m_frameHeightLines - 1;i > 0;i--) {
	    pct = i / (double)m_frameHeightLines;
	    caldn = pow(sin(pct * PI),5.0) * (MAX_DN-2000);
	    for (j = m_frameWidthSamples;j > 0;j--)
		*usp++ = caldn + 3 * (m_frameWidthSamples-j);
	}
    }
    else if (m_app->lastOBCState == OBC_LASER) {
	for (i = m_frameHeightLines - 1;i > 0;i--) {
	    if (i == m_frameHeightLines/2)
		caldn = MAX_DN-2000;
	    else caldn = 0;
	    for (j = m_frameWidthSamples;j > 0;j--)
		*usp++ = caldn + 3 * (m_frameWidthSamples-j);
	}
    }
    else /* not a cal frame */ {
	if (m_block->currentImageSim() == IMAGESIM_INCRWITHLINE)
	    dn = 0;
	for (i = m_frameHeightLines - 1;i > 0;i--) {
	    for (j = m_frameWidthSamples;j > 0;j--)
		*usp++ = dn + 3 * (m_frameWidthSamples-j);
	    if (m_block->currentImageSim() == IMAGESIM_INCRWITHLINE) {
		dn += 64;
		if (dn > MAX_DN-2000)
		    dn = 0;
	    }
	}
	if (m_block->currentImageSim() == IMAGESIM_INCRWITHFRAME) {
	    dn += 64;
	    if (dn > MAX_DN-2000)
		dn = 0;
	}
    }
}


void DisplayTab::addGPSSimToFrame(void)
{
    u_short *usp;
    int ppsInterval, msg3Interval, chunk, i;
    static u_short msgSimWordsLeft = 0;
    static int gpsMode = 0;
    static u_short msgSimBuf[GPS_MSG_3500_WORDS + GPS_MSG_3501_WORDS];
    static int baseLat = 0, baseLon = 0;
    int lat, lon, altitude, velNorth, velEast, velUp, pitch, roll, heading;
    static u_int simIteration = 0;

    ppsInterval = (int)(appFrame::fb->getFrameRateHz() + 0.5);
    msg3Interval = ppsInterval / 10;
    if (msg3Interval < 1)
	msg3Interval = 1;

    if (m_block->currentGPSSim() == GPSSIM_ALL) {
	if ((m_frameCount % ppsInterval) == 0) {
	    usp = m_imageBuffer + PPS_IMAGE_OFFSET / sizeof(short);
	    *usp++ = 0xBABE;
	    *usp++ = 2;
	    *usp++ = m_frameCount & 0xffff;
	    *usp++ = m_frameCount >> 16;

	    if (msgSimWordsLeft + GPS_MSG_3500_WORDS > 
		    (int)(sizeof(msgSimBuf) / sizeof(short)))
		printf("GPS msg buffer overflow!\n");
	    else {
		msgSimBuf[msgSimWordsLeft++] = GPS_MAGIC_NUMBER;
		msgSimBuf[msgSimWordsLeft++] = 3500;
		msgSimBuf[msgSimWordsLeft++] = GPS_MSG_3500_WORDS - 6;
		msgSimBuf[msgSimWordsLeft++] = 0x8000;
		msgSimBuf[msgSimWordsLeft++] = GPS_MSG_3500_CHECKSUM;
		msgSimBuf[msgSimWordsLeft++] = 0; /* timetag */
		msgSimBuf[msgSimWordsLeft++] = 0; /* timetag */
		msgSimBuf[msgSimWordsLeft++] = 0; /* timetag */
		msgSimBuf[msgSimWordsLeft++] = 0; /* timetag */

		if (m_frameCount < m_frameCountAtGPSInit)
		    gpsMode = 0;
		else if (m_frameCount - m_frameCountAtGPSInit < 1000) {
		    if (m_block->currentGPSInitMode() == GPSINITMODE_GROUND)
			gpsMode = GPS_MODE_FINE_ALIGNMENT;
		    else gpsMode = GPS_MODE_AIR_ALIGNMENT;
		}
		else gpsMode = GPS_MODE_AIR_NAVIGATION;
		msgSimBuf[msgSimWordsLeft++] = gpsMode;

		msgSimBuf[msgSimWordsLeft++] = 1; /* system status */
		msgSimBuf[msgSimWordsLeft++] = 0; /* SVs tracked */
		msgSimBuf[msgSimWordsLeft++] = 0; /* pos measurements */
		msgSimBuf[msgSimWordsLeft++] = 0; /* vel measurements */
		msgSimBuf[msgSimWordsLeft++] = 0; /* FOM info */
		msgSimBuf[msgSimWordsLeft++] = 0; /* hor pos error */
		msgSimBuf[msgSimWordsLeft++] = 0; /* hor pos error */
		msgSimBuf[msgSimWordsLeft++] = 0; /* vert pos error */
		msgSimBuf[msgSimWordsLeft++] = 0; /* vert pos error */
		msgSimBuf[msgSimWordsLeft++] = 0; /* velocity error */
		msgSimBuf[msgSimWordsLeft++] = 0; /* velocity error */
		msgSimBuf[msgSimWordsLeft++] = 0; /* data checksum */
	    }
	}
    }

    if ((m_frameCount % msg3Interval) == 0) {
	if (msgSimWordsLeft + GPS_MSG_3501_WORDS > 
		(int)(sizeof(msgSimBuf) / sizeof(short)))
	    printf("GPS msg buffer overflow!\n");
	else {
	    msgSimBuf[msgSimWordsLeft++] = GPS_MAGIC_NUMBER;
	    msgSimBuf[msgSimWordsLeft++] = 3501;
	    msgSimBuf[msgSimWordsLeft++] = GPS_MSG_3501_WORDS - 6;
	    msgSimBuf[msgSimWordsLeft++] = 0x8000;
	    msgSimBuf[msgSimWordsLeft++] = GPS_MSG_3501_CHECKSUM;
	    msgSimBuf[msgSimWordsLeft++] = 0; /* timetag */
	    msgSimBuf[msgSimWordsLeft++] = 0; /* timetag */
	    msgSimBuf[msgSimWordsLeft++] = 0; /* timetag */
	    msgSimBuf[msgSimWordsLeft++] = 0; /* timetag */

	    if (gpsMode == GPS_MODE_AIR_NAVIGATION) {
		if (baseLat == 0) {
		    baseLat = (int)(
			(rand() / (double)RAND_MAX - 0.5) *
			    pow(2.0, 32.0));
		    baseLon = (int)(
			(rand() / (double)RAND_MAX - 0.5) *
			    pow(2.0, 32.0));
		}
		lat = baseLat +
		    (int)(simIteration / -100000.0 * pow(2.0,32.0));
		lon = baseLon +
		    (int)(simIteration / 10000.0 * pow(2.0,32.0));
	    }
	    else lat = lon = 0;
	    msgSimBuf[msgSimWordsLeft++] = lat & 0xffff;
	    msgSimBuf[msgSimWordsLeft++] = lat >> 16;
	    msgSimBuf[msgSimWordsLeft++] = lon & 0xffff;
	    msgSimBuf[msgSimWordsLeft++] = lon >> 16;

	    altitude = simIteration * 100;
	    if (altitude > 20000)
		altitude = 20000;
	    altitude += (rand() / (double)RAND_MAX - 0.5) * 1000;
	    altitude = (int)(altitude * 12 / 39.3701 *
		pow(2.0, 16.0)); /* ft to fixed-point m */
	    msgSimBuf[msgSimWordsLeft++] = altitude & 0xffff;
	    msgSimBuf[msgSimWordsLeft++] = altitude >> 16;

	    if (gpsMode == GPS_MODE_AIR_NAVIGATION) {
		velNorth = (int)(83 * pow(2.0, 21.0));  /* fixed m/s */
		velEast = (int)(60 * pow(2.0, 21.0)); /* fixed m/s */
		velUp = (int)(-10 * pow(2.0, 21.0));     /* fixed m/s */
	    }
	    else velNorth = velEast = velUp = 0;
	    msgSimBuf[msgSimWordsLeft++] = velNorth & 0xffff;
	    msgSimBuf[msgSimWordsLeft++] = velNorth >> 16;
	    msgSimBuf[msgSimWordsLeft++] = velEast & 0xffff;
	    msgSimBuf[msgSimWordsLeft++] = velEast >> 16;
	    msgSimBuf[msgSimWordsLeft++] = velUp & 0xffff;
	    msgSimBuf[msgSimWordsLeft++] = velUp >> 16;

	    if (gpsMode == GPS_MODE_AIR_NAVIGATION) {
		pitch = (int)(
		    (sin((simIteration % 360) / 360.0 * 2*PI) / 10.0) *
			pow(2.0, 31.0));
		roll = (int)(
		    (cos((simIteration % 360) / 360.0 * 2*PI) / 20.0) *
			pow(2.0, 31.0));
		heading = pitch+roll;
	    }
	    else pitch = roll = heading = 0;
	    msgSimBuf[msgSimWordsLeft++] = pitch & 0xffff;
	    msgSimBuf[msgSimWordsLeft++] = pitch >> 16;
	    msgSimBuf[msgSimWordsLeft++] = roll & 0xffff;
	    msgSimBuf[msgSimWordsLeft++] = roll >> 16;
	    msgSimBuf[msgSimWordsLeft++] = heading & 0xffff;
	    msgSimBuf[msgSimWordsLeft++] = heading >> 16;
	    msgSimBuf[msgSimWordsLeft++] = 0; /* data checksum */
	    simIteration++;
	}
    }
    if (msgSimWordsLeft != 0) {
	chunk = msgSimWordsLeft;
	if (chunk > 21)
	    chunk = 21;
	usp = m_imageBuffer + GPS_IMAGE_OFFSET / sizeof(short);
	*usp++ = 0xBABE;
	*usp++ = chunk;
	memset(usp, 0, 21 * sizeof(short));
	memcpy(usp, msgSimBuf, chunk * sizeof(short));
	for (i=0;i < msgSimWordsLeft-chunk;i++)
	    msgSimBuf[i] = msgSimBuf[i+chunk];
	msgSimWordsLeft -= chunk;
    }
    *(m_imageBuffer + TIMESTAMP_IMAGE_OFFSET / sizeof(short)) =
	m_frameCount & 0xffff;
}


void DisplayTab::processImageTimingData(void)
{
    u_short thisTimeStamp;
    static u_short lastTimeStamp = 0;
    static int fpiePPSAcquisitionFailures = 0;
    static int msg3AcquisitionFailures = 0;
    u_int thisFPGAPPSCount;
    static u_int lastFPGAPPSCount = 0;
    static int fpgaPPSFailures = 0;

	/* check timestamp within image to verify PPS-FPIE connection */

    (void)memcpy(&thisTimeStamp,
	m_imageBuffer + TIMESTAMP_IMAGE_OFFSET / sizeof(short),
	sizeof(short));
    if (thisTimeStamp == 0 || thisTimeStamp == lastTimeStamp) {
	fpiePPSAcquisitionFailures++;
	if (fpiePPSAcquisitionFailures >
		1.5 * appFrame::fb->getFrameRateHz()) {
	    m_fpiePPSCheckColor = StatusDisplay::COLOR_RED;
	}
    }
    else {
	fpiePPSAcquisitionFailures = 0;
	m_fpiePPSCheckColor = StatusDisplay::COLOR_GREEN;
    }
    lastTimeStamp = thisTimeStamp;

	/* check PPS data within image to verify time message receipt */

    if (*(m_imageBuffer + PPS_IMAGE_OFFSET / sizeof(short)) == 0xDEAD) {
	msg3AcquisitionFailures++;
	if (msg3AcquisitionFailures > 1.5 * appFrame::fb->getFrameRateHz())
	    m_msg3CheckColor = StatusDisplay::COLOR_RED;
    }
    else {
	msg3AcquisitionFailures = 0;
	m_msg3CheckColor = StatusDisplay::COLOR_GREEN;
	m_msg3Count++;
    }

	/* check FPGA to verify PPS-FPGA connection */

    thisFPGAPPSCount = appFrame::fb->fpgaPPSCount();
    if (thisFPGAPPSCount == 0 || thisFPGAPPSCount == lastFPGAPPSCount) {
	fpgaPPSFailures++;
	if (fpgaPPSFailures > 1.5 * appFrame::fb->getFrameRateHz()) {
	    m_fpgaPPSCheckColor = StatusDisplay::COLOR_RED;
	}
    }
    else {
	fpgaPPSFailures = 0;
	m_fpgaPPSCheckColor = StatusDisplay::COLOR_GREEN;
    }
    lastFPGAPPSCount = thisFPGAPPSCount;
}


void DisplayTab::processImageGPSData(void)
{
    u_short *usp, wordCount, cs;
    int i, j;
    static int gpsMsgParseState = 0;
    static int gpsMsgWordsBuffered = 0;
    static u_short lastGPSMsg[GPS_MSG_MAX_WORDS];
    static int gps3501skip = 0;
    static int gpsCommFailures = 0;

	/* if we've got the start of a GPS update, buffer it.  if we've
	   got a full GPS update, process it.  I need to use a state
	   machine to parse nav messages here because the magic number
	   (81ff) can occur naturally as part of the data, at least in
	   theory */

    usp = m_imageBuffer + GPS_IMAGE_OFFSET / sizeof(short);
    if (*usp++ == 0xDEAD) {
	gpsCommFailures++;
	if (gpsCommFailures > 1.5 * appFrame::fb->getFrameRateHz()) {
	    m_gpsCommCheckColor = StatusDisplay::COLOR_RED;
	    m_airNavCheckColor = StatusDisplay::COLOR_YELLOW;
	    m_gpsValidCheckColor = StatusDisplay::COLOR_YELLOW;
	}
    }
    else {
	gpsCommFailures = 0;
	m_gpsCommCheckColor = StatusDisplay::COLOR_GREEN;
	m_gpsCount++;

	wordCount = *usp++;
	for (i=0;i < wordCount;i++,usp++) {
	    switch (gpsMsgParseState) {
		case 0: /* look for 81ff */
		    if (*usp == GPS_MAGIC_NUMBER) {
			gpsMsgWordsBuffered = 0;
			lastGPSMsg[gpsMsgWordsBuffered++] = *usp;
			gpsMsgParseState = 1;
		    }
		    break;
		case 1: /* look for message type */
		    if (*usp == 3500 || *usp == 3501) {
			lastGPSMsg[gpsMsgWordsBuffered++] = *usp;
			gpsMsgParseState = 2;
		    }
		    else if (*usp == GPS_MAGIC_NUMBER)
			/* do nothing */ ;
		    else gpsMsgParseState = 0;
		    break;
		case 2: /* look for length - 6 */
		    if (*usp == GPS_MAGIC_NUMBER) { /* not valid length */
			gpsMsgWordsBuffered = 1;
			gpsMsgParseState = 1;
		    }
		    else if ((*(usp-1) == 3500 &&
				*usp == GPS_MSG_3500_WORDS-6) ||
			    (*(usp-1) == 3501 &&
				*usp == GPS_MSG_3501_WORDS-6)) {
			lastGPSMsg[gpsMsgWordsBuffered++] = *usp;
			gpsMsgParseState = 3;
		    }
		    else gpsMsgParseState = 0;
		    break;
		case 3: /* look for flags word */
		    if (*usp == GPS_MAGIC_NUMBER) { /* not valid word */
			gpsMsgWordsBuffered = 1;
			gpsMsgParseState = 1;
		    }
		    else {
			lastGPSMsg[gpsMsgWordsBuffered++] = *usp;
			gpsMsgParseState = 4;
		    }
		    break;
		case 4: /* look for checksum */
		    lastGPSMsg[gpsMsgWordsBuffered++] = *usp;
		    gpsMsgParseState = 5;
		    break;
		case 5: /* validate checksum */
		    cs = 0;
		    for (j=0;j < 4;j++)
			cs += lastGPSMsg[j];
		    cs = (~cs)+1;
		    if (cs == lastGPSMsg[4]) {
			lastGPSMsg[gpsMsgWordsBuffered++] = *usp;
			gpsMsgParseState = 6;
		    }
		    else if (lastGPSMsg[4] == GPS_MAGIC_NUMBER) {
			if (*usp == 3500 || *usp == 3501) {
			    gpsMsgWordsBuffered = 2;
			    lastGPSMsg[1] = *usp;
			    gpsMsgParseState = 2;
			}
			else if (*usp == GPS_MAGIC_NUMBER) {
			    gpsMsgWordsBuffered = 1;
			    gpsMsgParseState = 1;
			}
			else gpsMsgParseState = 0;
		    }
		    else if (*usp == GPS_MAGIC_NUMBER) {
			gpsMsgWordsBuffered = 1;
			gpsMsgParseState = 1;
		    }
		    else gpsMsgParseState = 0;
		    break;
		case 6:
		    lastGPSMsg[gpsMsgWordsBuffered++] = *usp;
		    if (lastGPSMsg[1] == 3500 &&
			    gpsMsgWordsBuffered == GPS_MSG_3500_WORDS) {
			switch (lastGPSMsg[9]) {
			    case GPS_MODE_FINE_ALIGNMENT:
			    case GPS_MODE_AIR_ALIGNMENT:
				m_airNavCheckColor =
				    StatusDisplay::COLOR_YELLOW;
				break;
			    case GPS_MODE_AIR_NAVIGATION:
				m_airNavCheckColor =
				    StatusDisplay::COLOR_GREEN;
				break;
			    default:
				m_airNavCheckColor =
				    StatusDisplay::COLOR_RED;
				break;
			}
			if (lastGPSMsg[10] & 0x1)
			    m_gpsValidCheckColor =
				StatusDisplay::COLOR_GREEN;
			else m_gpsValidCheckColor =
			    StatusDisplay::COLOR_RED;
			gpsMsgParseState = 0;
		    }
		    else if (lastGPSMsg[1] == 3501 &&
			    gpsMsgWordsBuffered == GPS_MSG_3501_WORDS) {
			if (gps3501skip <= 0) {
			    add3501(lastGPSMsg);
			    if (!appFrame::headless)
				m_refreshNavDisplay();
			    gps3501skip = m_block->currentNavDurationSkip();
			}
			else gps3501skip--;
			gpsMsgParseState = 0;
		    }
		    break;
		default:
		    gpsMsgParseState = 0; /* should never happen */
		    break;
	    }
	}
    }
}


void DisplayTab::displayCurrentFrame(void)
{
    int nLines, i, j, pixel;
    u_short *usp, *dfp;
    u_char *ucp;

    if (m_block->currentViewerType() == VIEWERTYPE_FRAME)
	m_incr = 1;
    else /* VIEWERTYPE_COMBO */ m_incr = 2;
    nLines = m_frameHeightLines - 1;
    ucp = m_framePixels;
    usp = m_imageBuffer + m_frameWidthSamples;
    dfp = m_darkFrame + m_frameWidthSamples;
    for (i = nLines;i > 0;i-=m_incr) {
	for (j = m_frameWidthSamples;j > 0;j-=m_incr) {
	    pixel = *usp;
	    if (m_block->currentDarkSubOption() == DARKSUBOPTION_YES) {
		pixel -= *dfp;
		if (pixel < 0) pixel = 0;
	    }
	    u_char value = m_stretchLUT[pixel];
	    *ucp++ = value;
	    *ucp++ = value;
	    *ucp++ = value;
	    usp += m_incr;
	    dfp += m_incr;
	}
	usp += (m_incr-1) * m_frameWidthSamples;
	dfp += (m_incr-1) * m_frameWidthSamples;
    }
#if defined(LINETEST)
    ucp = m_framePixels;
    *(ucp+3) = *(ucp+5) = 0;
    *(ucp+4) = 255;
    ucp += m_frameWidthSamples/incr*3;
    *(ucp-6) = *(ucp-4) = 0;
    *(ucp-5) = 255;
    for (j = m_frameWidthSamples;j > 0;j-=incr) {
	*ucp = *(ucp+2) = 0;
	*(ucp+1) = 255;
	ucp += 3;
    }
    if (nLines % 2 == 1 && incr == 2)
	nLines--;
    for (i = nLines-4*incr;i > 0;i-=incr) {
	*(ucp+3) = *(ucp+5) = 0;
	*(ucp+4) = 255;
	ucp += 6;
	ucp += 3 * ((m_frameWidthSamples-4*incr)/incr);
	*ucp = *(ucp+2) = 0;
	*(ucp+1) = 255;
	ucp += 6;
    }
    for (j = m_frameWidthSamples;j > 0;j-=incr) {
	*ucp = *(ucp+2) = 0;
	*(ucp+1) = 255;
	ucp += 3;
    }
    *(ucp+3) = *(ucp+5) = 0;
    *(ucp+4) = 255;
    ucp += m_frameWidthSamples/incr*3;
    *(ucp-6) = *(ucp-4) = 0;
    *(ucp-5) = 255;
#endif
    m_refreshFrameDisplay();
}


void DisplayTab::displayWaterfallLine(void)
{
    u_char *ucp;
    u_short *usp_red, *usp_green, *usp_blue;
    int red, green, blue, j;
    unsigned short *df_red, *df_green, *df_blue;

    if (m_skipLeft <= 0) {
	if (m_block->currentViewerType() == VIEWERTYPE_COMBO)
	    m_incr = 2;
	else m_incr = 1;
	ucp = m_waterfallPixels;
	usp_red = m_imageBuffer + m_frameWidthSamples +
	    (m_block->currentRedBand() - 1) * m_frameWidthSamples;
	usp_green = m_imageBuffer + m_frameWidthSamples +
	    (m_block->currentGreenBand() - 1) * m_frameWidthSamples;
	usp_blue = m_imageBuffer + m_frameWidthSamples +
	    (m_block->currentBlueBand() - 1) * m_frameWidthSamples;
	df_red = m_darkFrame + m_frameWidthSamples +
	    (m_block->currentRedBand() - 1) * m_frameWidthSamples;
	df_green = m_darkFrame + m_frameWidthSamples +
	    (m_block->currentGreenBand() - 1) * m_frameWidthSamples;
	df_blue = m_darkFrame + m_frameWidthSamples +
	    (m_block->currentBlueBand() - 1) * m_frameWidthSamples;
	for (j = m_frameWidthSamples;j > 0;j-=m_incr) {
	    red = *usp_red;
	    green = *usp_green;
	    blue = *usp_blue;
	    if (m_block->currentDarkSubOption() == DARKSUBOPTION_YES) {
		red -= *df_red;
		if (red < 0) red = 0;
		green -= *df_green;
		if (green < 0) green = 0;
		blue -= *df_blue;
		if (blue < 0) blue = 0;
	    }
	    *ucp++ = m_stretchLUT[red];
	    *ucp++ = m_stretchLUTGreen[green];
	    *ucp++ = m_stretchLUTBlue[blue];
	    usp_red += m_incr;
	    usp_green += m_incr;
	    usp_blue += m_incr;
	    df_red += m_incr;
	    df_green += m_incr;
	    df_blue += m_incr;
	}
#if defined(LINETEST)
	ucp = m_waterfallPixels;
	*(ucp+3) = *(ucp+5) = 0;
	*(ucp+4) = 255;
	ucp += 6;
	ucp += 3 * ((m_frameWidthSamples-4*incr)/incr);
	*ucp = *(ucp+2) = 0;
	*(ucp+1) = 255;
#endif
	m_refreshWaterfallDisplay();

	m_skipLeft = m_block->currentDowntrackSkip();
    }
    else m_skipLeft--;
}


int DisplayTab::checkForMoreImageData(int consecutiveFrames)
{
    int dataAvailable, framesExpected;
    static int hadCpuError = -1;
    static u_int nominalDrops = 0;
    struct timeval recordEndTime;
    double secs;
    char buf[80];

	/* if simulating record, maintain dataAvailable to keep up with
	   expected data rate.  if simulating non-record, we can bail out.
	   bail out if we have too many consecutive frames so that the
	   user interface doesn't become unresponsive.  this may encourage
	   data loss, but in that case we'd probably be dying sooner or
	   later anyway. */

    if (m_block->currentImageSim() == IMAGESIM_NONE) {
	if (consecutiveFrames >= m_maxConsecutiveFrames)
	    dataAvailable = 0;
	else dataAvailable = appFrame::fb->frameIsAvailable();

	appFrame::fb->getStats(&m_framesDropped, &m_frameBacklog);
	if (m_frameBacklog > m_maxFrameBacklog)
	    m_maxFrameBacklog = m_frameBacklog;
	if (m_framesDropped > nominalDrops) {
	    if (hadCpuError < 2) {
		m_cpuCheckColor = StatusDisplay::COLOR_RED;
		hadCpuError = 2;
		if (m_recordState != RECORD_STATE_NOT_RECORDING) {
		    (void)sprintf(buf, "Alpha Data reports %d frames dropped.",
			m_framesDropped);
		    log(buf);
		}
	    }
	    nominalDrops = m_framesDropped;
	}
	else if (m_frameBacklog > 3) {
	    if (hadCpuError < 1) {
		m_cpuCheckColor = StatusDisplay::COLOR_YELLOW;
		hadCpuError = 1;
	    }
	    else if (hadCpuError == 2 &&
		    m_recordState == RECORD_STATE_NOT_RECORDING) {
		m_cpuCheckColor = StatusDisplay::COLOR_YELLOW;
		hadCpuError = 1;
	    }
	}
	else if (hadCpuError == -1 || hadCpuError == 1 ||
		(hadCpuError == 2 &&
		    m_recordState == RECORD_STATE_NOT_RECORDING)) {
	    m_cpuCheckColor = StatusDisplay::COLOR_GREEN;
	    hadCpuError = 0;
	}
    }
    else { /* simulating */
	if (consecutiveFrames >= m_maxConsecutiveFrames) {
	    if (hadCpuError < 1) {
		m_cpuCheckColor = StatusDisplay::COLOR_YELLOW;
		hadCpuError = 1;
	    }
	    dataAvailable = 0;
	}
	else if (m_recordState == RECORD_STATE_NOT_RECORDING) {
	    dataAvailable = 0;
	    if (hadCpuError != 0) {
		m_cpuCheckColor = StatusDisplay::COLOR_GREEN;
		hadCpuError = 0;
	    }
	}
	else /* recording of simulated data */ {
	    (void)gettimeofday(&recordEndTime, NULL);
	    secs = (recordEndTime.tv_sec - m_recordStartTime.tv_sec) +
		((int)recordEndTime.tv_usec -
		    (int)m_recordStartTime.tv_usec) / 1.0e6;
	    framesExpected = (int)(secs * appFrame::fb->getFrameRateHz());
	    if (m_framesWritten >= framesExpected) {
		dataAvailable = 0;
		if (hadCpuError != 0) {
		    m_cpuCheckColor = StatusDisplay::COLOR_GREEN;
		    hadCpuError = 0;
		}
	    }
	    else { /* allow a little lag, up to 200 frames worth */
		if (framesExpected - m_framesWritten < 200) {
		    dataAvailable = 1;
		    if (hadCpuError != 0) {
			m_cpuCheckColor = StatusDisplay::COLOR_GREEN;
			hadCpuError = 0;
		    }
		}
		else {
		    if (hadCpuError != 2) {
			m_cpuCheckColor = StatusDisplay::COLOR_RED;
			hadCpuError = 2;
		    }
		    // enterIdleState();
		    dataAvailable = 0;
		}
	    }
	}
    }
    return dataAvailable;
}


void DisplayTab::indicatorUpdateWork(void)
{
    m_idleDisplay.setColor(m_idleColor);
    m_startingCalDisplay.setColor(m_startingCalColor);
    m_activeDisplay.setColor(m_activeColor);
    m_endingCalDisplay.setColor(m_endingCalColor);
    m_imagerCheckDisplay.setColor(m_imagerCheckColor);
    m_gpsCommCheckDisplay.setColor(m_gpsCommCheckColor);
    m_fpiePPSCheckDisplay.setColor(m_fpiePPSCheckColor);
    m_msg3CheckDisplay.setColor(m_msg3CheckColor);
    m_cpuCheckDisplay.setColor(m_cpuCheckColor);
    m_tempCheckDisplay.setColor(m_tempCheckColor);
    m_airNavCheckDisplay.setColor(m_airNavCheckColor);
    m_gpsValidCheckDisplay.setColor(m_gpsValidCheckColor);
    m_fpgaPPSCheckDisplay.setColor(m_fpgaPPSCheckColor);
    m_mountCheckDisplay.setColor(m_mountCheckColor);
    m_fmsCheckDisplay.setColor(m_fmsCheckColor);
}


void DisplayTab::updateHeadlessIndicators(void)
{
    writeSerialStringForColor(m_imagerCheckColor, "IM");
    writeSerialStringForColor(m_gpsCommCheckColor, "GC");
    writeSerialStringForColor(m_fpiePPSCheckColor, "P1");
    writeSerialStringForColor(m_msg3CheckColor, "M3");
    writeSerialStringForColor(m_cpuCheckColor, "CP");
    writeSerialStringForColor(m_tempCheckColor, "TE");
    writeSerialStringForColor(m_airNavCheckColor, "GI");
    writeSerialStringForColor(m_gpsValidCheckColor, "GV");
    writeSerialStringForColor(m_fpgaPPSCheckColor, "P2");
    writeSerialStringForColor(m_mountCheckColor, "MC");
    writeSerialStringForColor(m_fmsCheckColor, "FM");
}


void DisplayTab::writeSerialStringForColor(StatusDisplay::color_t color,
    const char *prefix)
{
    char status[3];

    if (m_headlessDevFd != -1) {
	assert(strlen(prefix) == 2);
	memcpy(status, prefix, 2);
	switch (color) {
	    case StatusDisplay::COLOR_GREEN:
		status[2] = '0';
		appFrame::writePCSerial(m_headlessDevFd, status, 3);
		break;
	    case StatusDisplay::COLOR_YELLOW:
		status[2] = '1';
		appFrame::writePCSerial(m_headlessDevFd, status, 3);
		break;
	    case StatusDisplay::COLOR_RED:
		status[2] = '2';
		appFrame::writePCSerial(m_headlessDevFd, status, 3);
		break;
	    case StatusDisplay::COLOR_UNUSED:
		break;
	    default:
		assert(0);
	}
    }
}


void DisplayTab::frameUpdateWork(void)
{
    m_imageDisplay.displayFrame(m_framePixels, (m_frameHeightLines-1) / m_incr,
	m_frameWidthSamples / m_incr, (m_block->currentRedBand()-1) / m_incr,
	(m_block->currentGreenBand()-1) / m_incr,
	(m_block->currentBlueBand()-1) / m_incr);
}


void DisplayTab::waterfallUpdateWork(void)
{
    m_imageDisplay.displayLine(m_waterfallPixels, m_frameWidthSamples / m_incr);
}


void DisplayTab::navDisplayUpdateWork(void)
{
    draw();
}


#define FPS_COUNT	20

void DisplayTab::diagsUpdateWork(void)
{
    static struct timeval lastTime;
    struct timeval currentTime;
    double timeDelta, rate;
    static int lastFrameCount;
    int frameCountDelta;
    static double values[FPS_COUNT] = { 0.0 };
    static int numValues = 0;
    static int nextValue = 0;
    static double sum = 0.0;
    char buf[20];

    (void)gettimeofday(&currentTime, NULL);
    if (lastTime.tv_sec != 0) {
	timeDelta = currentTime.tv_sec - lastTime.tv_sec +
	    (currentTime.tv_usec - lastTime.tv_usec) / 1000000.0;
	frameCountDelta = m_fpgaFrameCount - lastFrameCount;
	rate = frameCountDelta / timeDelta;

	sum -= values[nextValue];
	values[nextValue] = rate;
	sum += values[nextValue];
	nextValue = (nextValue+1) % FPS_COUNT;
	if (numValues < FPS_COUNT)
	    numValues++;
	if (numValues < FPS_COUNT)
	    sprintf(buf, "Frame rate: Calculating...");
	else sprintf(buf, "Frame rate: %.1f fps", sum / numValues);
	m_fpsLabel.set_text(buf);
    }

    if (m_diagTab) {
	m_diagTab->setFrameCount(m_frameCount);
	m_diagTab->setFramesDropped(m_framesDropped);
	m_diagTab->setFrameBacklog(m_frameBacklog);
	m_diagTab->setMaxFrameBacklog(m_maxFrameBacklog);
	m_diagTab->setMsg3Count(m_msg3Count);
	m_diagTab->setGPSCount(m_gpsCount);
	m_diagTab->setLocalFrameCount(m_localFrameCount);
	m_diagTab->setTemps(m_numSensors, m_sensorNames, m_sensorValues);
	m_diagTab->setFPGAFrameCount(m_fpgaFrameCount);
	m_diagTab->setFPGAPPSCount(m_fpgaPPSCount);
	m_diagTab->setFPGA81ffCount(m_fpga81ffCount);
	m_diagTab->setFPGAFramesDropped(m_fpgaFramesDropped);
	m_diagTab->setFPGASerialErrors(m_fpgaSerialErrors);
	m_diagTab->setFPGASerialPortCtl(m_fpgaSerialPortCtl);
	m_diagTab->setFPGABufferDepth(m_fpgaBufferDepth);
	m_diagTab->setFPGAMaxBuffersUsed(m_fpgaMaxBuffersUsed);
    }

    lastTime = currentTime;
    lastFrameCount = m_fpgaFrameCount;
}


void DisplayTab::handleFMSRecord(void)
{
    recordButtonWork(0);
}


void DisplayTab::onRecordButton(void)
{
    recordButtonWork(1);
}


void DisplayTab::recordButtonWork(int userInvoked)
{
    char confirmation[200];

	/* log that user pressed record button */

    if (userInvoked)
	log("User pressed Record button.");
    else log("FMS Control pressed Record button.");

	/* make sure OBC is in auto, unless we're in test mode */

    if (m_block->currentOBCInterface() == OBCINTERFACE_NONE ||
	    m_mostRecentOBCMode == OBC_AUTO)
	/* do nothing */ ;
    else if (appFrame::headless) {
	warn("Setting OBC to operational Auto.");
	m_mostRecentOBCMode = OBC_AUTO;
    }
    else if (m_block->currentMode() == MODE_OPERATIONAL) {
	warn("Setting OBC to operational Auto.");
	m_obcControlComboForced = true;
	m_obcControlCombo->set_active_text(obcControlOptions[OBC_AUTO]);

	m_mostRecentOBCMode = OBC_AUTO;
    }
    else /* MODE_TEST */ {
	Gtk::MessageDialog d(
	    "You are in test mode and OBC control is not set to OBC Auto.\n\n"
		"Are you sure you want to record?",
	    false /* no markup */, Gtk::MESSAGE_QUESTION,
	    Gtk::BUTTONS_YES_NO);
	if ((Gtk::ResponseType)d.run() != Gtk::RESPONSE_YES) {
	    log("User cancelled test-mode record.");
	    return;
	}
	else log("User confirmed record with OBC not in Auto mode.");
    }

	/* make sure mount is in auto, unless we're in test mode */

    if (m_block->currentMountUsed() == MOUNTUSED_NO ||
	    m_mostRecentMountMode == MOUNT_AUTO)
	/* do nothing */ ;
    else if (appFrame::headless) {
	warn("Setting mount to operational Auto.");
	m_mostRecentMountMode = MOUNT_AUTO;
    }
    else if (m_block->currentMode() == MODE_OPERATIONAL) {
	warn("Setting mount to operational Auto.");
	m_mountControlComboForced = true;
	m_mountControlCombo->set_active_text(mountControlOptions[MOUNT_AUTO]);

	m_mostRecentMountMode = MOUNT_AUTO;
    }
    else /* MODE_TEST */ {
	Gtk::MessageDialog d(
	    "You are in test mode and mount mode is not set to Auto.\n\n"
		"Are you sure you want to record?",
	    false /* no markup */, Gtk::MESSAGE_QUESTION,
	    Gtk::BUTTONS_YES_NO);
	if ((Gtk::ResponseType)d.run() != Gtk::RESPONSE_YES) {
	    log("User cancelled test-mode record.");
	    return;
	}
	else log("User confirmed record with mount not in Auto mode.");
    }

	/* make sure we're at max frame rate, unless we're in test mode */

    if (m_block->currentFrameRate() == NUM_FRAMERATES-1)
	/* do nothing */ ;
    else if (appFrame::headless) {
	warn("Resuming operational frame rate.");
	m_block->setFrameRate(0);
	updateResourcesDisplay();
	appFrame::fb->setFrameRate(0);
    }
    else if (m_block->currentMode() == MODE_OPERATIONAL) {
	warn("Resuming operational frame rate.");
	m_settingsTab->forceDefaultFrameRate();
    }
    else /* MODE_TEST */ {
	(void)sprintf(confirmation,
	    "You are in test mode and the frame rate is set to %.1f Hz.\n\n"
		"Are you sure you want to record?",
	    appFrame::fb->getFrameRateHz());
	Gtk::MessageDialog d(confirmation,
	    false /* no markup */, Gtk::MESSAGE_QUESTION,
	    Gtk::BUTTONS_YES_NO);
	if ((Gtk::ResponseType)d.run() != Gtk::RESPONSE_YES) {
	    log("User cancelled test-mode record.");
	    return;
	}
	else log("User confirmed record at off-nominal frame rate.");
    }

	/* if we can open files, start recording */

    if (openFiles() != -1) {
	leaveIdleState();

	log("Before recording...");
	saveStateToLog();

	m_framesWritten = 0;
	(void)gettimeofday(&m_recordStartTime, NULL);
	if (!appFrame::headless) {
	    m_app->disableExitButton();
	    enableAbortButton();
	}

	if (m_block->currentMountUsed() == MOUNTUSED_YES &&
		m_mostRecentMountMode == MOUNT_AUTO)
	    (void)setMount(MOUNT_ACTIVE);

	if (m_block->currentDark1CalPeriodInSecs() == 0 ||
		m_mostRecentOBCMode != OBC_AUTO ||
		m_block->currentOBCInterface() == OBCINTERFACE_NONE)
	    enterScienceState();
	else enterDark1CalState();
    }
}


int DisplayTab::writeImageFrame(void)
{
    char msg[MAX_ERROR_LEN];

    if (fwrite(m_imageBuffer, m_frameWidthSamples * sizeof(short),
		(unsigned)m_frameHeightLines, m_imageFP) !=
	    (unsigned)m_frameHeightLines) {
	(void)sprintf(msg, "Image write failed -- is filesystem full?");
	error(msg);
	return -1;
    }
    m_framesWritten++;
    return 0;
}


int DisplayTab::writeFlightlineGPSData(void)
{
    static int suppressGPSErrors = 0;

    if (writeDataFromLine1(m_gpsFP, GPS_IMAGE_OFFSET, MAX_GPSDATA_BYTES,
	    "Write of flight-line GPS data failed.", &suppressGPSErrors,
	    0) == -1)
	return -1;
    return 0;
}


int DisplayTab::writePPSData(void)
{
    static int suppressPPSErrors = 0;

    if (writeDataFromLine1(m_ppsFP, PPS_IMAGE_OFFSET, MAX_PPSDATA_BYTES,
	    "Write of flight-line PPS data failed.", &suppressPPSErrors,
	    0) == -1)
	return -1;
    return 0;
}


void DisplayTab::writeAllgpsData(void)
{
    static int suppressAllgpsErrors = 0;

    (void)writeDataFromLine1(m_allgpsFP, GPS_IMAGE_OFFSET, MAX_GPSDATA_BYTES,
	"Write to end-to-end GPS file failed.", &suppressAllgpsErrors, 0);
}


void DisplayTab::writeAllppsData(void)
{
    static int suppressAllppsErrors = 0;

    (void)writeDataFromLine1(m_allppsFP, PPS_IMAGE_OFFSET, MAX_PPSDATA_BYTES,
	"Write to end-to-end PPS file failed.", &suppressAllppsErrors, 1);
}


int DisplayTab::writeDataFromLine1(FILE *fp, int imageOffset,
    int maxByteCount, const char *failMsg, int *suppression_p, int flush)
{
    char msg[MAX_ERROR_LEN];
    u_short *usp;
    u_short magic, wordCount;

    if (fp != NULL) {
	usp = m_imageBuffer + imageOffset / sizeof(short);
	magic = *usp++;
	if (magic != 0xDEAD) {
	    wordCount = *usp++;
	    if (wordCount <= maxByteCount / sizeof(short)) {
		if (fwrite(usp, sizeof(short), wordCount, fp) != wordCount) {
		    if (!*suppression_p) {
			(void)strcpy(msg, failMsg);
			(void)strcat(msg, 
			     "  Suppressing further messages.");
			warn(msg);
			*suppression_p = 1;
			return -1;
		    }
		}
		if (flush)
		    fflush(fp);
	    }
	}
    }
    return 0;
}


void DisplayTab::handleFMSStop(void)
{
    if (m_stopButton.get_sensitive())
	stopButtonWork(0);
}


void DisplayTab::onStopButton(void)
{
    if (m_stopButton.get_sensitive())
	stopButtonWork(1);
}


void DisplayTab::stopButtonWork(int userInvoked)
{
    if (userInvoked)
	log("User pressed Stop button.");
    else log("FMS Control pressed Stop button.");
    if (!appFrame::headless)
	m_stopButton.set_sensitive(false);

    m_stopRequested = 1;
    m_stopIsAbort = 0;
}


void DisplayTab::onGPSInitButton(void)
{
    u_short word, cs;
    int i;

    log("User pressed GPS Init button.");
    if (!appFrame::headless) {
	Gtk::MessageDialog d(
	    "This will reinitialize the GPS.\n\n"
		"Do you really want to do this?",
	    false /* no markup */, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO);
	if ((Gtk::ResponseType)d.run() != Gtk::RESPONSE_YES) {
	    log("User cancelled GPS init.");
	    return;
	}
	log("User confirmed GPS init.");
    }
    if (m_block->currentGPSSim() != GPSSIM_NONE)
	m_frameCountAtGPSInit = m_frameCount;
    else {
	word = 0;
	m_gpsInitCommand[word++] = GPS_MAGIC_NUMBER;
	m_gpsInitCommand[word++] = 3510;
	m_gpsInitCommand[word++] = GPS_MSG_3510_WORDS - 6;
	m_gpsInitCommand[word++] = 0x8000;
	m_gpsInitCommand[word++] = GPS_MSG_3510_CHECKSUM;
	m_gpsInitCommand[word++] = 0x0061; 	/* data validity */
	m_gpsInitCommand[word++] = 0; 	/* mode */
	m_gpsInitCommand[word++] = 0; 	/* latitude */
	m_gpsInitCommand[word++] = 0; 	/* latitude */
	m_gpsInitCommand[word++] = 0; 	/* latitude */
	m_gpsInitCommand[word++] = 0; 	/* longitude */
	m_gpsInitCommand[word++] = 0; 	/* longitude */
	m_gpsInitCommand[word++] = 0; 	/* longitude */
	m_gpsInitCommand[word++] = 0; 	/* altitude */
	m_gpsInitCommand[word++] = 0; 	/* altitude */
	m_gpsInitCommand[word++] = 0; 	/* ground speed */
	m_gpsInitCommand[word++] = 0; 	/* ground track */
	m_gpsInitCommand[word++] = 0; 	/* year */
	m_gpsInitCommand[word++] = 0; 	/* days */
	m_gpsInitCommand[word++] = 0; 	/* hours */
	m_gpsInitCommand[word++] = 0; 	/* minutes */
	m_gpsInitCommand[word++] = 0; 	/* seconds */
	m_gpsInitCommand[word++] = 0; 	/* true heading */
	if (m_block->currentGPSInitMode() == GPSINITMODE_AIR)
	    m_gpsInitCommand[word++] = 0x175; 	/* auto align/nav sequence */
	else m_gpsInitCommand[word++] = 0x174;
	m_gpsInitCommand[word++] = 0; 	/* reserved */
	m_gpsInitCommand[word++] = 0; 	/* reserved */

	cs = 0x0000;
	for (i=5;i < GPS_MSG_3510_WORDS-1;i++)
	    cs = cs + m_gpsInitCommand[i];
	cs = (~cs)+1;
	m_gpsInitCommand[word++] = cs; 	/* checksum */

	m_gpsInitBytesSent = 0;
    }
}


void DisplayTab::onAbortButton(void)
{
    if (m_abortButton.get_sensitive())
	abortButtonWork(1);
}


void DisplayTab::abortButtonWork(int userInvoked)
{
    log("User pressed Abort button.");
    if (!appFrame::headless) {
	Gtk::MessageDialog d(
	    "This will stop recording and delete the current flightline.\n\n"
		"Do you really want to abort?",
	    false /* no markup */, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO);
	if ((Gtk::ResponseType)d.run() != Gtk::RESPONSE_YES) {
	    log("User cancelled abort.");
	    return;
	}
	else log("User confirmed abort.");

	m_stopButton.set_sensitive(false);  /* already done if abort follows */
	m_abortButton.set_sensitive(false); /*     stop */
    }

    m_stopRequested = 1;
    m_stopIsAbort = 1;
}


void DisplayTab::leaveIdleState(void)
{
    if (!appFrame::headless) {
	m_recordButton.set_sensitive(false);
	m_gpsInitButton.set_sensitive(false);
    }
    m_stopRequested = m_stopIsAbort = 0; /* already done unless abort confirmed after record finished */
    if (m_recordingsTab)
	m_recordingsTab->disableRecordingsChanges();
    if (m_ecsDataTab)
	m_ecsDataTab->disableECSDataChanges();
    if (m_settingsTab)
	m_settingsTab->disableSettingsChanges();
    m_app->setPXIRecord(1);
}


void DisplayTab::enterDark1CalState(void)
{
    m_wait = (int)(m_block->currentDark1CalPeriodInSecs() *
	appFrame::fb->getFrameRateHz() + 0.5);
    m_recordState = RECORD_STATE_DARK1CAL;
    if (!appFrame::headless)
	showStartingCalMode();
    else if (m_headlessDevFd != -1)
	writeSerialStringForColor(StatusDisplay::COLOR_YELLOW, "RE");
    m_app->setOBC(OBC_DARK1);

    m_framesUntilDark = appFrame::fb->getFrameRateHz() * OBC_SETTLING_TIME_SECS;
    memset(m_darkFrameAccumulation, 0,
	m_frameWidthSamples * m_frameHeightLines * sizeof(int));
    m_darkFramesAccumulated = 0;
}


void DisplayTab::enterScienceState(void)
{
    m_recordState = RECORD_STATE_SCIENCE;
    if (!appFrame::headless)
	showActiveMode();
    else if (m_headlessDevFd != -1)
	writeSerialStringForColor(StatusDisplay::COLOR_GREEN, "RE");
    if (m_block->currentOBCInterface() != OBCINTERFACE_NONE &&
	    m_mostRecentOBCMode == OBC_AUTO)
	m_app->setOBC(OBC_SCIENCE);

    if (!appFrame::headless)
	m_refreshStopButton();
}


void DisplayTab::stopButtonUpdateWork(void)
{
    m_stopButton.set_sensitive(true);
}


void DisplayTab::enterDark2CalState(void)
{
    m_wait = (int)(m_block->currentDark2CalPeriodInSecs() *
	appFrame::fb->getFrameRateHz() + 0.5);
    m_recordState = RECORD_STATE_DARK2CAL;
    if (!appFrame::headless)
	showEndingCalMode();
    else if (m_headlessDevFd != -1)
	writeSerialStringForColor(StatusDisplay::COLOR_YELLOW, "RE");
    if (m_block->currentOBCInterface() != OBCINTERFACE_NONE &&
	    m_mostRecentOBCMode == OBC_AUTO)
	m_app->setOBC(OBC_DARK2);

    m_framesUntilDark = appFrame::fb->getFrameRateHz() * OBC_SETTLING_TIME_SECS;
    memset(m_darkFrameAccumulation, 0,
	m_frameWidthSamples * m_frameHeightLines * sizeof(int));
    m_darkFramesAccumulated = 0;
}


void DisplayTab::enterMediumCalState(void)
{
    m_wait = (int)(m_block->currentMediumCalPeriodInSecs() *
	appFrame::fb->getFrameRateHz() + 0.5);
    m_recordState = RECORD_STATE_MEDIUMCAL;
    if (!appFrame::headless)
	showEndingCalMode();
    else if (m_headlessDevFd != -1)
	writeSerialStringForColor(StatusDisplay::COLOR_YELLOW, "RE");
    if (m_block->currentOBCInterface() != OBCINTERFACE_NONE &&
	    m_mostRecentOBCMode == OBC_AUTO)
	m_app->setOBC(OBC_MEDIUM);
}


void DisplayTab::enterBrightCalState(void)
{
    m_wait = (int)(m_block->currentBrightCalPeriodInSecs() *
	appFrame::fb->getFrameRateHz() + 0.5);
    m_recordState = RECORD_STATE_BRIGHTCAL;
    if (!appFrame::headless)
	showEndingCalMode();
    else if (m_headlessDevFd != -1)
	writeSerialStringForColor(StatusDisplay::COLOR_YELLOW, "RE");
    if (m_block->currentOBCInterface() != OBCINTERFACE_NONE &&
	    m_mostRecentOBCMode == OBC_AUTO)
	m_app->setOBC(OBC_BRIGHT);
}


void DisplayTab::enterLaserCalState(void)
{
    m_wait = (int)(m_block->currentLaserCalPeriodInSecs() *
	appFrame::fb->getFrameRateHz() + 0.5);
    m_recordState = RECORD_STATE_LASERCAL;
    if (!appFrame::headless)
	showEndingCalMode();
    else if (m_headlessDevFd != -1)
	writeSerialStringForColor(StatusDisplay::COLOR_YELLOW, "RE");
    if (m_block->currentOBCInterface() != OBCINTERFACE_NONE &&
	    m_mostRecentOBCMode == OBC_AUTO)
	m_app->setOBC(OBC_LASER);
}


void DisplayTab::enterIdleState(void)
{
    struct timespec closeTime;
    char msg[100+MAXPATHLEN];

	/* let PXI know we're done recording */

    m_app->setPXIRecord(0);

	/* close flight-line files, and remove them if we aborted */

    closeFiles(&closeTime);
    if (m_stopIsAbort) {
	(void)unlink(m_currentRecording.imagepath);
	(void)unlink(m_currentRecording.imagehdrpath);
	(void)unlink(m_currentRecording.ppspath);
	(void)unlink(m_currentRecording.gpspath);
    }

	/* note stats for recording */

    m_currentRecording.endTime = closeTime.tv_sec;
    m_currentRecording.duration = m_currentRecording.endTime -
	m_currentRecording.startTime;
    m_currentRecording.recordingType = RECORDING_FLIGHTLINE;

	/* restore non-recording state */

    m_recordState = RECORD_STATE_NOT_RECORDING;
    if (!appFrame::headless)
	showIdleMode();

    if (m_block->currentOBCInterface() != OBCINTERFACE_NONE &&
	    m_mostRecentOBCMode == OBC_AUTO)
	m_app->setOBC(OBC_SCIENCE);
    if (m_block->currentMountUsed() != MOUNTUSED_NO &&
	    m_mostRecentMountMode == MOUNT_AUTO)
	(void)setMount(MOUNT_IDLE);

    if (!appFrame::headless)
	m_refreshRecordControls();
    else if (m_headlessDevFd != -1)
	writeSerialStringForColor(StatusDisplay::COLOR_RED, "RE");

	/* if we didn't abort, show post-recording status in log */

    if (!m_stopIsAbort) {
	log("After recording...");
	saveStateToLog();
	sprintf(msg, "Image filename is %s.", m_currentRecording.imagepath);
	log(msg);
    }

	/* make sure we got all data up to this point in case we're powered
 	   off soon */

    if (appFrame::headless) {
	fflush(m_logFP);
	fflush(m_allgpsFP);
	fflush(m_allppsFP);
	fflush(m_diagsFP);
    }
}


#define LOG_STRING(descr, s) { \
    (void)sprintf(logMsg, "    " descr " is %s.", s); log(logMsg); }
#define LOG_INT(descr, v) { \
    (void)sprintf(logMsg, "    " descr " = %d.", v); log(logMsg); }
#define LOG_HEX(descr, v) { \
    (void)sprintf(logMsg, "    " descr " = 0x%x.", v); log(logMsg); }
#define LOG_DOUBLE(descr, v, u) { \
    (void)sprintf(logMsg, "    " descr " = %.1f %s.", v, u); log(logMsg); }

void DisplayTab::saveStateToLog(void)
{
    char logMsg[200];
    int i;

    LOG_STRING("Imager light", m_imagerCheckDisplay.color())
    LOG_STRING("GPS Comm light", m_gpsCommCheckDisplay.color())
    LOG_STRING("FPIE PPS light", m_fpiePPSCheckDisplay.color())
    LOG_STRING("Msg 3 light", m_msg3CheckDisplay.color())
    LOG_STRING("CPU light", m_cpuCheckDisplay.color())
    LOG_STRING("Temp light", m_tempCheckDisplay.color())
    LOG_STRING("Air Nav light", m_airNavCheckDisplay.color())
    LOG_STRING("GPS Valid light", m_gpsValidCheckDisplay.color())
    LOG_STRING("FPGA PPS light", m_fpgaPPSCheckDisplay.color())
    LOG_STRING("Mount light", m_mountCheckDisplay.color())
    LOG_STRING("FMS light", m_fmsCheckDisplay.color())

    LOG_INT("Alpha Data frame count", m_frameCount)
    LOG_INT("Alpha Data frames dropped", m_framesDropped)
    LOG_INT("Max Alpha Data frame backlog", m_maxFrameBacklog)
    LOG_INT("Msg3 count", m_msg3Count)
    LOG_INT("GPS count", m_gpsCount)
    for (i=0;i < m_numSensors;i++) {
	(void)sprintf(logMsg, "    %s = %.1f C.", m_sensorNames[i],
	    m_sensorValues[i]);
	log(logMsg);
    }
    LOG_INT("FPGA frame count", m_fpgaFrameCount)
    LOG_INT("FPGA PPS count", m_fpgaPPSCount)
    LOG_INT("FPGA 81ff count", m_fpga81ffCount)
    LOG_INT("FPGA frames dropped", m_fpgaFramesDropped)
    LOG_INT("FPGA serial errors", m_fpgaSerialErrors)
    LOG_HEX("FPGA serial port control word", m_fpgaSerialPortCtl)
    LOG_INT("Local frame count", m_localFrameCount)
    LOG_INT("FPGA buffer depth", m_fpgaBufferDepth)
    LOG_INT("FPGA max buffers used", m_fpgaMaxBuffersUsed)

    LOG_DOUBLE("Latitude", m_lastLat, "deg")
    LOG_DOUBLE("Longitude", m_lastLon, "deg")
    LOG_DOUBLE("Altitude", m_lastAltitude, "ft")
    LOG_DOUBLE("Heading", m_lastHeading, "deg")
    LOG_DOUBLE("Velocity north", m_lastVelNorth, "knots")
    LOG_DOUBLE("Velocity east", m_lastVelEast, "knots")
    LOG_DOUBLE("Velocity up", m_lastVelUp, "knots")
    LOG_DOUBLE("Velocity magnitude", m_lastVelMag, "knots")
}


void DisplayTab::saveCSVHeader(void)
{
    int i;

    (void)fprintf(m_diagsFP, "Time,");
    (void)fprintf(m_diagsFP, "Imager light,");
    (void)fprintf(m_diagsFP, "GPS Comm light,");
    (void)fprintf(m_diagsFP, "FPIE PPS light,");
    (void)fprintf(m_diagsFP, "Msg 3 light,");
    (void)fprintf(m_diagsFP, "CPU light,");
    (void)fprintf(m_diagsFP, "Temp light,");
    (void)fprintf(m_diagsFP, "Air Nav light,");
    (void)fprintf(m_diagsFP, "GPS Valid light,");
    (void)fprintf(m_diagsFP, "FPGA PPS light,");
    (void)fprintf(m_diagsFP, "Mount light,");
    (void)fprintf(m_diagsFP, "FMS light,");
    (void)fprintf(m_diagsFP, "Alpha Data frame count,");
    (void)fprintf(m_diagsFP, "Alpha Data frames dropped,");
    (void)fprintf(m_diagsFP, "Max Alpha Data frame backlog,");
    (void)fprintf(m_diagsFP, "Msg3 count,");
    (void)fprintf(m_diagsFP, "GPS count,");
    for (i=0;i < m_numSensors;i++)
	(void)fprintf(m_diagsFP, "%s,", m_sensorNames[i]);
    (void)fprintf(m_diagsFP, "FPGA frame count,");
    (void)fprintf(m_diagsFP, "FPGA PPS count,");
    (void)fprintf(m_diagsFP, "FPGA 81ff count,");
    (void)fprintf(m_diagsFP, "FPGA frames dropped,");
    (void)fprintf(m_diagsFP, "FPGA serial errors,");
    (void)fprintf(m_diagsFP, "FPGA serial port control word,");
    (void)fprintf(m_diagsFP, "Local frame count,");
    (void)fprintf(m_diagsFP, "FPGA buffer depth,");
    (void)fprintf(m_diagsFP, "FPGA max buffers used,");
    (void)fprintf(m_diagsFP, "Latitude,");
    (void)fprintf(m_diagsFP, "Longitude,");
    (void)fprintf(m_diagsFP, "Altitude,");
    (void)fprintf(m_diagsFP, "Heading,");
    (void)fprintf(m_diagsFP, "Velocity north,");
    (void)fprintf(m_diagsFP, "Velocity east,");
    (void)fprintf(m_diagsFP, "Velocity up,");
    (void)fprintf(m_diagsFP, "Velocity magnitude,");
    (void)fprintf(m_diagsFP, "\n");
}


void DisplayTab::saveStateToCSV(void)
{
    time_t nowSecs;
    struct tm *nowStruct;
    char nowASCII[80];
    int i;

    (void)time(&nowSecs);
    nowStruct = gmtime(&nowSecs);
    (void)strftime(nowASCII, sizeof(nowASCII), "%H:%M:%S", nowStruct);
    (void)fprintf(m_diagsFP, "%s,", nowASCII);

    (void)fprintf(m_diagsFP, "%s,", m_imagerCheckDisplay.color());
    (void)fprintf(m_diagsFP, "%s,", m_gpsCommCheckDisplay.color());
    (void)fprintf(m_diagsFP, "%s,", m_fpiePPSCheckDisplay.color());
    (void)fprintf(m_diagsFP, "%s,", m_msg3CheckDisplay.color());
    (void)fprintf(m_diagsFP, "%s,", m_cpuCheckDisplay.color());
    (void)fprintf(m_diagsFP, "%s,", m_tempCheckDisplay.color());
    (void)fprintf(m_diagsFP, "%s,", m_airNavCheckDisplay.color());
    (void)fprintf(m_diagsFP, "%s,", m_gpsValidCheckDisplay.color());
    (void)fprintf(m_diagsFP, "%s,", m_fpgaPPSCheckDisplay.color());
    (void)fprintf(m_diagsFP, "%s,", m_mountCheckDisplay.color());
    (void)fprintf(m_diagsFP, "%s,", m_fmsCheckDisplay.color());

    (void)fprintf(m_diagsFP, "%d,", m_frameCount);
    (void)fprintf(m_diagsFP, "%d,", m_framesDropped);
    (void)fprintf(m_diagsFP, "%d,", m_maxFrameBacklog);
    (void)fprintf(m_diagsFP, "%d,", m_msg3Count);
    (void)fprintf(m_diagsFP, "%d,", m_gpsCount);
    for (i=0;i < m_numSensors;i++)
	(void)fprintf(m_diagsFP, "%.1f,", m_sensorValues[i]);
    (void)fprintf(m_diagsFP, "%d,", m_fpgaFrameCount);
    (void)fprintf(m_diagsFP, "%d,", m_fpgaPPSCount);
    (void)fprintf(m_diagsFP, "%d,", m_fpga81ffCount);
    (void)fprintf(m_diagsFP, "%d,", m_fpgaFramesDropped);
    (void)fprintf(m_diagsFP, "%d,", m_fpgaSerialErrors);
    (void)fprintf(m_diagsFP, "%d,", m_fpgaSerialPortCtl);
    (void)fprintf(m_diagsFP, "%d,", m_localFrameCount);
    (void)fprintf(m_diagsFP, "%d,", m_fpgaBufferDepth);
    (void)fprintf(m_diagsFP, "%d,", m_fpgaMaxBuffersUsed);

    (void)fprintf(m_diagsFP, "%f,", m_lastLat);
    (void)fprintf(m_diagsFP, "%f,", m_lastLon);
    (void)fprintf(m_diagsFP, "%f,", m_lastAltitude);
    (void)fprintf(m_diagsFP, "%f,", m_lastHeading);
    (void)fprintf(m_diagsFP, "%f,", m_lastVelNorth);
    (void)fprintf(m_diagsFP, "%f,", m_lastVelEast);
    (void)fprintf(m_diagsFP, "%f,", m_lastVelUp);
    (void)fprintf(m_diagsFP, "%f,", m_lastVelMag);
    (void)fprintf(m_diagsFP, "\n");

    if (appFrame::headless)
	(void)fflush(m_diagsFP);
}


void DisplayTab::recordControlsUpdateWork(void)
{
    m_stopButton.set_sensitive(false); /* may already be done */
    m_abortButton.set_sensitive(false); /* may already be done */

    if (m_recordingsTab && !m_stopIsAbort)
	m_recordingsTab->addRecording(&m_currentRecording);

    if (m_block->currentFMSControlOption() == FMSCONTROL_NO)
	m_recordButton.set_sensitive(true);
    m_gpsInitButton.set_sensitive(true);
    if (m_recordingsTab)
	m_recordingsTab->enableRecordingsChanges();
    if (m_ecsDataTab)
	m_ecsDataTab->enableECSDataChanges();
    if (m_settingsTab)
	m_settingsTab->enableSettingsChanges();

    m_app->enableExitButton();
}


int DisplayTab::openFiles(void)
{
    char timeString[MAXPATHLEN];
    char base[MAXPATHLEN];
    char imageFilename[MAXPATHLEN];
    char imageHdrFilename[MAXPATHLEN];
    char gpsFilename[MAXPATHLEN];
    char ppsFilename[MAXPATHLEN];
    struct timeval timebuffer;
    struct tm tm_struct;
    char errorMsg[MAXPATHLEN+60];

	/* determine time string */

    (void)gettimeofday(&timebuffer, NULL);
    (void)gmtime_r(&timebuffer.tv_sec, &tm_struct);
    (void)strftime(timeString, sizeof(timeString), "%Y%m%dt%H%M%S", &tm_struct);
    m_currentRecording.startTime = timebuffer.tv_sec;

	/* construct base name */

    strcpy(base, appFrame::dailyDir);
    strcat(base, "/");
    strcat(base, m_block->currentPrefix());
    strcat(base, timeString);

	/* determine filenames for imagery and header */

    sprintf(imageFilename, "%s_raw", base);
    strcpy(m_currentRecording.imagefile, m_block->currentPrefix());
    strcat(m_currentRecording.imagefile, timeString);
    strcat(m_currentRecording.imagefile, "_raw");
    strcpy(m_currentRecording.imagepath, imageFilename);

    sprintf(imageHdrFilename, "%s_raw.hdr", base);
    strcpy(m_currentRecording.imagehdrfile, m_currentRecording.imagefile);
    strcat(m_currentRecording.imagehdrfile, ".hdr");
    strcpy(m_currentRecording.imagehdrpath, imageHdrFilename);

	/* determine filename for GPS data */

    sprintf(gpsFilename, "%s_gps", base);
    strcpy(m_currentRecording.gpsfile, m_block->currentPrefix());
    strcat(m_currentRecording.gpsfile, timeString);
    strcat(m_currentRecording.gpsfile, "_gps");
    strcpy(m_currentRecording.gpspath, gpsFilename);

	/* determine filename for PPS data */

    sprintf(ppsFilename, "%s_pps", base);
    strcpy(m_currentRecording.ppsfile, m_block->currentPrefix());
    strcat(m_currentRecording.ppsfile, timeString);
    strcat(m_currentRecording.ppsfile, "_pps");
    strcpy(m_currentRecording.ppspath, ppsFilename);

	/* open files */

    m_imageFP = fopen(imageFilename, "wb");
    if (!m_imageFP) {
	sprintf(errorMsg, "Can't open output file \"%s\".", imageFilename);
	error(errorMsg);
	return -1;
    }
    m_imageHdrFP = fopen(imageHdrFilename, "w");
    if (!m_imageHdrFP) {
	sprintf(errorMsg, "Can't open output file \"%s\".", imageHdrFilename);
	error(errorMsg);
	fclose(m_imageFP);
	m_imageFP = NULL;
	return -1;
    }
    m_gpsFP = fopen(gpsFilename, "wb");
    if (!m_gpsFP) {
	sprintf(errorMsg, "Can't open output file \"%s\".", gpsFilename);
	error(errorMsg);
	fclose(m_imageFP);
	fclose(m_imageHdrFP);
	m_imageFP = NULL;
	m_imageHdrFP = NULL;
	return -1;
    }
    m_ppsFP = fopen(ppsFilename, "wb");
    if (!m_ppsFP) {
	sprintf(errorMsg, "Can't open output file \"%s\".", ppsFilename);
	error(errorMsg);
	fclose(m_imageFP);
	fclose(m_imageHdrFP);
	fclose(m_gpsFP);
	m_imageFP = NULL;
	m_imageHdrFP = NULL;
	m_gpsFP = NULL;
	return -1;
    }
    return 0;
}


void DisplayTab::closeFiles(struct timespec *closeTime_p)
{
    struct stat statbuf;

    if (closeTime_p)
	closeTime_p->tv_sec = closeTime_p->tv_nsec = 0;

    if (m_imageFP != NULL) {
	if (closeTime_p != NULL && fstat(fileno(m_imageFP), &statbuf) != -1)
	    (void)memcpy(&closeTime_p->tv_sec, &statbuf.st_mtime,
		sizeof(time_t));
	fclose(m_imageFP);
	m_imageFP = NULL;
    }

    if (m_imageHdrFP != NULL) {
	fprintf(m_imageHdrFP, "ENVI\n");
	fprintf(m_imageHdrFP, "description= {}\n");
	fprintf(m_imageHdrFP, "samples= %d\n", m_frameWidthSamples);
	fprintf(m_imageHdrFP, "lines= %d\n", m_framesWritten);
	fprintf(m_imageHdrFP, "bands= %d\n", m_frameHeightLines);
	fprintf(m_imageHdrFP, "header offset= 0\n");
	fprintf(m_imageHdrFP, "file type= ENVI\n");
	fprintf(m_imageHdrFP, "data type= 12\n");
	fprintf(m_imageHdrFP, "interleave= bil\n");
	fprintf(m_imageHdrFP, "byte order= 0\n");
	fclose(m_imageHdrFP);
	m_imageHdrFP = NULL;
    }

    if (m_gpsFP != NULL) {
	fclose(m_gpsFP);
	m_gpsFP = NULL;
    }
    if (m_ppsFP != NULL) {
	fclose(m_ppsFP);
	m_ppsFP = NULL;
    }
}


bool DisplayTab::performResourcesCheck(void)
{
    updateResourcesDisplay();
    return true; /* i.e., keep going */
}


#define OPEN \
    if (m_app->openMountConnection(&alreadyOpen, 1) == -1) { \
	m_mountCheckColor = StatusDisplay::COLOR_RED; \
	m_mountHandlerState = MOUNTHANDLER_IDLE; break; }
#define SEND(s) { \
    if (m_app->sendMountCmd(s, 1) == -1) { \
	m_mountCheckColor = StatusDisplay::COLOR_RED; \
	m_mountHandlerState = MOUNTHANDLER_IDLE; break; } \
    m_mountCmdIterations = 0; }
#define GET_AND_CHECK_RESPONSE(c) \
    if (m_app->getMountResponse(response, sizeof(response), \
	    &responseSize, 1) == -1) { \
	m_mountCmdIterations++; \
	if (m_mountCmdIterations == 200) { \
	    if (++m_mountCmdTries == 3) { \
		m_mountCheckColor = StatusDisplay::COLOR_RED; \
		m_mountHandlerState = MOUNTHANDLER_IDLE; } \
	    else SEND(c) } \
	break; }
#define CHECK_FOR_TIMEOUT(s) \
    m_mountCmdIterations++; \
    if (m_mountCmdIterations > 100) { error(s " timed out."); \
	m_mountCheckColor = StatusDisplay::COLOR_RED; \
	m_mountHandlerState = MOUNTHANDLER_IDLE; break; }

bool DisplayTab::handleMount(void)
{
    char response[20];
    int responseSize;
    int alreadyOpen;

    m_mountHandlerMutex.lock();
    switch (m_mountHandlerState) {
	case MOUNTHANDLER_IDLE:
	    break;
	case MOUNTHANDLER_S0_MAKE_CONNECTION:
	    OPEN
	    m_mountHandlerState = MOUNTHANDLER_SEND_S0;
	    break;
	case MOUNTHANDLER_SEND_S0:
	    SEND("S0")
	    m_mountCmdTries = 0;
	    m_mountHandlerState = MOUNTHANDLER_GET_S0_REPLY;
	    break;
	case MOUNTHANDLER_GET_S0_REPLY:
	    GET_AND_CHECK_RESPONSE("S0")
	    m_mountCheckColor = StatusDisplay::COLOR_GREEN;
	    m_mountHandlerState = MOUNTHANDLER_IDLE;
	    break;
	case MOUNTHANDLER_S1_MAKE_CONNECTION:
	    OPEN
	    m_mountHandlerState = MOUNTHANDLER_SEND_S1;
	    break;
	case MOUNTHANDLER_SEND_S1:
	    SEND("S1")
	    m_mountCmdTries = 0;
	    m_mountHandlerState = MOUNTHANDLER_GET_S1_REPLY;
	    break;
	case MOUNTHANDLER_GET_S1_REPLY:
	    GET_AND_CHECK_RESPONSE("S1")
	    m_mountHandlerState = MOUNTHANDLER_SEND_S;
	    break;
	case MOUNTHANDLER_SEND_S:
	    SEND("s")
	    m_mountCmdTries = 0;
	    m_mountHandlerState = MOUNTHANDLER_GET_S_REPLY;
	    break;
	case MOUNTHANDLER_GET_S_REPLY:
	    GET_AND_CHECK_RESPONSE("s")
	    if (responseSize == 2 && strncmp(response, "+1", 2) == 0) {
		m_mountCheckColor = StatusDisplay::COLOR_GREEN;
		m_mountHandlerState = MOUNTHANDLER_IDLE;
	    }
	    else m_mountHandlerState = MOUNTHANDLER_SEND_S;
	    break;
	default:
	    error("Internal error in DisplayTab::handleMount");
	    m_mountHandlerState = MOUNTHANDLER_IDLE;
	    break;
    }
    m_mountHandlerMutex.unlock();
    return true; /* i.e., keep going */
}


int DisplayTab::setMount(int newState)
{
	/* set state machine up for required mode */

    m_mountHandlerMutex.lock();
    switch (newState) {
        case MOUNT_IDLE:
	    m_mountHandlerState = MOUNTHANDLER_S0_MAKE_CONNECTION;
            break;
        case MOUNT_ACTIVE:
	    m_mountHandlerState = MOUNTHANDLER_S1_MAKE_CONNECTION;
            break;
        default:
	    error("Internal error in DisplayTab::setMount");
            break;
    }
    m_mountHandlerMutex.unlock();
    return 0;
}


void DisplayTab::fmsControlChanged(void)
{
	/* if we're not using FMS at all, close the device if enabled,
 	   note that FMS is unavailable for use, gray out FMS status, and
	   enable record button */
	
    if (m_block->currentFMSControlOption() == FMSCONTROL_NO) {
	if (m_recordToggleAttached)
            detachRecordToggleSwitch();
	m_recordToggleAttached = false;
	m_lineHigh = false;
        if (!appFrame::headless)
	    m_fmsCheckLabel.set_sensitive(false);
        m_fmsCheckColor = StatusDisplay::COLOR_UNUSED;
	enableRecordButton();
    }

	/* otherwise, if we want FMS but can't open DIO device, make FMS
 	   unavailable for use, note problem in FMS indicator, but don't let
	   user record manually since they're still not allowed to */

    else if (!attachRecordToggleSwitch()) {
	m_recordToggleAttached = false;
	m_lineHigh = false;
        if (!appFrame::headless)
	    m_fmsCheckLabel.set_sensitive(true);
        m_fmsCheckColor = StatusDisplay::COLOR_RED;
	disableRecordButton();
    }

	/* otherwise, note that FMS is available for use, get the current
 	   condition, make FMS indicator yellow (since we don't know the FMS
 	   state yet), and disable record button to prevent user recording */

    else {
	m_recordToggleAttached = true;
	m_lineHigh = lineIsHigh();
        if (!appFrame::headless)
	    m_fmsCheckLabel.set_sensitive(true);
        m_fmsCheckColor = StatusDisplay::COLOR_YELLOW;
	disableRecordButton();
    }
}


void DisplayTab::calcFreeSpace(const struct statfs *sb_p, double *usableSize_p,
    double *usableFree_p) const
{
    double totalSize, marginBytes;

    totalSize = sb_p->f_blocks * sb_p->f_bsize;
    marginBytes = m_block->currentRecMarginPct() / 100.0 * totalSize;
    *usableSize_p = totalSize - marginBytes;
    *usableFree_p = sb_p->f_bavail * sb_p->f_bsize - marginBytes;
    if (*usableFree_p < 0.0)
	*usableFree_p = 0.0;
}


void DisplayTab::updateResourcesDisplay(void)
{
    int timeLeft, hrs, mins, i;
    char buf[50];
    enum { SPACE_OKAY, SPACE_WARNING, SPACE_ERROR } status;
    double imageBytesPerSecond, gpsBytesPerSecond, ppsBytesPerSecond,
	allgpsBytesPerSecond, allppsBytesPerSecond, logBytesPerSecond, pct;
    struct statfs dailyDirStat;
    static int displayBroken = 0;
    GdkColor red, green;
    fs_descr_t fsDescrs[4];
    int numFS;

	/* calculate recording rates */

    imageBytesPerSecond = appFrame::fb->getFrameRateHz() *
	m_frameHeightLines * m_frameWidthSamples * sizeof(short);
    gpsBytesPerSecond = (double)GPS_WORDS_PER_SECOND * sizeof(short);
    ppsBytesPerSecond = (double)PPS_WORDS_PER_SECOND * sizeof(short);
    allgpsBytesPerSecond = (double)GPS_WORDS_PER_SECOND * sizeof(short);
    allppsBytesPerSecond = (double)PPS_WORDS_PER_SECOND * sizeof(short);
    logBytesPerSecond = 0; /* placeholder */

	/* if resources can't be measured for some reason, do nothing.  this
	   should never happen */

    if (displayBroken) ;

	/* otherwise, if we can't poll the filesystem for space available,
	   note that the display is broken and disable the resources bar */

    else if (statfs(appFrame::dailyDir, &dailyDirStat) == -1) {
	warn("Can't poll filesystem statistics.",
	    "Disabling resources display.");
	displayBroken = 1;
	if (!appFrame::headless) {
	    m_spaceAvailableLabel.set_sensitive(false);
	    m_spaceAvailableBar.set_sensitive(false);
	    m_timeLeftLabel.set_sensitive(false);
	    m_spaceAvailableBar.set_fraction(0.0);
	}
    }

	/* otherwise... */

    else {
	    /* build up filesystem list */

	numFS = 0;
	(void)memset(fsDescrs, 0, sizeof(fsDescrs));
	buildUpFilesystemList(fsDescrs, &numFS, &dailyDirStat,
	    imageBytesPerSecond + gpsBytesPerSecond + ppsBytesPerSecond +
		allgpsBytesPerSecond + allppsBytesPerSecond +
		logBytesPerSecond);

	    /* figure out how much time we have left on each filesystem */

	for (i=0;i < numFS;i++) {
	    fsDescrs[i].timeLeft = fsDescrs[i].usableFree /
		fsDescrs[i].bytesPerSecond;
	    fsDescrs[i].maxTimePossible = fsDescrs[i].usableSize /
		fsDescrs[i].bytesPerSecond;
	}

	    /* time left is the minimum across all filesystems.  pct is the
	       amount free / total amount possible */

	timeLeft = fsDescrs[0].timeLeft;
	pct = fsDescrs[0].usableFree / fsDescrs[0].usableSize;
	for (i=1;i < numFS;i++)
	    if (fsDescrs[i].timeLeft < timeLeft) {
		timeLeft = fsDescrs[i].timeLeft;
		pct = fsDescrs[i].usableFree / fsDescrs[i].usableSize;
	    }

            /* update time-left status on GUI */

        hrs = timeLeft / SECONDS_PER_HOUR;
        mins = (timeLeft % SECONDS_PER_HOUR) / SECONDS_PER_MINUTE;
        if (hrs != 0) {
            if (hrs == 1) {
                if (mins == 1)
                    sprintf(buf, "Time left: 1 hour, 1 minute");
                else sprintf(buf, "Time left: 1 hour, %d minutes", mins);
            }
            else {
                if (mins == 1)
                    sprintf(buf, "Time left: %d hours, 1 minute", hrs);
                else sprintf(buf, "Time left: %d hours, %d minutes", hrs, mins);
            }
        }
        else {
            if (mins == 1)
                sprintf(buf, "Time left: 1 minute");
            else sprintf(buf, "Time left: %d minutes", mins);
        }
	if (!appFrame::headless)
	    m_timeLeftLabel.set_text(buf);

            /* determine if we're okay on space */

        status = (hrs == 0 && mins == 0? SPACE_ERROR:
            hrs == 0 && mins < 20? SPACE_WARNING:SPACE_OKAY);

            /* update space available bars */

        if (status == SPACE_OKAY) {
	    if (!appFrame::headless) {
		(void)gdk_color_parse("green", &green);
		GtkWidget *w = (GtkWidget *)m_spaceAvailableBar.gobj(); /*lint !e740 cast okay */
		gtk_widget_modify_bg(w, GTK_STATE_SELECTED, &green);
		m_spaceAvailableBar.set_fraction(pct);
	    }
	    else if (m_headlessDevFd != -1)
		writeSerialStringForColor(StatusDisplay::COLOR_GREEN, "RS");
	    m_outOfSpace = 0;
	}
        else if (status == SPACE_WARNING) {
	    if (!appFrame::headless) {
		(void)gdk_color_parse("red", &red);
		GtkWidget *w = (GtkWidget *)m_spaceAvailableBar.gobj(); /*lint !e740 cast okay */
		gtk_widget_modify_bg(w, GTK_STATE_SELECTED, &red);
		m_spaceAvailableBar.set_fraction(pct);
	    }
	    else if (m_headlessDevFd != -1)
		writeSerialStringForColor(StatusDisplay::COLOR_YELLOW, "RS");
	    m_outOfSpace = 0;
	}
        else /* status == SPACE_ERROR */ {
	    if (!appFrame::headless) {
		(void)gdk_color_parse("red", &red);
		GtkWidget *w = (GtkWidget *)m_spaceAvailableBar.gobj(); /*lint !e740 cast okay */
		gtk_widget_modify_bg(w, GTK_STATE_SELECTED, &red);
		m_spaceAvailableBar.set_fraction(1.0);
	    }
	    else if (m_headlessDevFd != -1)
		writeSerialStringForColor(StatusDisplay::COLOR_RED, "RS");
	    m_outOfSpace = 1;
	}
    }
}


void DisplayTab::buildUpFilesystemList(DisplayTab::fs_descr_t *fsDescrs,
    int *numFS_p, struct statfs *statbuf_p, double bytesPerSecond)
{
    int i;
    double usableSize, usableFree;

    calcFreeSpace(statbuf_p, &usableSize, &usableFree);
    for (i=0;i < *numFS_p;i++)
	if (memcmp(&statbuf_p->f_fsid, fsDescrs[i].fsid, sizeof(fsid_t)) == 0)
	    break;
    if (i == *numFS_p) {
	fsDescrs[*numFS_p].usableSize = usableSize;
	fsDescrs[*numFS_p].usableFree = usableFree;
	fsDescrs[*numFS_p].bytesPerSecond = bytesPerSecond;
	fsDescrs[*numFS_p].fsid = &statbuf_p->f_fsid;
	(*numFS_p)++;
    }
    else fsDescrs[i].bytesPerSecond += bytesPerSecond;
}


void DisplayTab::onShutterCombo(void)
{
    int selection;
    static int first_time = 1;
    char error_msg[MAX_ERROR_LEN];
    static int ignore_change = 0;
    char logmsg[200];

	/* if forced change, ignore it */

    if (ignore_change) {
	ignore_change = 0;
	return;
    }

	/* determine selected position */

    m_app->comboEntryToIndex(m_shutterCombo,
	shutterOptions, &selection, "shutter setting");

	/* log action */

    sprintf(logmsg, "User changed shutter setting to %s.",
	shutterOptions[selection]);
    log(logmsg);

        /* if first time, init device */

    if (first_time) {
	log("Initializing shutter mechanism...");
	Gtk::MessageDialog d("Press OK to start.  This may take a minute...",
	    false /* no markup */, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK);
	(void)d.run();
        if (Maxon_init_device(error_msg) == -1) {
	    log("Shutter device init failed.");
	    warn(error_msg);
            (void)Maxon_close_device(NULL);
	    ignore_change = 1;
	    m_shutterCombo->set_active_text(shutterOptions[0]);
            return;
        }
        if (Maxon_home(error_msg) == -1) {
	    log("Shutter homing failed.");
	    warn(error_msg);
            (void)Maxon_close_device(NULL);
	    ignore_change = 1;
	    m_shutterCombo->set_active_text(shutterOptions[0]);
            return;
        }
        first_time = 0;
	log("Initialization was successful.");
    }

        /* open or close as appropriate */

    if (selection == SHUTTER_POS_UNKNOWN) {
	first_time = 1;
	(void)Maxon_close_device(NULL);
	m_shutterCombo->set_active_text(shutterOptions[0]);
    }
    else if (selection == SHUTTER_POS_CLOSED) {
        if (Maxon_position(SHUTTER_CLOSED_POS, error_msg) == -1) {
            first_time = 1;
	    warn(error_msg);
            (void)Maxon_close_device(NULL);
	    ignore_change = 1;
	    m_shutterCombo->set_active_text(shutterOptions[0]);
        }
    }
    else /* selection == SHUTTER_POS_OPEN */ {
        if (Maxon_position(SHUTTER_OPEN_POS, error_msg) == -1) {
            first_time = 1;
	    warn(error_msg);
            (void)Maxon_close_device(NULL);
	    ignore_change = 1;
	    m_shutterCombo->set_active_text(shutterOptions[0]);
        }
    }
}


void DisplayTab::onOBCControlCombo(void)
{
    int selection;
    char logmsg[200];

	/* if the program isn't forcing a change to OBC Auto, that is, if this
 	   change is from the user... */

    if (!m_obcControlComboForced) {

	    /* get the new mode and log it */

	m_app->comboEntryToIndex(m_obcControlCombo,
	    obcControlOptions, &selection, "OBC setting");
	sprintf(logmsg, "User changed OBC setting to %s.",
	    obcControlOptions[selection]);
	log(logmsg);

	    /* make the change to the OBC */

	if (selection == OBC_AUTO) {
	    switch (m_recordState) {
		case RECORD_STATE_NOT_RECORDING:
		    m_app->setOBC(OBC_SCIENCE);
		    break;
		case RECORD_STATE_DARK1CAL:
		    m_app->setOBC(OBC_DARK1);
		    break;
		case RECORD_STATE_SCIENCE:
		    m_app->setOBC(OBC_SCIENCE);
		    break;
		case RECORD_STATE_DARK2CAL:
		    m_app->setOBC(OBC_DARK2);
		    break;
		case RECORD_STATE_MEDIUMCAL:
		    m_app->setOBC(OBC_MEDIUM);
		    break;
		case RECORD_STATE_BRIGHTCAL:
		    m_app->setOBC(OBC_BRIGHT);
		    break;
		case RECORD_STATE_LASERCAL:
		    m_app->setOBC(OBC_LASER);
		    break;
		default:
		    error("Internal error in DisplayTab::onOBCControlCombo");
		    break;
	    }
	}
	else m_app->setOBC(obcControlCodes[selection]);

	    /* if we're using a dark cal mode, prepare to acquire dark frames
 	       which we can save for later subtraction.  wait for 100 ms worth
	       of frames before starting accumulation */

	if (obcControlCodes[selection] == OBC_DARK1) {
	    m_framesUntilDark = appFrame::fb->getFrameRateHz() *
		OBC_SETTLING_TIME_SECS;
	    memset(m_darkFrameAccumulation, 0,
		m_frameWidthSamples * m_frameHeightLines * sizeof(int));
	    m_darkFramesAccumulated = 0;
	}

	    /* note the current OBC mode */

	m_mostRecentOBCMode = selection;
    }

	/* otherwise, this was a forced transition to OBC Auto: reset the flag
 	   so that future changes are recognized as coming from the user */

    else m_obcControlComboForced = false;
}


void DisplayTab::onMountControlCombo(void)
{
    int selection;
    char logmsg[200];

	/* if the program isn't forcing a change to Auto, that is, if this
 	   change is from the user... */

    if (!m_mountControlComboForced) {

	    /* get the new mode and log it */

	m_app->comboEntryToIndex(m_mountControlCombo,
	    mountControlOptions, &selection, "Mount mode");
	sprintf(logmsg, "User changed mount mode to %s.",
	    mountControlOptions[selection]);
	log(logmsg);

	    /* make the change to the mount */

	if (selection == MOUNT_AUTO) {
	    if (m_recordState == RECORD_STATE_NOT_RECORDING)
		(void)setMount(MOUNT_IDLE);
	    else (void)setMount(MOUNT_ACTIVE);
	    m_mostRecentMountMode = selection;
	}
	else if (setMount(mountControlCodes[selection]) == -1) {
	    warn("Changing mount mode back.");
	    m_mountControlComboForced = true;
	    m_mountControlCombo->set_active_text(mountControlOptions[
		m_mostRecentMountMode]);
	}
	else m_mostRecentMountMode = selection;
    }

	/* otherwise, this was a forced transition: reset the flag
 	   so that future changes are recognized as coming from the user */

    else m_mountControlComboForced = false;
}


void DisplayTab::stopThreadsAndTimers(void)
{
    m_dataThreadEnabled = 0;
    if (m_dataThread)
	m_dataThread->join();

    m_resourcesCheckTimer.disconnect();
    m_mountHandler.disconnect();
}


void SliderDialog::onRedBandSliderChange(void)
{
    double value = m_redBandSlider.get_value();

    if (m_parent->m_settingsTab)
	m_parent->m_settingsTab->updateRedBandSelection((int)value, false);
    m_redBandSliderChanged = true;
}


void SliderDialog::onGreenBandSliderChange(void)
{
    double value = m_greenBandSlider.get_value();

    if (m_parent->m_settingsTab)
	m_parent->m_settingsTab->updateGreenBandSelection((int)value, false);
    m_greenBandSliderChanged = true;
}


void SliderDialog::onBlueBandSliderChange(void)
{
    double value = m_blueBandSlider.get_value();

    if (m_parent->m_settingsTab)
	m_parent->m_settingsTab->updateBlueBandSelection((int)value, false);
    m_blueBandSliderChanged = true;
}


void SliderDialog::onDowntrackSkipSliderChange(void)
{
    double value = m_downtrackSkipSlider.get_value();

    if (m_parent->m_settingsTab)
	m_parent->m_settingsTab->updateDowntrackSkipSelection((int)value,
	    false);
    m_downtrackSkipSliderChanged = true;
}


void SliderDialog::onStretchMinSliderChange(void)
{
    double min = m_stretchMinSlider.get_value();

    if (m_parent->m_settingsTab)
	m_parent->m_settingsTab->updateStretchMinSelection((int)min, false);
    m_parent->updateStretch();
    m_stretchMinSliderChanged = true;
}


void SliderDialog::onStretchMaxSliderChange(void)
{
    double max = m_stretchMaxSlider.get_value();

    if (m_parent->m_settingsTab)
	m_parent->m_settingsTab->updateStretchMaxSelection((int)max, false);
    m_parent->updateStretch();
    m_stretchMaxSliderChanged = true;
}


int DisplayTab::log(const char *msg)
{
    struct timeval timebuffer;
    struct tm tm_struct;
    char timestring[60];

    if (m_logFP) {
	(void)gettimeofday(&timebuffer, NULL);
	(void)gmtime_r(&timebuffer.tv_sec, &tm_struct);
	(void)strftime(timestring, sizeof(timestring), "%H:%M:%S", &tm_struct);
	Glib::RecMutex::Lock lock(m_logMutex);
	fprintf(m_logFP, "%s: %s\n", timestring, msg);
	fflush(m_logFP);
	return 0;
    }
    else return -1;
}


void DisplayTab::info(Glib::ustring msg)
{
    if (!appFrame::headless) {
	sigc::slot<void> slot = sigc::bind(
	    sigc::mem_fun(this, &DisplayTab::showMsg),
	    msg, "", Gtk::MESSAGE_INFO, "info");
	m_popupMsgDispatcher.send(slot);
    }
    else {
	fprintf(stderr, "%s\n", msg.c_str());
	log(msg.c_str());
    }
}


void DisplayTab::warn(Glib::ustring msg)
{
    if (!appFrame::headless) {
	sigc::slot<void> slot = sigc::bind(
	    sigc::mem_fun(this, &DisplayTab::showMsg),
	    msg, "", Gtk::MESSAGE_WARNING, "warning");
	m_popupMsgDispatcher.send(slot);
    }
    else {
	fprintf(stderr, "%s\n", msg.c_str());
	log(msg.c_str());
    }
}


void DisplayTab::error(Glib::ustring msg)
{
    if (!appFrame::headless) {
	sigc::slot<void> slot = sigc::bind(
	    sigc::mem_fun(this, &DisplayTab::showMsg),
	    msg, "", Gtk::MESSAGE_ERROR, "error");
	m_popupMsgDispatcher.send(slot);
    }
    else {
	fprintf(stderr, "%s\n", msg.c_str());
	log(msg.c_str());
    }
}


void DisplayTab::info(Glib::ustring msg, Glib::ustring secondaryMsg)
{
    if (!appFrame::headless) {
	sigc::slot<void> slot = sigc::bind(
	    sigc::mem_fun(this, &DisplayTab::showMsg),
	    msg, secondaryMsg, Gtk::MESSAGE_INFO, "info");
	m_popupMsgDispatcher.send(slot);
    }
    else {
	fprintf(stderr, "%s\n", msg.c_str());
	log(msg.c_str());
    }
}


void DisplayTab::warn(Glib::ustring msg, Glib::ustring secondaryMsg)
{
    if (!appFrame::headless) {
	sigc::slot<void> slot = sigc::bind(
	    sigc::mem_fun(this, &DisplayTab::showMsg),
	    msg, secondaryMsg, Gtk::MESSAGE_WARNING, "warning");
	m_popupMsgDispatcher.send(slot);
    }
    else {
	fprintf(stderr, "%s\n", msg.c_str());
	log(msg.c_str());
    }
}


void DisplayTab::error(Glib::ustring msg, Glib::ustring secondaryMsg)
{
    if (!appFrame::headless) {
	sigc::slot<void> slot = sigc::bind(
	    sigc::mem_fun(this, &DisplayTab::showMsg),
	    msg, secondaryMsg, Gtk::MESSAGE_ERROR, "error");
	m_popupMsgDispatcher.send(slot);
    }
    else {
	fprintf(stderr, "%s\n", msg.c_str());
	log(msg.c_str());
    }
}


void DisplayTab::showMsg(Glib::ustring msg, Glib::ustring secondaryMsg,
    Gtk::MessageType type, Glib::ustring description)
{
    char *fullMsg;

    Gtk::MessageDialog d(msg, false /* no markup */, type, Gtk::BUTTONS_OK);
    if (!secondaryMsg.empty())
	d.set_secondary_text(secondaryMsg);
    (void)d.run();

    fullMsg = new char[description.size()+msg.size()+20];
    (void)sprintf(fullMsg, "Issued %s msg \"%s\"", description.c_str(),
	msg.c_str());
    log(fullMsg);
    delete[] fullMsg;

    if (!secondaryMsg.empty()) {
	fullMsg = new char[description.size()+secondaryMsg.size()+20];
	(void)sprintf(fullMsg, "Issued %s msg \"%s\"", description.c_str(),
	    secondaryMsg.c_str());
	log(fullMsg);
	delete[] fullMsg;
    }
}


void DisplayTab::add3501(u_short *msg)
{
    u_short *usp;
    int itemp;
    double pitchAndRoll[2];

	/* extract and plot lat and lon */

    usp = msg + 9;
    memcpy(&itemp, usp, sizeof(int));
    m_lastLat = itemp / pow(2.0, 31.0) * 180;

    usp += 2;
    memcpy(&itemp, usp, sizeof(int));
    m_lastLon = itemp / pow(2.0, 31.0) * 180;

    if (!appFrame::headless)
	m_plotsTab->addLatAndLon(m_lastLat, m_lastLon);

	/* extract and plot altitude */

    usp = msg + 13;
    memcpy(&itemp, usp, sizeof(int));
    m_lastAltitude = itemp / pow(2.0, 16.0); /* convert from fixed point */
    m_lastAltitude = m_lastAltitude * 39.3701 / 12; /* m to ft */

    if (!appFrame::headless)
	m_plotsTab->addAltitude(m_lastAltitude);

	/* extract and plot velocity */

    usp = msg + 15;
    memcpy(&itemp, usp, sizeof(int));
    m_lastVelNorth = itemp / pow(2.0, 21.0) / 0.514444; /* convert to knots */

    usp += 2;
    memcpy(&itemp, usp, sizeof(int));
    m_lastVelEast = itemp / pow(2.0, 21.0) / 0.514444; /* convert to knots */

    usp += 2;
    memcpy(&itemp, usp, sizeof(int));
    m_lastVelUp = itemp / pow(2.0, 21.0) / 0.514444; /* convert to knots */

    m_lastVelMag = sqrt(m_lastVelNorth * m_lastVelNorth +
	m_lastVelEast * m_lastVelEast + m_lastVelUp * m_lastVelUp);
    if (!appFrame::headless)
	m_plotsTab->addVelocity(m_lastVelMag);

	/* extract and plot pitch, roll, and heading */

    usp = msg + 21;
    memcpy(&itemp, usp, sizeof(int));
    pitchAndRoll[0] = itemp / pow(2.0, 31.0) * 180;

    usp += 2;
    memcpy(&itemp, usp, sizeof(int));
    pitchAndRoll[1] = itemp / pow(2.0, 31.0) * 180;

    usp += 2;
    memcpy(&itemp, usp, sizeof(int));
    m_lastHeading = itemp / pow(2.0, 31.0) * 180;

    if (!appFrame::headless) {
	m_pitchAndRollPlots->addPoint(pitchAndRoll);
	m_headingPlot->addPoint(&m_lastHeading);
    }
}


void DisplayTab::draw(void)
{
    m_pitchAndRollPlots->draw();
    m_headingPlot->draw();

    m_plotsTab->draw();
}


void DisplayTab::resetNavPlots(void)
{
    m_pitchAndRollPlots->reset();
    m_headingPlot->reset();

    m_plotsTab->resetNavPlots();
}


//  Attempts to attach to ACCES IO 32 device
bool DisplayTab::attachRecordToggleSwitch()
{
   char msg[500];
   static int errorsSuppressed = 0;

   unsigned long result = AIOUSB_Init();

   if(result == AIOUSB_SUCCESS)
   {
      unsigned long deviceMask = GetDevices();
      if(deviceMask != 0)
      {
         //AIOUSB_ListDevices();  //  Print list of devices found on the bus
      }

      int devicesFound = 0;
      int index = 0;
      DIODeviceInfo_t *device;

      while(deviceMask !=0 && devicesFound < DIO_DEVICES_REQUIRED)
      {
         if((deviceMask & 1) != 0)
         {
            device = &m_deviceTable[devicesFound];
            device->nameSize = DIO_MAX_NAME_SIZE;

	    //printf("Looking for device mask %08lx\n",deviceMask);
            result = QueryDeviceInfo(index, &device->productID,
	       &device->nameSize, device->name, &device->numDIOBytes,
	       &device->numCounters);

            if(result == AIOUSB_SUCCESS)
            {
               if(device->productID == USB_DIO_32)
               {
                  device->index = index;
                  devicesFound++;
		  //printf("Found a USB_DIO_32 device\n");
               }
               else
               {
                  if (!errorsSuppressed)
		     warn("Warning: Unexpectedly found something other than a "
		        "USB_DIO_32 device.");
		  errorsSuppressed = 1;
               }
            }
            else
            {
               sprintf(msg, "Error '%s' querying device at index %d.",
		  AIOUSB_GetResultCodeAsString(result), index);
	       if (!errorsSuppressed)
		  warn(msg);
	       errorsSuppressed = 1;
            }
         }
         index++;
         deviceMask >>= 1;
      }  // end while

      // printf("\ndevices found = %d\n", devicesFound);
      // printf("\nindex = %d\n", index);
      if (devicesFound == 0)
      {
	 if (!errorsSuppressed)
            warn("Error: Can't find AccessIO USB_DIO_32 device for FMS "
	       "control.");
         errorsSuppressed = 1;

         AIOUSB_Exit();
	 return(false);
      }
      if((devicesFound > DIO_DEVICES_REQUIRED) || (index != 1))
      {
	 if (!errorsSuppressed)
            warn("Error: More than one AccessIO USB_DIO_32 device not "
	       "supported.");
	 errorsSuppressed = 1;

         AIOUSB_Exit();
         return(false);
      }

      //  We assume single device.  In addition, we assume toggle is on port A
      index = 0;
      device = &m_deviceTable[index];

      //  Print out information message indicating AccesIO device serial number
      result = GetDeviceSerialNumber(device->index, &device->serialNumber);
      if (result != AIOUSB_SUCCESS) {
         sprintf(msg, "Error '%s' getting serial number of device at index %d.",
                     AIOUSB_GetResultCodeAsString(result), device->index);
	 if (!errorsSuppressed)
	    warn(msg);
	 errorsSuppressed = 1;
      }

      //  Demonstrate DIO configuration
      device = &m_deviceTable[index];     // select first/only device
      //  AIOUSB_SetCommTimeout(device->index, 1000); // set timeout for USB ops
      AIOUSB_SetCommTimeout(device->index, 1); // set timeout for USB ops

      //  Set all ports to input mode.  Really only applies to right-most 4
      //  bytes (4 ports)
      memset(device->outputMask, 0x00, DIO_MASK_BYTES);
      result = DIO_Configure(device->index, AIOUSB_FALSE, device->outputMask,
	 device->writeBuffer);

      //  If we can't configure it, close it down and indicate failure
      if (result == AIOUSB_SUCCESS)
      {
         //printf("Dev at index %d successfully configured\n", device->index);
         return(true);
      }
      else
      {
         sprintf(msg, "Error '%s' configuring device at index %d.",
	    AIOUSB_GetResultCodeAsString(result), device->index);
	 if (!errorsSuppressed)
	    warn(msg);
	 errorsSuppressed = 1;
         AIOUSB_Exit();
         return(false);
      }

   }

   return(false);
}

bool DisplayTab::detachRecordToggleSwitch()
{
   AIOUSB_Exit();

   return(true);  //  Always return true
}

void DisplayTab::testDigitalLineTransition(void)
{
   //  If line goes high, click the record button 
   if(lineIsHigh() && (m_lineHigh == false))
   {
      switch (m_recordState) {
	 case RECORD_STATE_NOT_RECORDING:
            m_lineHigh = true;
            m_pressRecordButton();
	    break;
	 case RECORD_STATE_DARK1CAL:
	 case RECORD_STATE_SCIENCE:
	    m_lineHigh = true; /* shouldn't be possible */
	    break;
	 case RECORD_STATE_DARK2CAL:  /* we must be in the process of */
	 case RECORD_STATE_MEDIUMCAL: /* stopping so come back later  */
	 case RECORD_STATE_BRIGHTCAL:
	 case RECORD_STATE_LASERCAL:
	    break;
	 default: /* can't happen */
	    break;
      }
   }
   //  If line goes low, click the stop button
   else if(!lineIsHigh() && (m_lineHigh == true))
   {
      switch (m_recordState) {
	 case RECORD_STATE_NOT_RECORDING:
	    m_lineHigh = false; /* ignore toggle: user stopped us */
	    break;
	 case RECORD_STATE_DARK1CAL:
	    break; /* come back later to stop in RECORD_STATE_SCIENCE */
	 case RECORD_STATE_SCIENCE:
	    m_lineHigh = false;
	    m_pressStopButton();
	    break;
	 case RECORD_STATE_DARK2CAL:
	 case RECORD_STATE_MEDIUMCAL:
	 case RECORD_STATE_BRIGHTCAL:
	 case RECORD_STATE_LASERCAL:
	    m_lineHigh = false; /* ignore toggle: we're already stopping */
	    break;
	 default: /* can't happen */
	    break;
      }
      return;
   }

   return;
}

inline bool DisplayTab::lineIsHigh(void)
{
   //  We assumed toggle is on port A (or port 0) on the sole USB_DIO_32 device
   int index = 0;
   unsigned port = 0;
   unsigned int result;
   static bool lastResult = false;

   //  Demonstrate DIO read
   DIODeviceInfo_t *device = &m_deviceTable[index];

   //  Read sole ACCES IO DIO 32 device, port A
   result = DIO_Read8(device->index, port, device->readBuffer);
   if (result == AIOUSB_SUCCESS)
   {
      m_fmsCheckColor = StatusDisplay::COLOR_GREEN;
      if(device->readBuffer[port] > 0) {
	 lastResult = true;
         return(true);
      }
      else {
         lastResult = false;
	 return(false);
      }
   }

   //  Error accessing the DIO device.  this could be a disconnection, in
   //  which case we want to stop polling the device and let the user fix it,
   //  or it could be a transient of some kind (e.g., a timeout), in which case
   //  we hope it goes away and don't detach
   m_fmsCheckColor = StatusDisplay::COLOR_RED;
   if (AIOUSB_RESULT_TO_LIBUSB_RESULT(result) == LIBUSB_ERROR_NO_DEVICE) {
      detachRecordToggleSwitch();
      m_recordToggleAttached = false;
      return(false);
   }
   else if (AIOUSB_RESULT_TO_LIBUSB_RESULT(result) == LIBUSB_ERROR_TIMEOUT)
      return lastResult;
   else return(false);
}


void DisplayTab::checkHeadlessInterface(void)
{
    static char headlessCmd[3];
    static int headlessCmdBytes = 0;
    int bytesRead, resync;
    static struct timeval lastTime;
    struct timeval timeNow;

	/* make sure that we don't have more than one previously-received
 	   character buffered, and that we have a legal file descriptor for
	   the serial connection.  these should always be true at this point */

    assert(headlessCmdBytes >= 0 && headlessCmdBytes <= 1);
    assert(m_headlessDevFd != -1);

	/* if we have one byte buffered from previously, try to read one more.
 	   otherwise try to read two, the size of a command */

    bytesRead = appFrame::readPCSerial(m_headlessDevFd,
	headlessCmd+headlessCmdBytes, 2-headlessCmdBytes, 0);

	/* if we had an error, flush our buffer.  this shouldn't happen unless
 	   perhaps the device has become disconnected */

    if (bytesRead < 0)
	headlessCmdBytes = 0;

	/* otherwise, if we didn't read anything and we have one byte already 
 	   buffered, see how long ago we received it, and it's more than 500 ms
	   flush the buffer with a bad-command response.  something must have
	   glitched */

    else if (bytesRead == 0 && headlessCmdBytes == 1) {
	gettimeofday(&timeNow, NULL);
	if (timeNow.tv_sec == lastTime.tv_sec)
	    resync = (timeNow.tv_usec - lastTime.tv_usec >= 500000);
	else if (timeNow.tv_sec == lastTime.tv_sec+1)
	    resync = (timeNow.tv_usec + 1000000 - lastTime.tv_usec >= 500000);
	else resync = 1;
	if (resync) {
	    headlessCmdBytes = 0; /* something's wrong -- resync */
	    (void)appFrame::writePCSerial(m_headlessDevFd, "A!", 2);
	}
    }

	/* otherwise we got at least one character back... */

    else if (bytesRead > 0) {

	    /* make sure we're not confused */

	assert((bytesRead == 1 || bytesRead == 2) &&
	    headlessCmdBytes + bytesRead <= 2);

	    /* update number of bytes buffered */

	headlessCmdBytes += bytesRead;

	    /* if we have two characters, i.e., a full command... */

	if (headlessCmdBytes == 2) {

		/* add a null byte onto the end so we can treat the command
 		   as a string (for convenience) */

	    headlessCmd[2] = '\0';

		/* if command is RECORD, and we're in the right state, start
 		   recording.  provide response */

	    if (strcmp(headlessCmd, "RE") == 0) {
		if (m_recordState == RECORD_STATE_NOT_RECORDING) {
		    (void)appFrame::writePCSerial(m_headlessDevFd, "A+", 2);
		    recordButtonWork(1);
		}
		else (void)appFrame::writePCSerial(m_headlessDevFd, "A!", 2);
	    }

		/* if command is STOP, and we're in the right state, stop
 		   recording.  provide response */

	    else if (strcmp(headlessCmd, "ST") == 0) {
		if (m_recordState == RECORD_STATE_SCIENCE) {
		    (void)appFrame::writePCSerial(m_headlessDevFd, "A+", 2);
		    stopButtonWork(1);
		}
		else (void)appFrame::writePCSerial(m_headlessDevFd, "A!", 2);
	    }

		/* if command is ABORT, and we're in the right state, stop
 		   recording.  provide response */

	    else if (strcmp(headlessCmd, "AB") == 0) {
		if (m_recordState != RECORD_STATE_NOT_RECORDING) {
		    (void)appFrame::writePCSerial(m_headlessDevFd, "A+", 2);
		    abortButtonWork(1);
		}
		else (void)appFrame::writePCSerial(m_headlessDevFd, "A!", 2);
	    }

		/* if command is GPS INIT, and we're in the right state, init
 		   the GPS.  provide response */

	    else if (strcmp(headlessCmd, "II") == 0) {
		(void)appFrame::writePCSerial(m_headlessDevFd, "A+", 2);
		onGPSInitButton();
	    }

		/* otherwise provide failed-command response */

	    else (void)appFrame::writePCSerial(m_headlessDevFd, "A!", 2);

		/* flush buffer and clear out time last character received,
 		   although the latter really isn't necessary */

	    headlessCmdBytes = 0;
	    lastTime.tv_sec = lastTime.tv_usec = 0;
	}

	    /* if we have just a partial command, note the current time so we
 	       can timeout later if necessary */

	else gettimeofday(&lastTime, NULL);
    }
}
