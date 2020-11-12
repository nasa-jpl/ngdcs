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


#define OBC_OPER_A_LOW     0  /* OBC TTL line values */
#define OBC_OPER_A_HIGH    1
#define OBC_OPER_B_LOW     0
#define OBC_OPER_B_HIGH    1
#define OBC_SHUTTER_LOW    0
#define OBC_SHUTTER_HIGH   1
#define OBC_TARGET_LOW     0
#define OBC_TARGET_HIGH    1
#define OBC_LASER_LOW      0
#define OBC_LASER_HIGH     1

#define OBC_GPIO_BITMASK   0x1f

#define PXI_START_RECORD   0x0100
#define PXI_STOP_RECORD    0x0200
#define PXI_GPIO_BITMASK   0x0300


class SettingsBlock;
class DisplayTab;
class DiagTab;
class PlotsTab;
class RecordingsTab;
class ECSDataTab;
class SettingsTab;

extern const char *const diagTabDate;
extern const char *const plotsTabDate;
extern const char *const displayTabDate;
extern const char *const ecsDataTabDate;
extern const char *const framebufDate;
extern const char *const libftpDate;
extern const char *const mainDate;
extern const char *const maxonDate;
extern const char *const recordingsTabDate;
extern const char *const settingsBlockDate;
extern const char *const settingsTabDate;
extern const char *const plottingDate;


class appFrame
{
protected:

	/* pointers to the settings blocks and tab objects */

    SettingsBlock *m_settingsBlock;
    DisplayTab *m_displayTab;
    DiagTab *m_diagTab;
    PlotsTab *m_plotsTab;
    RecordingsTab *m_recordingsTab;
    ECSDataTab *m_ecsDataTab;
    SettingsTab *m_settingsTab;

        /* GUI items */

    Gtk::VBox m_mainBox;
    Gtk::Notebook m_notebook;
    Gtk::Label m_displayLabel;
    Gtk::Label m_recordingsLabel;
    Gtk::Label m_ecsDataLabel;
    Gtk::Label m_settingsLabel;

    Gtk::HBox m_versionAndExit;
    Gtk::Label m_versionLabel;
    Gtk::HButtonBox m_buttonBox;
    Gtk::Button m_exitButton;

	/* other variables */

    time_t m_version;
    int m_pendingFrameHeight;
    int m_pendingFrameWidth;
    int m_pendingCompressionOption;
    int m_mountDevFd;

        /* misc support */

    void setOBCWork(u_char ttlOut1, u_char ttlOut2, u_char ttlOut3,
	u_char ttlOut4, u_char ttlOut5) const;
    int showMountError(const char *string);
    void registerSourceFile(const char *date);
    int handleExitRequest(void);
public:
        /* construction/destruction */

    int failed;
    appFrame(int h);
    ~appFrame();

	/* frame buffer */

    static FrameBuffer *fb;
    static int headless;

	/* data dir */

    static char dailyDir[MAXPATHLEN];

	/* current OBC position */

    int lastOBCState;

	/* version string */

    char versionString[200];

        /* public operations */

    void setOBC(int state);
    int openMountConnection(int *alreadyOpenFlag_p, int quiet);
    int sendMountCmd(const char *cmd, int quiet);
    int getMountResponse(char *response, int maxResponseSize,
	int *actualResponseSize_p, int quiet);
    void setPXIRecord(int recording) const;
    void comboEntryToIndex(const Gtk::ComboBoxText *combo,
	const char *const strings[], int *setting_p, const char *description);
    static int ensureDirectoryExists(const char *dir);
    static int makeDailyDir(const char *root, char *dailyDirPath);
    void disableExitButton(void) { m_exitButton.set_sensitive(false); }
    void enableExitButton(void) { m_exitButton.set_sensitive(true); }
    void setPendingFrameHeight(int selection) {
	m_pendingFrameHeight = selection; }
    void setPendingFrameWidth(int selection) {
	m_pendingFrameWidth = selection; }
    void setPendingCompressionOption(int selection) {
	m_pendingCompressionOption = selection; }
    static int openPCSerial(const char *devName, int baud);
    static int writePCSerial(int fd, const char *buf, int nbytes);
    static int readPCSerial(int fd, char *buf, int bufSize, int timeoutUs);
    static void closePCSerial(int fd);
};


class appFrameHeadless: public appFrame
{
public:
    appFrameHeadless();
    ~appFrameHeadless();
};


class appFrameWin: public Gtk::Window, public appFrame
{
protected:
    void on_exit_button(void);
    bool on_delete_event(GdkEventAny *);
public:
    appFrameWin();
    ~appFrameWin();
};
