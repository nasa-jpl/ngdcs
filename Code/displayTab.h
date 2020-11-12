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

#include "dispatcher.h"

#define GPS_MSG_3_WORDS	    	82
#define GPS_MSG_3500_WORDS	22
#define GPS_MSG_3501_WORDS	28
#define GPS_MSG_3510_WORDS	27
#define GPS_MSG_3512_WORDS	22
#define GPS_MSG_3623_WORDS      123
#define GPS_MSG_MAX_WORDS	123
#define GPS_WORDS_PER_SECOND    (GPS_MSG_3_WORDS + \
				    GPS_MSG_3500_WORDS + \
				    10 * GPS_MSG_3501_WORDS + \
				    100 * GPS_MSG_3512_WORDS + \
				    GPS_MSG_3623_WORDS)
#define GPS_MAGIC_NUMBER	0x81ff
#define GPS_MSG_3500_CHECKSUM	(~((GPS_MAGIC_NUMBER + 3500 + \
				    (GPS_MSG_3500_WORDS-6) + 0x8000) & \
				    0xffff) + 1)
#define GPS_MSG_3501_CHECKSUM	(~((GPS_MAGIC_NUMBER + 3501 + \
				    (GPS_MSG_3501_WORDS-6) + 0x8000) & \
				    0xffff) + 1)
#define GPS_MSG_3510_CHECKSUM	(~((GPS_MAGIC_NUMBER + 3510 + \
				    (GPS_MSG_3510_WORDS-6) + 0x8000) & \
				    0xffff) + 1)

#define GPS_MODE_FINE_ALIGNMENT	4
#define GPS_MODE_AIR_ALIGNMENT	5
#define GPS_MODE_AIR_NAVIGATION	7

#define PPS_WORDS_PER_SECOND	13 /* from Didier's memo */

#define TIMESTAMP_IMAGE_OFFSET		4
#define OBC_STATE_IMAGE_OFFSET		640
#define PPS_IMAGE_OFFSET		644
#define MAX_PPSDATA_BYTES		40
#define GPS_IMAGE_OFFSET		680
#define MAX_GPSDATA_BYTES		600
#define LOCAL_FRAME_COUNT_IMAGE_OFFSET	1248

#define RECORD_BUTTON_HEIGHT	30
#define RECORD_BUTTON_WIDTH	150
#define STOP_BUTTON_HEIGHT	30
#define STOP_BUTTON_WIDTH	150
#define ABORT_BUTTON_HEIGHT	30
#define ABORT_BUTTON_WIDTH	60

#define NON_COMBO_MARGIN	15

#define OBC_SETTLING_TIME_SECS  0.2
#define DARK_ACCUM_TIME_SECS	0.7

#define DIO_DEVICES_REQUIRED 1  // change this to 1 if only one device
#define DIO_BITS_PER_BYTE 8
#define MAX_DIO_BYTES 4     // a modest little assumption for convenience
#define DIO_MASK_BYTES (( MAX_DIO_BYTES + DIO_BITS_PER_BYTE - 1 ) / DIO_BITS_PER_BYTE )
#define DIO_MAX_NAME_SIZE 20

class StatusDisplay: public Gtk::DrawingArea
{
public:
    typedef enum {
	COLOR_NONE, COLOR_RED, COLOR_GREEN, COLOR_GRAY, COLOR_YELLOW,
	    COLOR_UNUSED
    } color_t;
    StatusDisplay();
    void setColor(color_t color);
    char *color(void);
    virtual ~StatusDisplay() { }
protected:
    color_t m_color;
    virtual bool on_expose_event(GdkEventExpose *event);
};


