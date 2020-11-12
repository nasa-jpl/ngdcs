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


#define MODE_OPERATIONAL   	0
#define MODE_TEST	   	1
#define NUM_MODES	   	2

#define NUM_FRAMERATES     	7

#define FRAMEHEIGHT_481LINES    1
#define NUM_FRAMEHEIGHTS        3

#define FRAMEWIDTH_640SAMPLES	0
#define NUM_FRAMEWIDTHS		2

#define COMPRESSION_UNSUPPORTED 0
#define COMPRESSION_OFF	        1
#define COMPRESSION_BOTH	2
#define COMPRESSION_ONLY        3
#define NUM_COMPRESSIONOPTIONS  4

#define FMSCONTROL_NO		0
#define FMSCONTROL_YES		1
#define NUM_FMSCONTROLOPTIONS	2

#define VIEWERTYPE_FRAME        0
#define VIEWERTYPE_WATERFALL    1
#define VIEWERTYPE_COMBO    	2
#define NUM_VIEWERTYPES	    	3

#define DARKSUBOPTION_NO	0
#define DARKSUBOPTION_YES	1
#define NUM_DARKSUBOPTIONS      2
#define DEFAULT_DARKSUBDIR      "/usr/local/lib"

#define NAVDURATION_30SECS	0
#define NAVDURATION_1MIN	1
#define NAVDURATION_2MIN	2
#define NAVDURATION_5MIN	3
#define NUM_NAVDURATIONS	4

#define REFLECTIONOPTION_OFF   	0
#define REFLECTIONOPTION_ON    	1
#define NUM_REFLECTIONOPTIONS  	2

#define RECMARGIN_NONE		0
#define RECMARGIN_0PT1PCT	1
#define RECMARGIN_0PT2PCT	2
#define RECMARGIN_0PT5PCT	3
#define RECMARGIN_1PCT		4
#define RECMARGIN_2PCT		5
#define RECMARGIN_5PCT		6
#define RECMARGIN_10PCT		7
#define RECMARGIN_20PCT		8
#define RECMARGIN_50PCT		9
#define NUM_RECMARGINS		10

#define DEFAULT_PREFIX          "NGDCS"
#define DEFAULT_PRODUCTROOTDIR  "/data"

#define OBCINTERFACE_NONE   	0
#define OBCINTERFACE_FPGA   	1
#define NUM_OBCINTERFACES   	2

#define NUM_CALPERIODS		13

#define SHUTTERINTERFACE_NONE   0
#define SHUTTERINTERFACE_MAXON  1
#define NUM_SHUTTERINTERFACES   2

#define GPSSERIALBAUD_38400BPS  0
#define GPSSERIALBAUD_115200BPS 1
#define NUM_GPSSERIALBAUDS     	2

#define GPSSERIALPARITY_NONE    0
#define GPSSERIALPARITY_EVEN    1
#define GPSSERIALPARITY_ODD     2
#define NUM_GPSSERIALPARITIES   3

#define GPSSERIALTXINVERT_OFF   0
#define GPSSERIALTXINVERT_ON    1
#define NUM_GPSSERIALTXINVERTS  2

#define GPSINITMODE_GROUND      0
#define GPSINITMODE_AIR         1
#define NUM_GPSINITMODES        2

#define ECSCONNECTED_NO		0
#define ECSCONNECTED_YES	1
#define NUM_ECSOPTIONS		2

#define MIN_ECS_TIMEOUT		0
#define MAX_ECS_TIMEOUT		5000
#define MIN_ECS_PORT		1
#define MAX_ECS_PORT		65535
#define DEFAULT_ECSHOST		"192.168.1.4"
#define DEFAULT_ECSPORT         21
#define DEFAULT_ECSTIMEOUTINMS  1000
#define DEFAULT_ECSUSERNAME     "ngdcsftp"
#define DEFAULT_ECSPASSWORD     ""

#define MOUNTUSED_NO		0
#define MOUNTUSED_YES		1
#define NUM_MOUNTOPTIONS	2
#define DEFAULT_MOUNTDEV	"/dev/ttyUSB0"

#define DEFAULT_HEADLESSDEV     "/dev/ttyUSB1"

#define IMAGESIM_NONE		0
#define IMAGESIM_INCRWITHLINE	1
#define IMAGESIM_INCRWITHFRAME	2
#define NUM_IMAGESIMS		3

#define GPSSIM_NONE		0
#define GPSSIM_ALL		1
#define GPSSIM_ALLBUT3		2
#define NUM_GPSSIMS		3

#define DIAGOPTION_OFF		0
#define DIAGOPTION_ON		1
#define NUM_DIAGOPTIONS         2

#define MAX_NGDCS_LINE_LEN	256

#define MAX_DN			((1<<14)-1)


class SettingsBlock
{
protected:
	/* context */

    int m_firstTime;
    bool m_headless;

	/* mode variables */

    int m_mode;

    static const char *const modes[NUM_MODES+1];

	/* acquisition variables */

