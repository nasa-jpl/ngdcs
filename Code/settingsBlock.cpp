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
#include <sys/types.h>
#include <sys/param.h>
#include <sys/mount.h>
#if defined(__APPLE__)
#include <ifaddrs.h>
#include <net/if_dl.h>
#endif
#include <pwd.h>
#include <gtkmm.h>
#include <tomcrypt.h>
#include "settingsBlock.h"

extern const char *const settingsBlockDate = "$Date: 2015/12/03 21:33:15 $";

// #define DEBUG

const char *const SettingsBlock::modes[NUM_MODES+1] = {
    "Operational", "Test", NULL };

const char *const SettingsBlock::frameHeights[NUM_FRAMEHEIGHTS+1] = {
    "285 lines",
    "481 lines",
    "1024 lines", NULL };
const int SettingsBlock::frameHeightsLines[NUM_FRAMEHEIGHTS] = {
    285, 481, 1024 };
const char *const SettingsBlock::frameWidths[NUM_FRAMEWIDTHS+1] = {
    "640 pixels",
    "1024 pixels", NULL };
const int SettingsBlock::frameWidthsSamples[NUM_FRAMEWIDTHS] = {
    640, 1024 };
const char *const SettingsBlock::compressionOptions[
	NUM_COMPRESSIONOPTIONS+1] = {
    "Unsupported", "Off", "Both", "Only", NULL };
const char *const SettingsBlock::fmsControlOptions[NUM_FMSCONTROLOPTIONS+1] = {
    "No", "Yes", NULL };

const char *const SettingsBlock::viewerTypes[NUM_VIEWERTYPES+1] = {
    "Frame", "Waterfall", "Combo", NULL };
const char *const SettingsBlock::darkSubOptions[NUM_DARKSUBOPTIONS+1] = {
    "No", "Yes", NULL };
const char *const SettingsBlock::navDurations[NUM_NAVDURATIONS+1] = {
    "30 secs", "1 min", "2 mins", "5 mins", NULL };
const int SettingsBlock::navDurationSkips[NUM_NAVDURATIONS] = {
    0, 1, 3, 9 };
const char *const SettingsBlock::reflectionOptions[NUM_REFLECTIONOPTIONS+1] = {
    "Off", "On", NULL };

const char *const SettingsBlock::recMargins[NUM_RECMARGINS+1] = {
    "0 %",
    "0.1 %", "0.2 %", "0.5 %",
    "1 %", "2 %", "5 %",
    "10 %", "20 %", "50 %",
    NULL };
const double SettingsBlock::recMarginPcts[NUM_RECMARGINS] = {
    0.0,
    0.1, 0.2, 0.5,
    1.0, 2.0, 5.0,
    10.0, 20.0, 50.0 };

const char *const SettingsBlock::obcInterfaces[NUM_OBCINTERFACES+1] = {
    "None", "FPGA", NULL };
const char *const SettingsBlock::calPeriods[NUM_CALPERIODS+1] = {
    "Disabled", "1 sec", "2 secs", "3 secs", "4 secs", "5 secs",
    "6 secs", "7 secs", "8 secs", "9 secs", "10 secs", "11 secs", "12 secs",
    NULL };
const u_char SettingsBlock::calPeriodLengths[NUM_CALPERIODS] = {
    0,1,2,3,4,5,6,7,8,9,10,11,12 };

const char *const SettingsBlock::shutterInterfaces[NUM_SHUTTERINTERFACES+1] = {
    "None", "Maxon", NULL };

const char *const SettingsBlock::gpsSerialBauds[NUM_GPSSERIALBAUDS+1] = {
    "38400 bps",
    "115200 bps", NULL };
const int SettingsBlock::gpsSerialBaudsBPS[NUM_GPSSERIALBAUDS] = {
    38400,
    115200 };

const char *const SettingsBlock::gpsSerialParities[NUM_GPSSERIALPARITIES+1] = {
    "None", "Even", "Odd", NULL };

const char *const SettingsBlock::gpsSerialTXInverts[NUM_GPSSERIALTXINVERTS+1] = {
    "Off", "On", NULL };

const char *const SettingsBlock::gpsInitModes[NUM_GPSINITMODES+1] = {
    "Ground", "Air", NULL };

const char *const SettingsBlock::ecsOptions[NUM_ECSOPTIONS+1] = {
    "No", "Yes", NULL };

const char *const SettingsBlock::mountOptions[NUM_MOUNTOPTIONS+1] = {
    "No", "Yes", NULL };

const char *const SettingsBlock::imageSims[NUM_IMAGESIMS+1] = {
    "None", "DN increases with line", "DN increases with frame", NULL };
const char *const SettingsBlock::gpsSims[NUM_GPSSIMS+1] = {
    "None", "All messages", "All messages except #3", NULL };
const char *const SettingsBlock::diagOptions[NUM_DIAGOPTIONS+1] = {
    "Off", "On", NULL };