class ImageDisplay: public Gtk::DrawingArea
{
public:
    ImageDisplay(int fh, int fw, DisplayTab *dt);
    void reset(void);
    void displayFrame(const u_char *image, int height, int width,
        int redBand, int greenBand, int blueBand);
    void displayLine(const u_char *image, u_short width);
    void clearDataSeen() { m_dataSeen = false; }
    ~ImageDisplay();
protected:
    DisplayTab *m_displayTab;
    int m_frameHeightLines;
    int m_frameWidthSamples;
    Cairo::RefPtr<Cairo::ImageSurface> m_surface;
    Cairo::RefPtr<Cairo::Context> m_context;
    Glib::RefPtr<Gdk::Pixbuf> m_pixbuf;
    int m_currentWaterfallHeight;
    bool m_dataSeen;
    int m_redBand, m_greenBand, m_blueBand;
    virtual bool on_expose_event(GdkEventExpose *event);
};


class SliderDialog: public Gtk::Dialog
{
friend class DisplayTab;
protected:
    DisplayTab *m_parent;

    Gtk::Table m_viewerCtlsTable;

    Gtk::Adjustment m_redBandAdj;
    Gtk::Adjustment m_greenBandAdj;
    Gtk::Adjustment m_blueBandAdj;
    Gtk::Adjustment m_downtrackSkipAdj;
    Gtk::Adjustment m_stretchMinAdj;
    Gtk::Adjustment m_stretchMaxAdj;

    Gtk::HScale m_redBandSlider;
    Gtk::HScale m_greenBandSlider;
    Gtk::HScale m_blueBandSlider;
    Gtk::HScale m_downtrackSkipSlider;
    Gtk::HScale m_stretchMinSlider;
    Gtk::HScale m_stretchMaxSlider;

    bool m_redBandSliderChanged;
    bool m_greenBandSliderChanged;
    bool m_blueBandSliderChanged;
    bool m_downtrackSkipSliderChanged;
    bool m_stretchMinSliderChanged;
    bool m_stretchMaxSliderChanged;

    void onRedBandSliderChange(void);
    void onGreenBandSliderChange(void);
    void onBlueBandSliderChange(void);
    void onDowntrackSkipSliderChange(void);
    void onStretchMinSliderChange(void);
    void onStretchMaxSliderChange(void);
public:
    SliderDialog(DisplayTab *parent);
    virtual ~SliderDialog() { }
};


class DisplayTab: public Gtk::VBox
{
friend class ImageDisplay;
friend class SliderDialog;
protected:

	/* typedefs */

    typedef struct
    {
        unsigned char outputMask[ DIO_MASK_BYTES ];
        unsigned char readBuffer[ MAX_DIO_BYTES ];  // image of data read from board
        unsigned char writeBuffer[ MAX_DIO_BYTES ]; // image of data written to board
        char name[ DIO_MAX_NAME_SIZE + 2 ];
        unsigned long productID;
        unsigned long nameSize;
        unsigned long numDIOBytes;
        unsigned long numCounters;
        __uint64_t serialNumber;
        int index;
    } DIODeviceInfo_t;
    typedef struct {
	double usableSize;
	double usableFree;
	double bytesPerSecond;
	fsid_t *fsid;
	int timeLeft;
	int maxTimePossible;
    } fs_descr_t;

        /* AccessIO Device variables section */

    bool m_recordToggleAttached;
    bool m_lineHigh;  //  Keep track of the toggle position on the AccessIO device if any
    DIODeviceInfo_t m_deviceTable[DIO_DEVICES_REQUIRED];

	/* serial-control */

    int m_headlessDevFd;

	/* recording state variables */

    enum {
	RECORD_STATE_NOT_RECORDING,
	RECORD_STATE_DARK1CAL,
	RECORD_STATE_SCIENCE,
	RECORD_STATE_DARK2CAL,
	RECORD_STATE_MEDIUMCAL,
	RECORD_STATE_BRIGHTCAL,
	RECORD_STATE_LASERCAL
    } m_recordState;
    int m_wait;
    FILE *m_imageFP;
    FILE *m_imageHdrFP;
    FILE *m_gpsFP;
    FILE *m_ppsFP;
    int m_framesWritten;
    int m_stopRequested;
    int m_stopIsAbort;
    recording_t m_currentRecording;
    struct timeval m_recordStartTime;
    int m_outOfSpace;
    int m_skipLeft;