    int m_frameRate;
    int m_frameHeight;
    int m_frameWidth;
    int m_compressionOption;
    int m_fmsControlOption;

    static const char *const frameHeights[NUM_FRAMEHEIGHTS+1];
    static const int frameHeightsLines[NUM_FRAMEHEIGHTS];
    static const char *const frameWidths[NUM_FRAMEWIDTHS+1];
    static const int frameWidthsSamples[NUM_FRAMEWIDTHS];
    static const char *const compressionOptions[NUM_COMPRESSIONOPTIONS+1];
    static const char *const fmsControlOptions[NUM_FMSCONTROLOPTIONS+1];

	/* display variables */

    int m_viewerType;
    int m_redBand;
    int m_greenBand;
    int m_blueBand;
    int m_downtrackSkip;
    int m_stretchMin;
    int m_stretchMax;
    double m_greenGain;
    double m_blueGain;
    int m_darkSubOption;
    char *m_darkSubDirMRU;
    int m_navDuration;
    int m_reflectionOption;

    static const char *const viewerTypes[NUM_VIEWERTYPES+1];
    static const char *const darkSubOptions[NUM_DARKSUBOPTIONS+1];
    static const char *const navDurations[NUM_NAVDURATIONS+1];
    static const int navDurationSkips[NUM_NAVDURATIONS];
    static const char *const reflectionOptions[NUM_REFLECTIONOPTIONS+1];

	/* data storage variables */

    int m_recMargin;
    char *m_prefix;
    char *m_productRootDirMRU;

    static const char *const recMargins[NUM_RECMARGINS+1];
    static const double recMarginPcts[NUM_RECMARGINS];

	/* calibration variables */

    int m_obcInterface;
    int m_dark1CalPeriod;
    int m_dark2CalPeriod;
    int m_mediumCalPeriod;
    int m_brightCalPeriod;
    int m_laserCalPeriod;

    static const char *const obcInterfaces[NUM_OBCINTERFACES+1];
    static const char *const calPeriods[NUM_CALPERIODS+1];
    static const u_char calPeriodLengths[NUM_CALPERIODS];

	/* shutter variables */

    int m_shutterInterface;

    static const char *const shutterInterfaces[NUM_SHUTTERINTERFACES+1];

	/* GPS variables */

    int m_gpsSerialBaud;
    int m_gpsSerialParity;
    int m_gpsSerialTXInvert;
    int m_gpsInitMode;

    static const char *const gpsSerialBauds[NUM_GPSSERIALBAUDS+1];
    static const int gpsSerialBaudsBPS[NUM_GPSSERIALBAUDS];
    static const char *const gpsSerialParities[NUM_GPSSERIALPARITIES+1];
    static const char *const gpsSerialTXInverts[NUM_GPSSERIALTXINVERTS+1];
    static const char *const gpsInitModes[NUM_GPSINITMODES+1];

	/* ECS-connection variables */

    int m_ecsConnected;
    char *m_ecsHost;
    int m_ecsPort;
    int m_ecsTimeoutInMs;
    char *m_ecsUsername;
    char *m_ecsPassword;

    static const char *const ecsOptions[NUM_ECSOPTIONS+1];

	/* Mount variables */

    int m_mountUsed;
    char *m_mountDev;

    static const char *const mountOptions[NUM_MOUNTOPTIONS+1];

	/* headless variables */

    char *m_headlessDev;

	/* sim variables */

    int m_imageSim;
    int m_gpsSim;
    int m_diagOption;

    static const char *const imageSims[NUM_IMAGESIMS+1];
    static const char *const gpsSims[NUM_GPSSIMS+1];
    static const char *const diagOptions[NUM_DIAGOPTIONS+1];

	/* private support routines */

    void getIntValue(FILE *fp, const char *key, int *value_p,
	bool warnIfMissing, int min, int max) const;
    void getDoubleValue(FILE *fp, const char *key, double *value_p,
	bool warnIfMissing, double min, double max) const;
    void getIndex(FILE *fp, const char *key, bool warnIfMissing,
	const char *const strings[], int *setting_p, const char *description)
	const;
    void getMRU(FILE *fp, const char *base, char *MRU) const;
    int getValue(FILE *fp, const char *key, char **value_p,
	bool warnIfMissing) const;
    char *decrypted(const char *password) const;
    char *encrypted(const char *password) const;
    int getMacAddr(unsigned char *addr) const;
    void insertInMRU(char **mru_p, const char *newbuf) const;
public:

        /* object creation and destruction */

    SettingsBlock(bool headless);
    ~SettingsBlock();

        /* registry load and save */

    void loadSettings(const char *filename);
    int saveSettings(const char *filename);

	/* logging */

    void logSettings(FILE *fp);

        /* current-value setting and retrieval */

    int currentFirstTime(void) const;

    const char *const *availableModes(void);
    int currentMode(void) const;
    void setMode(int value);