SettingsBlock::SettingsBlock(bool headless)
{
    int i;

	/* establish defaults */

    m_firstTime = 1;
    m_headless = headless;

    m_mode = MODE_OPERATIONAL;

    m_frameRate = NUM_FRAMERATES-1;
    m_frameHeight = FRAMEHEIGHT_481LINES;
    m_frameWidth = FRAMEWIDTH_640SAMPLES;
    m_compressionOption = COMPRESSION_UNSUPPORTED;
    m_fmsControlOption = FMSCONTROL_NO;

    m_viewerType = VIEWERTYPE_WATERFALL;
    m_redBand = 1;
    m_greenBand = 1;
    m_blueBand = 1;
    m_downtrackSkip = 0;
    m_stretchMin = 0;
    m_stretchMax = MAX_DN;
    m_greenGain = 1.0;
    m_blueGain = 1.0;
    m_darkSubOption = DARKSUBOPTION_NO;
    m_darkSubDirMRU = new char[MAXPATHLEN*4+1];
    strcpy(m_darkSubDirMRU, DEFAULT_DARKSUBDIR);
    for (i=1;i < 4;i++)
	m_darkSubDirMRU[i*MAXPATHLEN] = '\0';
    m_navDuration = NAVDURATION_30SECS;
    m_reflectionOption = REFLECTIONOPTION_OFF;

    m_recMargin = RECMARGIN_NONE;
    m_prefix = new char[strlen(DEFAULT_PREFIX)+1];
    strcpy(m_prefix, DEFAULT_PREFIX);
    m_productRootDirMRU = new char[MAXPATHLEN*4+1];
    strcpy(m_productRootDirMRU, DEFAULT_PRODUCTROOTDIR);
    for (i=1;i < 4;i++)
	m_productRootDirMRU[i*MAXPATHLEN] = '\0';

    m_obcInterface = OBCINTERFACE_NONE;
    m_dark1CalPeriod = 0;
    m_dark2CalPeriod = 0;
    m_mediumCalPeriod = 0;
    m_brightCalPeriod = 0;
    m_laserCalPeriod = 0;

    m_shutterInterface = SHUTTERINTERFACE_NONE;

    m_gpsSerialBaud = GPSSERIALBAUD_38400BPS;
    m_gpsSerialParity = GPSSERIALPARITY_ODD;
    m_gpsSerialTXInvert = GPSSERIALTXINVERT_OFF;
    m_gpsInitMode = GPSINITMODE_AIR;

    m_ecsConnected = ECSCONNECTED_YES;
    m_ecsHost = new char[strlen(DEFAULT_ECSHOST)+1];
    strcpy(m_ecsHost, DEFAULT_ECSHOST);
    m_ecsPort = DEFAULT_ECSPORT;
    m_ecsTimeoutInMs = DEFAULT_ECSTIMEOUTINMS;
    m_ecsUsername = new char[strlen(DEFAULT_ECSUSERNAME)+1];
    strcpy(m_ecsUsername, DEFAULT_ECSUSERNAME);
    m_ecsPassword = new char[strlen(DEFAULT_ECSPASSWORD)+1];
    strcpy(m_ecsPassword, DEFAULT_ECSPASSWORD);

    m_mountUsed = MOUNTUSED_NO;
    m_mountDev = new char[strlen(DEFAULT_MOUNTDEV)+1];
    strcpy(m_mountDev, DEFAULT_MOUNTDEV);

    m_headlessDev = new char[strlen(DEFAULT_HEADLESSDEV)+1];
    strcpy(m_headlessDev, DEFAULT_HEADLESSDEV);

    m_imageSim = IMAGESIM_NONE;
    m_gpsSim = GPSSIM_NONE;
    m_diagOption = DIAGOPTION_OFF;
}


SettingsBlock::~SettingsBlock()
{
    delete[] m_darkSubDirMRU;
    delete[] m_prefix;
    delete[] m_productRootDirMRU;
    delete[] m_ecsHost;
    delete[] m_ecsUsername;
    delete[] m_ecsPassword;
    delete[] m_mountDev;
    delete[] m_headlessDev;
}


void SettingsBlock::loadSettings(const char *filename)
{
    FILE *fp;
    char *encrypted, *plainText;

	/* open file -- if there isn't one yet, we're done */

    if ((fp=fopen(filename, "r")) == NULL)
	return;

        /* read in previous values if possible.  if not, we'll use defaults
	   and save the current settings when we leave */

    getIntValue(fp, "firsttime", &m_firstTime, false, 0, 1);

    getIndex(fp, "mode", true, modes, &m_mode, "mode");

    getIntValue(fp, "framerateindex", &m_frameRate, true, 0, NUM_FRAMERATES-1);
    getIndex(fp, "frameheight", true, frameHeights, &m_frameHeight,
	"frame height");
    getIndex(fp, "framewidth", true, frameWidths, &m_frameWidth,
	"frame width");
    getIndex(fp, "compressionoption", true, compressionOptions,
	&m_compressionOption, "compression option");
    getIndex(fp, "fmscontroloption", true, fmsControlOptions,
	&m_fmsControlOption, "FMS control option");

    getIndex(fp, "viewertype", true, viewerTypes, &m_viewerType, "viewer type");
    getIntValue(fp, "redband", &m_redBand, true, 1,
	currentFrameHeightLines()-1);
    getIntValue(fp, "greenband", &m_greenBand, true, 1,
	currentFrameHeightLines()-1);
    getIntValue(fp, "blueband", &m_blueBand, true, 1,
	currentFrameHeightLines()-1);
    getIntValue(fp, "downtrackskip", &m_downtrackSkip, true, 0, 9);
    getIntValue(fp, "stretchmin", &m_stretchMin, true, 0, MAX_DN);
    getIntValue(fp, "stretchmax", &m_stretchMax, true, 0, MAX_DN);
    getDoubleValue(fp, "greengain", &m_greenGain, true, 0.0, 10.0);
    getDoubleValue(fp, "bluegain", &m_blueGain, true, 0.0, 10.0);
    getIndex(fp, "darksuboption", true, darkSubOptions,
	&m_darkSubOption, "dark-subtraction option");
    getMRU(fp, "darksubdir", m_darkSubDirMRU);
    getIndex(fp, "navduration", true, navDurations, &m_navDuration,
	"nav-plot duration");
    getIndex(fp, "reflectionoption", true, reflectionOptions,
	&m_reflectionOption, "reflection option");

    getIndex(fp, "recmargin", true, recMargins, &m_recMargin,
	"recording margin");
    (void)getValue(fp, "prefix", &m_prefix, true);
    getMRU(fp, "productrootdir", m_productRootDirMRU);

    getIndex(fp, "obcinterface", true, obcInterfaces, &m_obcInterface,
	"OBC interface");
    getIndex(fp, "dark1calperiod", true, calPeriods, &m_dark1CalPeriod,
	"first dark cal period");
    getIndex(fp, "dark2calperiod", true, calPeriods, &m_dark2CalPeriod,
	"second dark cal period");
    getIndex(fp, "mediumcalperiod", true, calPeriods, &m_mediumCalPeriod,
	"medium cal period");
    getIndex(fp, "brightcalperiod", true, calPeriods, &m_brightCalPeriod,
	"bright cal period");
    getIndex(fp, "lasercalperiod", true, calPeriods, &m_laserCalPeriod,
	"laser cal period");

    getIndex(fp, "shutterinterface", true, shutterInterfaces,
	&m_shutterInterface, "shutter interface");

    getIndex(fp, "gpsserialbaud", true, gpsSerialBauds, &m_gpsSerialBaud,
	"GPS serial baudrate");
    getIndex(fp, "gpsserialparity", true, gpsSerialParities,
	&m_gpsSerialParity, "GPS serial parity");
    getIndex(fp, "gpsserialtxinvert", true, gpsSerialTXInverts,
	&m_gpsSerialTXInvert, "GPS serial TX-invert");
    getIndex(fp, "gpsinitmode", true, gpsInitModes, &m_gpsInitMode,
	"GPS init mode");

    getIndex(fp, "ecsconnected", true, ecsOptions, &m_ecsConnected,
	"ECS presence");
    (void)getValue(fp, "ecshost", &m_ecsHost, true);
    getIntValue(fp, "ecsport", &m_ecsPort, true, MIN_ECS_PORT, MAX_ECS_PORT);
    getIntValue(fp, "ecstimeoutinms", &m_ecsTimeoutInMs, true, MIN_ECS_TIMEOUT,
	MAX_ECS_TIMEOUT);
    (void)getValue(fp, "ecsusername", &m_ecsUsername, true);
    if (getValue(fp, "ecspassword", &encrypted, true) == -1)
	(void)strcpy(m_ecsPassword, "");
    else {
	plainText = decrypted(encrypted);
	delete[] encrypted;
	delete[] m_ecsPassword;
	m_ecsPassword = new char[strlen(plainText)+1];
	strcpy(m_ecsPassword, plainText);
    }

    getIndex(fp, "mountused", true, mountOptions, &m_mountUsed,
	"mount availability");
    (void)getValue(fp, "mountdev", &m_mountDev, true);

    (void)getValue(fp, "headlessdev", &m_headlessDev, true);

    getIndex(fp, "imagesim", true, imageSims, &m_imageSim, "image sim");
    getIndex(fp, "gpssim", true, gpsSims, &m_gpsSim, "GPS sim");
    getIndex(fp, "diagoption", true, diagOptions,
	&m_diagOption, "diagnostics option");

	/* we're done */

    fclose(fp);
}