	/* GPS init */

    u_int m_frameCountAtGPSInit;
    u_short m_gpsInitCommand[GPS_MSG_3510_WORDS];
    u_int m_gpsInitBytesSent;

	/* image dimensions -- mirrors dimensions from settings */

    int m_frameHeightLines;
    int m_frameWidthSamples;

	/* image buffer */

    unsigned short *m_imageBuffer;
    unsigned char *m_framePixels;
    unsigned char m_incr;
    unsigned char *m_waterfallPixels;

	/* image correction */

    int m_framesUntilDark;
    unsigned int *m_darkFrameAccumulation;
    int m_darkFramesAccumulated;
    unsigned short *m_darkFrame;
    int m_darkFrameBuffered;

	/* stretch */

    unsigned char *m_stretchLUT;
    unsigned char *m_stretchLUTGreen;
    unsigned char *m_stretchLUTBlue;

	/* plots */

    PlotDisplay *m_pitchAndRollPlots;
    PlotDisplay *m_headingPlot;

	/* end-to-end variables */

    char *m_endtoendStartTime;

    char *m_allgpsName;
    FILE *m_allgpsFP;

    char *m_allppsName;
    FILE *m_allppsFP;

    char *m_diagsName;
    FILE *m_diagsFP;

    char *m_logName;
    FILE *m_logFP;
    Glib::RecMutex m_logMutex;

	/* options lists */

    static const char *const shutterOptions[];
    static const char *const obcControlOptions[];
    static const u_char obcControlCodes[];
    static const char *const mountControlOptions[];
    static const u_char mountControlCodes[];

	/* pointer to settings block */

    SettingsBlock *m_block;

	/* pointer to app */

    appFrame *m_app;

	/* pointers to other tabs */

    DiagTab *m_diagTab;
    PlotsTab *m_plotsTab;
    SettingsTab *m_settingsTab;
    RecordingsTab *m_recordingsTab;
    ECSDataTab *m_ecsDataTab;

	/* buttons box */

    Gtk::HBox m_buttonHBox;
    Gtk::Button m_recordButton;
    Gtk::Button m_stopButton;

	/* OBC-state icons */

    Gtk::HBox m_obcStateHBox;
    Gtk::HBox m_idleHBox;
    StatusDisplay m_idleDisplay;
    StatusDisplay::color_t m_idleColor;
    Gtk::Label m_idleLabel;
    Gtk::HBox m_startingCalHBox;
    StatusDisplay m_startingCalDisplay;
    StatusDisplay::color_t m_startingCalColor;
    Gtk::Label m_startingCalLabel;
    Gtk::HBox m_activeHBox;
    StatusDisplay m_activeDisplay;
    StatusDisplay::color_t m_activeColor;
    Gtk::Label m_activeLabel;
    Gtk::HBox m_endingCalHBox;
    StatusDisplay m_endingCalDisplay;
    StatusDisplay::color_t m_endingCalColor;
    Gtk::Label m_endingCalLabel;

	/* misc controls */

    Gtk::HBox m_miscControlsHBox;

    Gtk::ComboBoxText *m_shutterCombo;

    Gtk::ComboBoxText *m_obcControlCombo;
    bool m_obcControlComboForced;
    int m_mostRecentOBCMode;

    Gtk::ComboBoxText *m_mountControlCombo;
    bool m_mountControlComboForced;
    int m_mostRecentMountMode;

    Gtk::Button m_gpsInitButton;

	/* vertical box for buttons and recording-state indicators */

    Gtk::VBox m_controlsVBox;

	/* health box */