    int currentFrameRate(void) const;
    void setFrameRate(int value);
    const char *const *availableFrameHeights(void);
    int currentFrameHeight(void) const;
    void setFrameHeight(int value);
    int currentFrameHeightLines(void) const;
    const char *const *availableFrameWidths(void);
    int currentFrameWidth(void) const;
    void setFrameWidth(int value);
    int currentFrameWidthSamples(void) const;
    const char *const *availableCompressionOptions(void);
    int currentCompressionOption(void) const;
    void setCompressionOption(int value);
    const char *const *availableFMSControlOptions(void);
    int currentFMSControlOption(void) const;
    void setFMSControlOption(int value);

    const char *const *availableViewerTypes(void);
    int currentViewerType(void) const;
    void setViewerType(int value);
    int currentRedBand(void) const;
    void setRedBand(int value);
    int currentGreenBand(void) const;
    void setGreenBand(int value);
    int currentBlueBand(void) const;
    void setBlueBand(int value);
    int currentDowntrackSkip(void) const;
    void setDowntrackSkip(int value);
    int currentStretchMin(void) const;
    void setStretchMin(int value);
    int currentStretchMax(void) const;
    void setStretchMax(int value);
    double currentGreenGain(void) const;
    void setGreenGain(double value);
    double currentBlueGain(void) const;
    void setBlueGain(double value);
    const char *const *availableDarkSubOptions(void);
    int currentDarkSubOption(void) const;
    void setDarkSubOption(int value);
    char *currentDarkSubDir(void) const;
    char *currentDarkSubDirMRU(void) const;
    void insertInDarkSubDirMRU(const char *dir);
    const char *const *availableNavDurations(void);
    int currentNavDuration(void) const;
    void setNavDuration(int value);
    int currentNavDurationSkip(void) const;
    int currentNavDurationSecs(void) const;
    const char *const *availableReflectionOptions(void);
    int currentReflectionOption(void) const;
    void setReflectionOption(int value);

    const char *const *availableRecMargins(void);
    int currentRecMargin(void) const;
    void setRecMargin(int value);
    double currentRecMarginPct(void) const;
    char *currentPrefix(void) const;
    void setPrefix(const char *prefix);
    char *currentProductRootDir(void) const;
    char *currentProductRootDirMRU(void) const;
    void insertInProductRootDirMRU(const char *dir);

    const char *const *availableOBCInterfaces(void);
    int currentOBCInterface(void) const;
    void setOBCInterface(int value);
    const char *const *availableCalPeriods(void);
    int currentDark1CalPeriod(void) const;
    int currentDark1CalPeriodInSecs(void) const;
    void setDark1CalPeriod(int value);
    int currentDark2CalPeriod(void) const;
    int currentDark2CalPeriodInSecs(void) const;
    void setDark2CalPeriod(int value);
    int currentMediumCalPeriod(void) const;
    int currentMediumCalPeriodInSecs(void) const;
    void setMediumCalPeriod(int value);
    int currentBrightCalPeriod(void) const;
    int currentBrightCalPeriodInSecs(void) const;
    void setBrightCalPeriod(int value);
    int currentLaserCalPeriod(void) const;
    int currentLaserCalPeriodInSecs(void) const;
    void setLaserCalPeriod(int value);

    const char *const *availableShutterInterfaces(void);
    int currentShutterInterface(void) const;
    void setShutterInterface(int value);

    const char *const *availableGPSSerialBauds(void);
    int currentGPSSerialBaud(void) const;
    void setGPSSerialBaud(int value);
    int currentGPSSerialBaudBPS(void) const;
    const char *const *availableGPSSerialParities(void);
    int currentGPSSerialParity(void) const;
    void setGPSSerialParity(int value);
    const char *const *availableGPSSerialTXInverts(void);
    int currentGPSSerialTXInvert(void) const;
    void setGPSSerialTXInvert(int value);
    const char *const *availableGPSInitModes(void);
    int currentGPSInitMode(void) const;
    void setGPSInitMode(int value);

    const char *const *availableECSOptions(void);
    int currentECSConnected(void) const;
    void setECSConnected(int value);
    char *currentECSHost(void) const;
    void setECSHost(const char *host);
    int currentECSPort(void) const;
    void setECSPort(int value);
    int currentECSTimeoutInMs(void) const;
    void setECSTimeoutInMs(int value);
    char *currentECSUsername(void) const;
    void setECSUsername(const char *username);
    char *currentECSPassword(void) const;
    void setECSPassword(const char *password);

    const char *const *availableMountOptions(void);
    int currentMountUsed(void) const;
    void setMountUsed(int value);
    char *currentMountDev(void) const;
    void setMountDev(const char *dev);

    char *currentHeadlessDev(void) const;
    void setHeadlessDev(const char *dev);

    const char *const *availableImageSims(void);
    int currentImageSim(void) const;
    void setImageSim(int value);
    const char *const *availableGPSSims(void);
    int currentGPSSim(void) const;
    void setGPSSim(int value);
    const char *const *availableDiagOptions(void);
    int currentDiagOption(void) const;
    void setDiagOption(int value);

	/* misc public routines */

    void stringToIndex(const char *string, const char *const strings[],
	int *setting_p, const char *description) const;
};