int SettingsBlock::saveSettings(const char *filename)
{
    int i;
    FILE *fp;

	/* open file */

    if ((fp=fopen(filename, "w")) == NULL)
	return -1;

        /* save the current settings.  firsttime is forced to 0 */

    fprintf(fp, "firsttime = 0\n");

    fprintf(fp, "mode = %s\n", modes[m_mode]);

    fprintf(fp, "framerateindex = %d\n", m_frameRate);
    fprintf(fp, "frameheight = %s\n", frameHeights[m_frameHeight]);
    fprintf(fp, "framewidth = %s\n", frameWidths[m_frameWidth]);
    fprintf(fp, "compressionoption = %s\n",
	compressionOptions[m_compressionOption]);
    fprintf(fp, "fmscontroloption = %s\n",
	fmsControlOptions[m_fmsControlOption]);

    fprintf(fp, "viewertype = %s\n", viewerTypes[m_viewerType]);
    fprintf(fp, "redband = %d\n", m_redBand);
    fprintf(fp, "greenband = %d\n", m_greenBand);
    fprintf(fp, "blueband = %d\n", m_blueBand);
    fprintf(fp, "downtrackskip = %d\n", m_downtrackSkip);
    fprintf(fp, "stretchmin = %d\n", m_stretchMin);
    fprintf(fp, "stretchmax = %d\n", m_stretchMax);
    fprintf(fp, "greengain = %g\n", m_greenGain);
    fprintf(fp, "bluegain = %g\n", m_blueGain);
    fprintf(fp, "darksuboption = %s\n", darkSubOptions[m_darkSubOption]);
    for (i=0;i < 4;i++)
	fprintf(fp, "darksubdir%d = %s\n", i, m_darkSubDirMRU+i*MAXPATHLEN);
    fprintf(fp, "navduration = %s\n", navDurations[m_navDuration]);
    fprintf(fp, "reflectionoption = %s\n",
	reflectionOptions[m_reflectionOption]);

    fprintf(fp, "recmargin = %s\n", recMargins[m_recMargin]);
    fprintf(fp, "prefix = %s\n", currentPrefix());
    for (i=0;i < 4;i++)
	fprintf(fp, "productrootdir%d = %s\n",
	    i, m_productRootDirMRU+i*MAXPATHLEN);

    fprintf(fp, "obcinterface = %s\n", obcInterfaces[m_obcInterface]);
    fprintf(fp, "dark1calperiod = %s\n", calPeriods[m_dark1CalPeriod]);
    fprintf(fp, "dark2calperiod = %s\n", calPeriods[m_dark2CalPeriod]);
    fprintf(fp, "mediumcalperiod = %s\n", calPeriods[m_mediumCalPeriod]);
    fprintf(fp, "brightcalperiod = %s\n", calPeriods[m_brightCalPeriod]);
    fprintf(fp, "lasercalperiod = %s\n", calPeriods[m_laserCalPeriod]);

    fprintf(fp, "shutterinterface = %s\n",
	shutterInterfaces[m_shutterInterface]);

    fprintf(fp, "gpsserialbaud = %s\n", gpsSerialBauds[m_gpsSerialBaud]);
    fprintf(fp, "gpsserialparity = %s\n",
	gpsSerialParities[m_gpsSerialParity]);
    fprintf(fp, "gpsserialtxinvert = %s\n",
	gpsSerialTXInverts[m_gpsSerialTXInvert]);
    fprintf(fp, "gpsinitmode = %s\n", gpsInitModes[m_gpsInitMode]);

    fprintf(fp, "ecsconnected = %s\n", ecsOptions[m_ecsConnected]);
    fprintf(fp, "ecshost = %s\n", currentECSHost());
    fprintf(fp, "ecsport = %d\n", m_ecsPort);
    fprintf(fp, "ecstimeoutinms = %d\n", m_ecsTimeoutInMs);
    fprintf(fp, "ecsusername = %s\n", currentECSUsername());
    fprintf(fp, "ecspassword = %s\n", encrypted(currentECSPassword()));

    fprintf(fp, "mountused = %s\n", mountOptions[m_mountUsed]);
    fprintf(fp, "mountdev = %s\n", currentMountDev());

    fprintf(fp, "headlessdev = %s\n", currentHeadlessDev());

    fprintf(fp, "imagesim = %s\n", imageSims[m_imageSim]);
    fprintf(fp, "gpssim = %s\n", gpsSims[m_gpsSim]);
    fprintf(fp, "diagoption = %s\n", diagOptions[m_diagOption]);
    fclose(fp);
    return 0;
}