    Gtk::VBox m_healthVBox;
    Gtk::HBox m_spaceAvailableHBox;
    Gtk::Label m_spaceAvailableLabel;
    Gtk::ProgressBar m_spaceAvailableBar;
    Gtk::Label m_timeLeftLabel;
    Gtk::Label m_fpsLabel;
    Gtk::Table m_checksCol1Table;
    Gtk::Table m_checksCol2Table;
    Gtk::Table m_checksCol3Table;
    Gtk::Table m_checksCol4Table;
    Gtk::HBox m_checksBox;
    Gtk::Label m_imagerCheckLabel;
    StatusDisplay m_imagerCheckDisplay;
    StatusDisplay::color_t m_imagerCheckColor;
    Gtk::Label m_gpsCommCheckLabel;
    StatusDisplay m_gpsCommCheckDisplay;
    StatusDisplay::color_t m_gpsCommCheckColor;
    Gtk::Label m_fpiePPSCheckLabel;
    StatusDisplay m_fpiePPSCheckDisplay;
    StatusDisplay::color_t m_fpiePPSCheckColor;
    Gtk::Label m_msg3CheckLabel;
    StatusDisplay m_msg3CheckDisplay;
    StatusDisplay::color_t m_msg3CheckColor;
    Gtk::Label m_cpuCheckLabel;
    StatusDisplay m_cpuCheckDisplay;
    StatusDisplay::color_t m_cpuCheckColor;
    Gtk::Label m_tempCheckLabel;
    StatusDisplay m_tempCheckDisplay;
    StatusDisplay::color_t m_tempCheckColor;
    Gtk::Label m_airNavCheckLabel;
    StatusDisplay m_airNavCheckDisplay;
    StatusDisplay::color_t m_airNavCheckColor;
    Gtk::Label m_gpsValidCheckLabel;
    StatusDisplay m_gpsValidCheckDisplay;
    StatusDisplay::color_t m_gpsValidCheckColor;
    Gtk::Label m_fpgaPPSCheckLabel;
    StatusDisplay m_fpgaPPSCheckDisplay;
    StatusDisplay::color_t m_fpgaPPSCheckColor;
    Gtk::Label m_mountCheckLabel;
    StatusDisplay m_mountCheckDisplay;
    StatusDisplay::color_t m_mountCheckColor;
    Gtk::Label m_fmsCheckLabel;
    StatusDisplay m_fmsCheckDisplay;
    StatusDisplay::color_t m_fmsCheckColor;

	/* abort button */

    Gtk::Button m_abortButton;

	/* horizontal box to hold buttons and all indicators */

    Gtk::HBox m_aboveImageDisplayHBox;

	/* image and plots area */

    ImageDisplay m_imageDisplay;
    Gtk::Label m_slidersPromptLabel;
    Gtk::VBox m_imageDisplayVBox;
    Gtk::VBox m_navVBox;
    Gtk::HBox m_imageAndPlotsHBox;

	/* annotation */

    Gtk::HBox m_logEntryHBox;
    Gtk::Label m_logEntryLabel;
    Gtk::Entry *m_logEntry;

	/* timers and threads */

    int m_dataThreadEnabled;
    Glib::Thread *m_dataThread;
    Glib::Dispatcher m_refreshIndicators;
    Glib::Dispatcher m_refreshFrameDisplay;
    Glib::Dispatcher m_refreshWaterfallDisplay;
    Glib::Dispatcher m_refreshNavDisplay;
    Glib::Dispatcher m_refreshRecordControls;
    Glib::Dispatcher m_refreshDiags;
    Glib::Dispatcher m_refreshStopButton;
    Glib::Dispatcher m_pressRecordButton;
    Glib::Dispatcher m_pressStopButton;
    Glib::Mutex m_dataThreadMutex;

    sigc::connection m_resourcesCheckTimer;

	/* mount capability */

    enum {
	MOUNTHANDLER_IDLE = 0,
	MOUNTHANDLER_S0_MAKE_CONNECTION,
	MOUNTHANDLER_S0_SEND_R,
	MOUNTHANDLER_S0_GET_R_REPLY,
	MOUNTHANDLER_S0_SEND_POUND,
	MOUNTHANDLER_S0_GET_POUND_REPLY,
	MOUNTHANDLER_SEND_S0,
	MOUNTHANDLER_GET_S0_REPLY,
	MOUNTHANDLER_S1_MAKE_CONNECTION,
	MOUNTHANDLER_S1_SEND_R,
	MOUNTHANDLER_S1_GET_R_REPLY,
	MOUNTHANDLER_S1_SEND_POUND,
	MOUNTHANDLER_S1_GET_POUND_REPLY,
	MOUNTHANDLER_SEND_S1,
	MOUNTHANDLER_GET_S1_REPLY,
	MOUNTHANDLER_SEND_S,
	MOUNTHANDLER_GET_S_REPLY,
    } m_mountHandlerState;
    int m_mountCmdIterations;
    int m_mountCmdTries;
    sigc::connection m_mountHandler;
    Glib::Mutex m_mountHandlerMutex;

	/* diagnostics */

    u_int m_frameCount;
    u_int m_framesDropped;
    u_int m_frameBacklog;
    u_int m_maxFrameBacklog;
    u_int m_msg3Count;
    u_int m_gpsCount;
    int m_numSensors;
    char **m_sensorNames;
    double *m_sensorValues;
    int *m_sensorRanges;
    int m_fpgaFrameCount;
    int m_fpgaPPSCount;
    int m_fpga81ffCount;
    int m_fpgaFramesDropped;
    int m_fpgaSerialErrors;
    int m_fpgaSerialPortCtl;
    int m_localFrameCount;
    int m_fpgaBufferDepth;
    int m_fpgaMaxBuffersUsed;

	/* recent GPS info */

    double m_lastLat;
    double m_lastLon;
    double m_lastAltitude;
    double m_lastVelNorth;
    double m_lastVelEast;
    double m_lastVelUp;
    double m_lastHeading;
    double m_lastVelMag;

	/* misc */

    int m_maxConsecutiveFrames;

	/* popup dialog */

    Glib::RefPtr<Gtk::UIManager> m_uiManager;
    Glib::RefPtr<Gtk::ActionGroup> m_actionGroup;
    Gtk::Menu *m_popupMenu;

	/* popup messages */

    CallbackDispatcher m_popupMsgDispatcher;

	/* handlers */

    void onRecordButton(void);
    void recordButtonWork(int userInvoked);
    void onStopButton(void);
    void stopButtonWork(int userInvoked);
    void onAbortButton(void);
    void abortButtonWork(int userInvoked);
    void onGPSInitButton(void);
    void onShutterCombo(void);
    void onOBCControlCombo(void);
    void onMountControlCombo(void);
    bool onRightMouseClick(GdkEventButton *event) const;
    void onLogEntryChange(void);

	/* state changes */

    void leaveIdleState(void);
    void enterDark1CalState(void);
    void enterScienceState(void);
    void enterDark2CalState(void);
    void enterMediumCalState(void);
    void enterBrightCalState(void);
    void enterLaserCalState(void);
    void enterIdleState(void);

	/* recording-mode indicators */

    void showIdleMode(void);
    void showStartingCalMode(void);
    void showActiveMode(void);
    void showEndingCalMode(void);

	/* other support routines */

    void dataThread(void);
    void lookForImageData(void);
    void getImageFrame(void);
    void simImageFrame(void);
    void addGPSSimToFrame(void);
    void processImageTimingData(void);
    void processImageGPSData(void);
    void displayCurrentFrame(void);
    void displayWaterfallLine(void);
    int checkForMoreImageData(int consecutiveFrames);