void SettingsBlock::logSettings(FILE *fp)
{
    fprintf(fp, "Mode = %s\n", modes[m_mode]);

    fprintf(fp, "Frame rate = %d\n", m_frameRate);
    fprintf(fp, "Frame height = %s\n", frameHeights[m_frameHeight]);
    fprintf(fp, "Frame width = %s\n", frameWidths[m_frameWidth]);
    fprintf(fp, "Compression option = %s\n",
	compressionOptions[m_compressionOption]);
    fprintf(fp, "FMS control option = %s\n",
	fmsControlOptions[m_fmsControlOption]);

    fprintf(fp, "Viewer type = %s\n", viewerTypes[m_viewerType]);
    fprintf(fp, "Red band = %d\n", m_redBand);
    fprintf(fp, "Green band = %d\n", m_greenBand);
    fprintf(fp, "Blue band = %d\n", m_blueBand);
    fprintf(fp, "Downtrack skip = %d\n", m_downtrackSkip);
    fprintf(fp, "Stretch min = %d DN\n", m_stretchMin);
    fprintf(fp, "Stretch max = %d DN\n", m_stretchMax);
    fprintf(fp, "Green gain = %g\n", m_greenGain);
    fprintf(fp, "Blue gain = %g\n", m_blueGain);
    fprintf(fp, "Dark subtraction = %s\n", darkSubOptions[m_darkSubOption]);
    fprintf(fp, "Dark subtraction dir = %s\n", m_darkSubDirMRU);
    fprintf(fp, "Nav-plot duration = %s\n", navDurations[m_navDuration]);
    fprintf(fp, "Reflection option = %s\n",
	reflectionOptions[m_reflectionOption]);

    fprintf(fp, "Recording margin = %s\n", recMargins[m_recMargin]);
    fprintf(fp, "Data product prefix = %s\n", currentPrefix());
    fprintf(fp, "Product root directory = %s\n", m_productRootDirMRU);

    fprintf(fp, "OBC interface = %s\n", obcInterfaces[m_obcInterface]);
    fprintf(fp, "Dark 1 cal period = %s\n", calPeriods[m_dark1CalPeriod]);
    fprintf(fp, "Dark 2 cal period = %s\n", calPeriods[m_dark2CalPeriod]);
    fprintf(fp, "Medium cal period = %s\n", calPeriods[m_mediumCalPeriod]);
    fprintf(fp, "Bright cal period = %s\n", calPeriods[m_brightCalPeriod]);
    fprintf(fp, "Laser cal period = %s\n", calPeriods[m_laserCalPeriod]);

    fprintf(fp, "Shutter interface = %s\n",
	shutterInterfaces[m_shutterInterface]);

    fprintf(fp, "GPS serial baud = %s\n", gpsSerialBauds[m_gpsSerialBaud]);
    fprintf(fp, "GPS serial parity = %s\n",
	gpsSerialParities[m_gpsSerialParity]);
    fprintf(fp, "GPS serial TX-invert = %s\n",
	gpsSerialTXInverts[m_gpsSerialTXInvert]);
    fprintf(fp, "GPS init mode = %s\n", gpsInitModes[m_gpsInitMode]);

    fprintf(fp, "ECS connected = %s\n", ecsOptions[m_ecsConnected]);
    fprintf(fp, "ECS host = %s\n", currentECSHost());
    fprintf(fp, "ECS port = %d\n", m_ecsPort);
    fprintf(fp, "ECS timeout = %d ms\n", m_ecsTimeoutInMs);
    fprintf(fp, "ECS username = %s\n", currentECSUsername());
    fprintf(fp, "ECS password = %s\n", encrypted(currentECSPassword()));

    fprintf(fp, "Mount used = %s\n", mountOptions[m_mountUsed]);
    fprintf(fp, "Mount dev = %s\n", currentMountDev());

    fprintf(fp, "Headless dev = %s\n", currentHeadlessDev());

    fprintf(fp, "Image simulation = %s\n", imageSims[m_imageSim]);
    fprintf(fp, "GPS simulation = %s\n", gpsSims[m_gpsSim]);
    fprintf(fp, "Diag option = %s\n", diagOptions[m_diagOption]);
}


char *SettingsBlock::decrypted(const char *password) const
{
    char c;
    unsigned char digit, pt[8], ct[8], key[8];
    symmetric_key skey;
    int i, j;
    static char result[64];

	/* get MAC address as key */

    (void)memset(key, 0, sizeof(key));
    (void)getMacAddr(key);
#if defined(DEBUG)
    printf("key = ");
    for (i=0;i < 8;i++) printf("%02x ", key[i]);
    printf("\n");
#endif

	/* init encryption.  if we can't decrypt for some reason (bad library
	   configuration probably), all we can do is return empty string */

    if (blowfish_setup(key, 8, 0, &skey) != CRYPT_OK) {
        *result = '\0';
	return result;
    }

	/* for each 16-character chunk (i.e., 8 bytes)... */

    for (j=0;j < (int)strlen(password);j+=16) {
#if defined(DEBUG)
	printf("decryption input = ");
#endif

	    /* convert ASCII hex into binary */

	for (i=0;i < 16;i+=2) {
	    c = password[j+i];
	    digit = (u_char)(c <= '9'? c-'0':c-'a'+10);
	    ct[i/2] = (u_char)(digit << 4);
	    c = password[j+i+1];
	    digit = (u_char)(c <= '9'? c-'0':c-'a'+10);
	    ct[i/2] += digit;
#if defined(DEBUG)
	    printf("%02x ", ct[i/2]);
#endif
	}
#if defined(DEBUG)
	printf("\n");
#endif

	    /* decrypt using our key from above */

	(void)blowfish_ecb_decrypt(ct, pt, &skey);
#if defined(DEBUG)
	printf("decryption output = ");
	for (i=0;i < 8;i++) printf("%02x ", pt[i]);
	printf("\n");
#endif

	    /* save result in caller's buffer */

	for (i=0;i < 8;i++) result[j/2+i] = (char)pt[i];
    }
#if defined(DEBUG)
    printf("decryption finished = %s\n", result);
#endif

	/* we're done */

    blowfish_done(&skey);
    return result;
}