    void attachIndicator(Gtk::Table& table, int row, Gtk::Alignment& alignment,
	Gtk::Label& label, StatusDisplay& indicator);
    void indicatorUpdateWork(void);
    void frameUpdateWork(void);
    void waterfallUpdateWork(void);
    void navDisplayUpdateWork(void);
    void recordControlsUpdateWork(void);
    void diagsUpdateWork(void);
    void stopButtonUpdateWork(void);
    void handleFMSRecord(void);
    void handleFMSStop(void);
    void updateHeadlessIndicators(void);
    void writeSerialStringForColor(StatusDisplay::color_t color,
	const char *prefix);

    void showSliderDialog(void);

    int writeImageFrame(void);
    void writeAllgpsData(void);
    void writeAllppsData(void);
    int writeFlightlineGPSData(void);
    int writePPSData(void);
    int writeDataFromLine1(FILE *fp, int imageOffset, int maxByteCount,
	const char *failMsg, int *suppression_p, int flush);
    bool performResourcesCheck(void);
    bool handleMount(void);
    int setMount(int state);
    int openFiles(void);
    void closeFiles(struct timespec *closeTime_p);
    void copyFile(const char *to, const char *from);
    void calcFreeSpace(const struct statfs *sb_p, double *usableSize_p,
	double *usableFree_p) const;
    void buildUpFilesystemList(DisplayTab::fs_descr_t *fsDescrs, int *numFS_p,
	struct statfs *statbuf_p, double bytesPerSecond);

    void add3501(u_short *msg);
    void draw(void);

    void showMsg(Glib::ustring msg, Glib::ustring secondaryMsg,
	Gtk::MessageType type, Glib::ustring description);
    void saveStateToLog(void);
    void saveCSVHeader(void);
    void saveStateToCSV(void);

    bool attachRecordToggleSwitch(void);
    bool detachRecordToggleSwitch(void);

    void testDigitalLineTransition(void);
    inline bool lineIsHigh(void);

    void checkHeadlessInterface(void);
public:
    DisplayTab(SettingsBlock *sb, appFrame *app);
    ~DisplayTab();

    void disableControlChanges(int disableRecord, int disableShutterControl,
	int disableOBCControl, int disableMountControl);
    void enableControlChanges(void);
    void enableRecordButton(void);
    void disableRecordButton(void);
    void enableStopButton(void);
    void disableStopButton(void);
    void enableGPSInitButton(void);
    void disableGPSInitButton(void);
    void enableAbortButton(void);
    void disableAbortButton(void);
    void enableShutterCombo(void);
    void disableShutterCombo(void);
    void enableOBCControlCombo(void);
    void disableOBCControlCombo(void);
    void enableMountControlCombo(void);
    void disableMountControlCombo(void);

    void makeMountUsed(void);
    void makeMountUnused(void);

    void startDataThread(void);
    void startDataThreadAndBlock(void);
    void updateResourcesDisplay(void);
    void stopThreadsAndTimers(void);
    int recording(void) const {
	return (m_recordState != RECORD_STATE_NOT_RECORDING); }
    void resetImageDisplay(void) { m_imageDisplay.reset(); }
    void setDiagTab(DiagTab *dt) { m_diagTab = dt; }
    void setPlotsTab(PlotsTab *pt) { m_plotsTab = pt; }
    void setSettingsTab(SettingsTab *st) { m_settingsTab = st; }
    void setRecordingsTab(RecordingsTab *rt) { m_recordingsTab = rt; }
    void setECSDataTab(ECSDataTab *tt) { m_ecsDataTab = tt; }
    void updateStretch(void);
    void readDarkFrame(void);
    void writeDarkFrame(void);
    int log(const char *msg);
    void info(Glib::ustring msg);
    void warn(Glib::ustring msg);
    void error(Glib::ustring msg);
    void info(Glib::ustring msg, Glib::ustring secondaryMsg);
    void warn(Glib::ustring msg, Glib::ustring secondaryMsg);
    void error(Glib::ustring msg, Glib::ustring secondaryMsg);
    void cleanupForExit(void);
    void resetNavPlots(void);
    void fmsControlChanged(void);
};