char *SettingsBlock::encrypted(const char *password) const
{
    unsigned char digit, pt[8], ct[8], key[8];
    symmetric_key skey;
    int i, j;
    static char result[64];

#if defined(DEBUG)
	/* show input string */

    printf("clear = %s\n", password);
#endif

	/* use the MAC address as key.  this approach at least separates the
	   key from the source code */

    (void)memset(key, 0, sizeof(key));
    (void)getMacAddr(key);
#if defined(DEBUG)
    printf("key = ");
    for (i=0;i < 8;i++) printf("%02x ", key[i]);
    printf("\n");
#endif

	/* init encryption.  if we can't encrypt for some reason (bad
	   library configuration probably), just return empty string */

    if (blowfish_setup(key, 8, 0, &skey) != CRYPT_OK) {
        *result = '\0';
	return result;
    }

	/* for each 8-character grouping in password... */

    for (j=0;j < (int)(strlen(password)+7)/8;j++) {

	    /* make a copy.  we have to look at all 8 bytes, even if they
	       don't exist in the original array, and strncpy simplifies this */

	(void)strncpy((char *)pt, password+j*8, 8);

	    /* encrypt into "ct" using the key we established above */

	(void)blowfish_ecb_encrypt(pt, ct, &skey);
#if defined(DEBUG)
	printf("encryption output = ");
	for (i=0;i < 8;i++) printf("%02x ", ct[i]);
	printf("\n");
#endif

	    /* convert "ct" into ASCII hex in our output array */

	for (i=0;i < 8;i++) {
	    digit = ct[i] >> 4;
	    result[j*16+2*i] = (char)(digit < 10? '0'+digit:'a'+digit-10);
	    digit = ct[i] & 0xf;
	    result[j*16+2*i+1] = (char)(digit < 10? '0'+digit:'a'+digit-10);
	}
    }

	/* make the end of our output string */

    result[j*16] = '\0';
#if defined(DEBUG)
    printf("encryption finished = %s\n", result);
#endif

	/* we're done -- return our encryption */

    blowfish_done(&skey);
    return result;
}


int SettingsBlock::getMacAddr(unsigned char *addr) const
{
#if defined(__APPLE__)
    struct ifaddrs *ifa;
    struct sockaddr *sa;
    struct sockaddr_dl *dl;

    if (getifaddrs(&ifa) == -1)
        return -1;
    else {
        while (ifa != NULL) {
            sa = ifa->ifa_addr;
            if (strncmp(ifa->ifa_name, "en", 2) == 0 &&
		    sa->sa_family == AF_LINK) {
                dl = (struct sockaddr_dl *)sa; /*lint !e740 !e826 bad cast */
                (void)memcpy(addr, &dl->sdl_data[dl->sdl_nlen], 6);
                return 0;
            }
            ifa = ifa->ifa_next;
        }
        return -1;
    }
#else
    int i, byte;
    FILE *fp;
    fp = fopen("/sys/class/net/em1/address", "r");
    if (!fp) 
	return -1;
    else {
	for (i=0;i < 6;i++) {
	    fscanf(fp, "%02x", &byte);
	    fgetc(fp);
	    *(addr+i) = byte;
	}
	fclose(fp);
	return 0;
    }
#endif
}


int SettingsBlock::currentFirstTime(void) const
{
    return m_firstTime;
}


const char *const *SettingsBlock::availableModes(void)
{
    return modes;
}


int SettingsBlock::currentMode(void) const
{
    return m_mode;
}


void SettingsBlock::setMode(int value)
{
    m_mode = value;
}


int SettingsBlock::currentFrameRate(void) const
{
    return m_frameRate;
}


void SettingsBlock::setFrameRate(int value)
{
    m_frameRate = value;
}


const char *const *SettingsBlock::availableFrameHeights(void)
{
    return frameHeights;
}


int SettingsBlock::currentFrameHeight(void) const
{
    return m_frameHeight;
}


void SettingsBlock::setFrameHeight(int value)
{
    m_frameHeight = value;
}


int SettingsBlock::currentFrameHeightLines(void) const
{
    return frameHeightsLines[m_frameHeight];
}


const char *const *SettingsBlock::availableFrameWidths(void)
{
    return frameWidths;
}


int SettingsBlock::currentFrameWidth(void) const
{
    return m_frameWidth;
}


void SettingsBlock::setFrameWidth(int value)
{
    m_frameWidth = value;
}


int SettingsBlock::currentFrameWidthSamples(void) const
{
    return frameWidthsSamples[m_frameWidth];
}


const char *const *SettingsBlock::availableCompressionOptions(void)
{
    return compressionOptions;
}


int SettingsBlock::currentCompressionOption(void) const
{
    return m_compressionOption;
}


void SettingsBlock::setCompressionOption(int value)
{
    m_compressionOption = value;
}


const char *const *SettingsBlock::availableFMSControlOptions(void)
{
    return fmsControlOptions;
}


int SettingsBlock::currentFMSControlOption(void) const
{
    return m_fmsControlOption;
}


void SettingsBlock::setFMSControlOption(int value)
{
    m_fmsControlOption = value;
}


const char *const *SettingsBlock::availableViewerTypes(void)
{
    return viewerTypes;
}


int SettingsBlock::currentViewerType(void) const
{
    return m_viewerType;
}


void SettingsBlock::setViewerType(int value)
{
    m_viewerType = value;
}


int SettingsBlock::currentRedBand(void) const
{
    return m_redBand;
}


void SettingsBlock::setRedBand(int value)
{
    m_redBand = value;
}


int SettingsBlock::currentGreenBand(void) const
{
    return m_greenBand;
}


void SettingsBlock::setGreenBand(int value)
{
    m_greenBand = value;
}


int SettingsBlock::currentBlueBand(void) const
{
    return m_blueBand;
}


void SettingsBlock::setBlueBand(int value)
{
    m_blueBand = value;
}


int SettingsBlock::currentDowntrackSkip(void) const
{
    return m_downtrackSkip;
}


void SettingsBlock::setDowntrackSkip(int value)
{
    m_downtrackSkip = value;
}


int SettingsBlock::currentStretchMin(void) const
{
    return m_stretchMin;
}


void SettingsBlock::setStretchMin(int value)
{
    m_stretchMin = value;
}


int SettingsBlock::currentStretchMax(void) const
{
    return m_stretchMax;
}


void SettingsBlock::setStretchMax(int value)
{
    m_stretchMax = value;
}


double SettingsBlock::currentGreenGain(void) const
{
    return m_greenGain;
}


void SettingsBlock::setGreenGain(double value)
{
    m_greenGain = value;
}


double SettingsBlock::currentBlueGain(void) const
{
    return m_blueGain;
}


void SettingsBlock::setBlueGain(double value)
{
    m_blueGain = value;
}


const char *const *SettingsBlock::availableDarkSubOptions(void)
{
    return darkSubOptions;
}


int SettingsBlock::currentDarkSubOption(void) const
{
    return m_darkSubOption;
}


void SettingsBlock::setDarkSubOption(int value)
{
    m_darkSubOption = value;
}


char *SettingsBlock::currentDarkSubDir(void) const
{
    char *dir;

    dir = new char[MAXPATHLEN+1];
    strcpy(dir, m_darkSubDirMRU);
    if (strlen(dir) == 0)
	strcpy(dir, DEFAULT_DARKSUBDIR);
    return dir;
}


char *SettingsBlock::currentDarkSubDirMRU(void) const
{
    char *mru;

    mru = new char[MAXPATHLEN*4+1];
    memcpy(mru, m_darkSubDirMRU, MAXPATHLEN*4+1);
    return mru;
}


void SettingsBlock::insertInDarkSubDirMRU(const char *dir)
{
    insertInMRU(&m_darkSubDirMRU, dir);
}


const char *const *SettingsBlock::availableNavDurations(void)
{
    return navDurations;
}


int SettingsBlock::currentNavDuration(void) const
{
    return m_navDuration;
}


void SettingsBlock::setNavDuration(int value)
{
    m_navDuration = value;
}


int SettingsBlock::currentNavDurationSkip(void) const
{
    return navDurationSkips[m_navDuration];
}


int SettingsBlock::currentNavDurationSecs(void) const
{
    return (navDurationSkips[m_navDuration]+1) * 30;
}


const char *const *SettingsBlock::availableReflectionOptions(void)
{
    return reflectionOptions;
}


int SettingsBlock::currentReflectionOption(void) const
{
    return m_reflectionOption;
}


void SettingsBlock::setReflectionOption(int value)
{
    m_reflectionOption = value;
}


const char *const *SettingsBlock::availableRecMargins(void)
{
    return recMargins;
}


int SettingsBlock::currentRecMargin(void) const
{
    return m_recMargin;
}


void SettingsBlock::setRecMargin(int value)
{
    m_recMargin = value;
}


double SettingsBlock::currentRecMarginPct(void) const
{
    return recMarginPcts[m_recMargin];
}


const char *const *SettingsBlock::availableOBCInterfaces(void)
{
    return obcInterfaces;
}


int SettingsBlock::currentOBCInterface(void) const
{
    return m_obcInterface;
}


void SettingsBlock::setOBCInterface(int value)
{
    m_obcInterface = value;
}


const char *const *SettingsBlock::availableCalPeriods(void)
{
    return calPeriods;
}


int SettingsBlock::currentDark1CalPeriod(void) const
{
    return m_dark1CalPeriod;
}


int SettingsBlock::currentDark1CalPeriodInSecs(void) const
{
    return calPeriodLengths[m_dark1CalPeriod];
}


void SettingsBlock::setDark1CalPeriod(int value)
{
    m_dark1CalPeriod = value;
}


int SettingsBlock::currentDark2CalPeriod(void) const
{
    return m_dark2CalPeriod;
}


int SettingsBlock::currentDark2CalPeriodInSecs(void) const
{
    return calPeriodLengths[m_dark2CalPeriod];
}


void SettingsBlock::setDark2CalPeriod(int value)
{
    m_dark2CalPeriod = value;
}


int SettingsBlock::currentMediumCalPeriod(void) const
{
    return m_mediumCalPeriod;
}


int SettingsBlock::currentMediumCalPeriodInSecs(void) const
{
    return calPeriodLengths[m_mediumCalPeriod];
}


void SettingsBlock::setMediumCalPeriod(int value)
{
    m_mediumCalPeriod = value;
}


int SettingsBlock::currentBrightCalPeriod(void) const
{
    return m_brightCalPeriod;
}


int SettingsBlock::currentBrightCalPeriodInSecs(void) const
{
    return calPeriodLengths[m_brightCalPeriod];
}


void SettingsBlock::setBrightCalPeriod(int value)
{
    m_brightCalPeriod = value;
}


int SettingsBlock::currentLaserCalPeriod(void) const
{
    return m_laserCalPeriod;
}


int SettingsBlock::currentLaserCalPeriodInSecs(void) const
{
    return calPeriodLengths[m_laserCalPeriod];
}


void SettingsBlock::setLaserCalPeriod(int value)
{
    m_laserCalPeriod = value;
}


const char *const *SettingsBlock::availableShutterInterfaces(void)
{
    return shutterInterfaces;
}


int SettingsBlock::currentShutterInterface(void) const
{
    return m_shutterInterface;
}


void SettingsBlock::setShutterInterface(int value)
{
    m_shutterInterface = value;
}


const char *const *SettingsBlock::availableGPSSerialBauds(void)
{
    return gpsSerialBauds;
}


int SettingsBlock::currentGPSSerialBaud(void) const
{
    return m_gpsSerialBaud;
}


void SettingsBlock::setGPSSerialBaud(int value)
{
    m_gpsSerialBaud = value;
}


int SettingsBlock::currentGPSSerialBaudBPS(void) const
{
    return gpsSerialBaudsBPS[m_gpsSerialBaud];
}


const char *const *SettingsBlock::availableGPSSerialParities(void)
{
    return gpsSerialParities;
}


int SettingsBlock::currentGPSSerialParity(void) const
{
    return m_gpsSerialParity;
}


void SettingsBlock::setGPSSerialParity(int value)
{
    m_gpsSerialParity = value;
}


const char *const *SettingsBlock::availableGPSSerialTXInverts(void)
{
    return gpsSerialTXInverts;
}


int SettingsBlock::currentGPSSerialTXInvert(void) const
{
    return m_gpsSerialTXInvert;
}


void SettingsBlock::setGPSSerialTXInvert(int value)
{
    m_gpsSerialTXInvert = value;
}


const char *const *SettingsBlock::availableGPSInitModes(void)
{
    return gpsInitModes;
}


int SettingsBlock::currentGPSInitMode(void) const
{
    return m_gpsInitMode;
}


void SettingsBlock::setGPSInitMode(int value)
{
    m_gpsInitMode = value;
}


const char *const *SettingsBlock::availableECSOptions(void)
{
    return ecsOptions;
}


int SettingsBlock::currentECSConnected(void) const
{
    return m_ecsConnected;
}


void SettingsBlock::setECSConnected(int value)
{
    m_ecsConnected = value;
}


char *SettingsBlock::currentECSHost(void) const
{
    char *host;

    host = new char[strlen(m_ecsHost)+1];
    strcpy(host, m_ecsHost);
    return host;
}


void SettingsBlock::setECSHost(const char *host)
{
    delete[] m_ecsHost;
    m_ecsHost = new char[strlen(host)+1];
    strcpy(m_ecsHost, host);
}


int SettingsBlock::currentECSPort(void) const
{
    return m_ecsPort;
}


void SettingsBlock::setECSPort(int value)
{
    m_ecsPort = value;
}


int SettingsBlock::currentECSTimeoutInMs(void) const
{
    return m_ecsTimeoutInMs;
}


void SettingsBlock::setECSTimeoutInMs(int value)
{
    m_ecsTimeoutInMs = value;
}


char *SettingsBlock::currentECSUsername(void) const
{
    char *username;

    username = new char[strlen(m_ecsUsername)+1];
    strcpy(username, m_ecsUsername);
    return username;
}


void SettingsBlock::setECSUsername(const char *username)
{
    delete[] m_ecsUsername;
    m_ecsUsername = new char[strlen(username)+1];
    strcpy(m_ecsUsername, username);
}


char *SettingsBlock::currentECSPassword(void) const
{
    char *password;

    password = new char[strlen(m_ecsPassword)+1];
    strcpy(password, m_ecsPassword);
    return password;
}


void SettingsBlock::setECSPassword(const char *password)
{
    delete[] m_ecsPassword;
    m_ecsPassword = new char[strlen(password)+1];
    strcpy(m_ecsPassword, password);
}


const char *const *SettingsBlock::availableMountOptions(void)
{
    return mountOptions;
}


int SettingsBlock::currentMountUsed(void) const
{
    return m_mountUsed;
}


void SettingsBlock::setMountUsed(int value)
{
    m_mountUsed = value;
}


char *SettingsBlock::currentMountDev(void) const
{
    char *dev;

    dev = new char[strlen(m_mountDev)+1];
    strcpy(dev, m_mountDev);
    return dev;
}


void SettingsBlock::setMountDev(const char *dev)
{
    delete[] m_mountDev;
    m_mountDev = new char[strlen(dev)+1];
    strcpy(m_mountDev, dev);
}


char *SettingsBlock::currentHeadlessDev(void) const
{
    char *dev;

    dev = new char[strlen(m_headlessDev)+1];
    strcpy(dev, m_headlessDev);
    return dev;
}


void SettingsBlock::setHeadlessDev(const char *dev)
{
    delete[] m_headlessDev;
    m_headlessDev = new char[strlen(dev)+1];
    strcpy(m_headlessDev, dev);
}


const char *const *SettingsBlock::availableImageSims(void)
{
    return imageSims;
}


int SettingsBlock::currentImageSim(void) const
{
    return m_imageSim;
}


void SettingsBlock::setImageSim(int value)
{
    m_imageSim = value;
}


const char *const *SettingsBlock::availableGPSSims(void)
{
    return gpsSims;
}


int SettingsBlock::currentGPSSim(void) const
{
    return m_gpsSim;
}


void SettingsBlock::setGPSSim(int value)
{
    m_gpsSim = value;
}


const char *const *SettingsBlock::availableDiagOptions(void)
{
    return diagOptions;
}


int SettingsBlock::currentDiagOption(void) const
{
    return m_diagOption;
}


void SettingsBlock::setDiagOption(int value)
{
    m_diagOption = value;
}


char *SettingsBlock::currentPrefix(void) const
{
    char *prefix;

    prefix = new char[strlen(m_prefix)+1];
    strcpy(prefix, m_prefix);
    return prefix;
}


void SettingsBlock::setPrefix(const char *prefix)
{
    delete[] m_prefix;
    m_prefix = new char[strlen(prefix)+1];
    strcpy(m_prefix, prefix);
}


char *SettingsBlock::currentProductRootDir(void) const
{
    char *dir;

    dir = new char[MAXPATHLEN+1];
    strcpy(dir, m_productRootDirMRU);
    if (strlen(dir) == 0)
	strcpy(dir, DEFAULT_PRODUCTROOTDIR);
    return dir;
}


char *SettingsBlock::currentProductRootDirMRU(void) const
{
    char *mru;

    mru = new char[MAXPATHLEN*4+1];
    memcpy(mru, m_productRootDirMRU, MAXPATHLEN*4+1);
    return mru;
}


void SettingsBlock::insertInProductRootDirMRU(const char *dir)
{
    insertInMRU(&m_productRootDirMRU, dir);
}


void SettingsBlock::getIntValue(FILE *fp, const char *key, int *value_p,
    bool warnIfMissing, int min, int max) const
{
    char *buf, *p;
    char msg[MAX_NGDCS_LINE_LEN+100];
    int i;

    if (getValue(fp, key, &buf, warnIfMissing) != -1) {
	i = strtol(buf, &p, 0);
	if (*p != '\0') {
	    sprintf(msg,
		"Saved-settings file (.ngdcs) looks corrupted "
		    "at key \"%s\".\n",
		key);
	    if (m_headless) {
		(void)fputs(msg, stderr);
		(void)fputc('\n', stderr);
	    }
	    else {
		Gtk::MessageDialog d(msg, false /* no markup */,
		    Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK);
		d.set_secondary_text("This message is unloggable.");
		(void)d.run();
	    }
	}
	else {
	    if (i < min) i = min;
	    else if (i > max) i = max;
	    *value_p = i;
	}
	delete[] buf;
    }
}


void SettingsBlock::getDoubleValue(FILE *fp, const char *key, double *value_p,
    bool warnIfMissing, double min, double max) const
{
    char *buf, *p;
    char msg[MAX_NGDCS_LINE_LEN+100];
    double f;

    if (getValue(fp, key, &buf, warnIfMissing) != -1) {
	f = strtod(buf, &p);
	if (*p != '\0') {
	    sprintf(msg,
		"Saved-settings file (.ngdcs) looks corrupted "
		    "at key \"%s\".\n",
		key);
	    if (m_headless) {
		(void)fputs(msg, stderr);
		(void)fputc('\n', stderr);
	    }
	    else {
		Gtk::MessageDialog d(msg, false /* no markup */,
		    Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK);
		d.set_secondary_text("This message is unloggable.");
		(void)d.run();
	    }
	}
	else {
	    if (f < min) f = min;
	    else if (f > max) f = max;
	    *value_p = f;
	}
	delete[] buf;
    }
}


void SettingsBlock::getIndex(FILE *fp, const char *key, bool warnIfMissing,
    const char *const strings[], int *setting_p, const char *description) const
{
    char *buf;

    if (getValue(fp, key, &buf, warnIfMissing) != -1) {
	stringToIndex(buf, strings, setting_p, description);
	delete[] buf;
    }
}


void SettingsBlock::stringToIndex(const char *string,
    const char *const strings[], int *setting_p, const char *description) const
{
    int i;
    char error[80];

    for (i=0;strings[i] != NULL;i++) {
	if (!strcmp(string, strings[i])) {
	    *setting_p = i;
	    break;
	}
    }
    if (strings[i] == NULL) {
	sprintf(error, "Internal error: Can't determine new %s; using %s",
	    description, strings[0]);
	if (m_headless) {
	    (void)fputs(error, stderr);
	    (void)fputc('\n', stderr);
	}
	else {
	    Gtk::MessageDialog d( error, false /* no markup */,
		Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK);
	    d.set_secondary_text("This message is unloggable.");
	    (void)d.run();
	}
	*setting_p = 0;
    }
}


void SettingsBlock::getMRU(FILE *fp, const char *base, char *MRU) const
{
    char *buf, *key;
    int i;

    key = new char[strlen(base)+20];
    for (i=0;i < 4;i++) {
	(void)sprintf(key, "%s%d", base, i);
	if (getValue(fp, key, &buf, true) != -1) {
	    (void)strcpy(MRU+i*MAXPATHLEN, buf);
	    delete[] buf;
	}
    }
    delete[] key;
}


void SettingsBlock::insertInMRU(char **mru_p, const char *newbuf) const
{
    char *from, *from_limit, *to, *newMRU;
    int i;

	/* allocate space for a new MRU and put our selection at the front */

    newMRU = new char[MAXPATHLEN*4+1];
    strcpy(newMRU, newbuf);

	/* set pointers to the next slot in our new MRU, and the first slot
	   in the old MRU */

    to = newMRU + MAXPATHLEN;
    from = *mru_p;

	/* note the last entry of the old MRU */

    from_limit = *mru_p + 3 * MAXPATHLEN;

	/* for each entry we still have to fill in... */

    for (i=1;i < 4;i++) {

	    /* advance past any names in the old MRU that match the current
	       name.  this can happen if an old name was selected to be the
	       new name */

	while (strcmp(from, newbuf) == 0 && from <= from_limit)
	    from += MAXPATHLEN;

	    /* if we found something, add it to the new MRU and advance both
	       pointers */

	if (from <= from_limit) {
	    strcpy(to, from);
	    to += MAXPATHLEN;
	    from += MAXPATHLEN;
	}

	    /* otherwise just make the slot in the new MRU blank */

	else {
	    *to = '\0';
	    to += MAXPATHLEN;
	}
    }

	/* we're done -- replace the old MRU with the new */

    delete[] *mru_p;
    *mru_p = newMRU;
}


int SettingsBlock::getValue(FILE *fp, const char *key, char **value_p,
    bool warnIfMissing) const
{
    char buf[MAX_NGDCS_LINE_LEN+1], *p, *q;
    char msg[MAX_NGDCS_LINE_LEN+100];

	/* point to the beginning of the file */

    fseek(fp, 0L, SEEK_SET);

	/* for every line in the file, or until we find what we want... */

    while (fgets(buf, sizeof(buf), fp) != NULL) {

	    /* if the current line matches our key... */

	if (strncmp(buf, key, strlen(key)) == 0) {

		/* set pointer to following position */

	    p = buf + strlen(key);

		/* if we've got the delimiter where we expect it... */

	    if (strncmp(p, " = ", 3) == 0) {

		    /* allocate space for the value */

		*value_p = new char[MAX_NGDCS_LINE_LEN+1];

		    /* save the value for the caller and return,
		       making sure we remove the trailing newline */

		p += 3;
		q = *value_p;
		while (*p != '\n')
		    *q++ = *p++;
		*q = '\0';
		return 0;
	    }
	}
    }

	/* we didn't find our key -- it might have just been added, so we'll
           return an error so the caller can use the default */

    if (warnIfMissing) {
	sprintf(msg,
	    "Didn't find key \"%s\" in saved settings (.ngdcs).\n"
		"Using default value.",
	    key);
	if (m_headless) {
	    (void)fputs(msg, stderr);
	    (void)fputc('\n', stderr);
	}
	else {
	    Gtk::MessageDialog d(msg, false /* no markup */,
		Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK);
	    d.set_secondary_text("This message is unloggable.");
	    (void)d.run();
	}
    }
    return -1;
}
