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


#include <semaphore.h>
#include <adcommon.h>
#include <adclink.h>
#include <adframeserver.h>
#include <adframeclient.h>
#include "/home/avirisng/NGDCS/AlphaData/adframeserver/apps/source/jplclink_console/cameracard_jpl_adb3.h"

using namespace AlphaData::FrameServer;
using namespace AlphaData::FrameClient;

#define FB_PIXEL_WIDTH	640

#define FB_SERIAL_BAUD	9600

#define SENSOR_NOMINAL	0
#define SENSOR_WARM	1
#define SENSOR_HOT	2


class FrameBuffer
{
protected:
    int m_frameHeight;
    void (*m_errorReporter)(const char *string);
    int m_compressionCapable;
public:
    int failed;
    char last_error[1024];
    FrameBuffer(int fh, void (*er)(const char *), int cc) {
	m_frameHeight = fh; m_errorReporter = er; m_compressionCapable = cc; }
    virtual int frameIsAvailable(void) = 0;
    virtual void *getFrame(void) = 0;
    virtual void getStats(u_int *dropped_p, u_int *backlog_p) = 0;
    virtual void setCCLines(int cc1, int cc2) = 0;
    virtual void setGPIO(u_int mask, u_int bits) = 0;
    virtual void setExtSerialBaud(int baud) = 0;
    virtual void setExtSerialParityEven(void) = 0;
    virtual void setExtSerialParityOdd(void) = 0;
    virtual void setExtSerialParityNone(void) = 0;
    virtual void setExtSerialTXInvertOff(void) = 0;
    virtual void setExtSerialTXInvertOn(void) = 0;
    virtual int writeExtSerial(u_char *bytes, int numBytes) = 0;
    virtual void writeCLSerial(char *string) = 0;
    virtual void readCLSerial(char *buffer, int maxChars) = 0;
    virtual int fpgaVersion(void) = 0;
    virtual int numFrameRates(void) = 0;
    virtual double getFrameRateHz(void) = 0;
    virtual double getFrameRateHz(int selection) = 0;
    virtual int setFrameRate(int selection) = 0;
    virtual void readSensors(int *numSensors_p, char ***sensorNames_p,
	double **sensorValues_p, int **sensorRanges_p) = 0;
    virtual u_int fpgaPPSCount(void) = 0;
    virtual void readFPGARegs(int *m_fpgaFrameCount_p,
	int *m_fpgaPPSCount_p, int *m_fpga81ffCount_p,
	int *m_fpgaFramesDropped_p, int *m_serialErrors_p,
	int *m_serialPortCtl_p) = 0;
    virtual void clearSerialErrors(void) = 0;
    virtual ~FrameBuffer() { }
};


using namespace AlphaData::CLink;

#define NUM_IMAGE_BUFFERS	200
#define NUM_FRAMERATES		7

#define ADFB_BITFILE_PATH		"/usr/local/lib"

#define ADFB_FPGA_GPIO_ADDR		0xC0000
#define ADFB_FPGA_FRAME_COUNT_ADDR	0xC0006
#define ADFB_FPGA_PPS_COUNT_ADDR	0xC0008
#define ADFB_FPGA_81FF_COUNT_ADDR	0xC000A
#define ADFB_FPGA_FRAMES_DROPPED_ADDR	0xC000C

#define ADFB_FPGA_SERIAL_CONFIG_REG	0xE0000
#define ADFB_FPGA_SERIAL_CONFIG_ODD	0xC78000
#define ADFB_FPGA_SERIAL_RESET_MASK	0xFFFF7FFF
#define ADFB_FPGA_SERIAL_PARITY_MASK	0xFF3FFFFF
#define ADFB_FPGA_SERIAL_PARITY_ODD	(3 << 22)
#define ADFB_FPGA_SERIAL_PARITY_EVEN	(2 << 22)
#define ADFB_FPGA_SERIAL_PARITY_NONE	(0 << 22)
#define ADFB_FPGA_SERIAL_TXINVERT_MASK	0xFFFDFFFF
#define ADFB_FPGA_SERIAL_TXINVERT_OFF	(1 << 17)
#define ADFB_FPGA_SERIAL_TXINVERT_ON	(0 << 17)

#define ADFB_FPGA_SERIAL_DIVIDER_REG	0xE0001
#define ADFB_FPGA_SERIAL_DIVIDER_38400	0x144
#define ADFB_FPGA_SERIAL_DIVIDER_115200	0x6B

#define ADFB_FPGA_SERIAL_TXFIFO_REG	0xE0003

#define ADFB_FPGA_SERIAL_TXSTATUS_REG	0xE0005

#define ADFB_FPGA_SERIAL_ERRORS_REG	0xE0007

#define ADFB_FPGA_VERSION_FROM_C0000(x)	(((x) >> 16) & 0xff)

#define BASELINE_HEIGHT_LINES		481

    /* define max buffer height for reading multiple frames as single image.
       this seems to prevent dropped frames observed with PRISM and once in
       a while with AVIRISng */

#define MAX_BUFFER_HEIGHT		1500

class AlphaDataFrameBuffer : public FrameBuffer
{
protected:
    ACameraCard *m_card;
    ACamera *m_camera;
    unsigned int m_cameraIndex;
    unsigned int m_cardIndex;
    bool m_appRunning;
    CEvent m_alarmEvent;
    const char *m_bitDir;
    CThread *m_alarmThread;
    CFrameServer *m_frameServer;
    CFrameServerClient m_clientHandle;
    FrmSrvrCmPtrT m_serverHandle;

    int m_nextFreeImageBuffer;
    int m_nextPopulatedImageBuffer;
    int m_readyImageBuffers;
    void *m_imageBuffers[NUM_IMAGE_BUFFERS];
    sem_t m_fetchSem;
    CThread *m_fetchThread;

    int m_numSensors;
    ADMXRC3_HANDLE m_cardHandle;
    ADMXRC3_CARD_INFOEX m_cardInfo;
    int *m_sensorIndices;
    char **m_sensorNames;
    double *m_sensorValues;
    int *m_sensorRanges;

    int m_currentFrameRate;

    int m_framesPerBuffer;
    void *m_currentImageBuffer;
    int m_framesLeftInBuffer;

    u_int m_serialPortCtlReg;

    static double baselineFrameRatesInHz[NUM_FRAMERATES];
    double availableFrameRatesInHz[NUM_FRAMERATES];
    static char frameRateCodes[NUM_FRAMERATES];

    static void AlarmThread(void *pArg);
    static void FetchThread(void *pArg);
public:
    AlphaDataFrameBuffer(int fh, void (*er)(const char *), int cc);
    ~AlphaDataFrameBuffer();
    int frameIsAvailable(void);
    void *getFrame(void);
    void getStats(u_int *dropped_p, u_int *backlog_p);
    void setCCLines(int cc1, int cc2);
    void setGPIO(u_int mask, u_int bits);
    void setExtSerialBaud(int baud);
    void setExtSerialParityEven(void);
    void setExtSerialParityOdd(void);
    void setExtSerialParityNone(void);
    void setExtSerialTXInvertOff(void);
    void setExtSerialTXInvertOn(void);
    int writeExtSerial(u_char *bytes, int numBytes);
    void writeCLSerial(char *string);
    void readCLSerial(char *buffer, int maxChars);
    int fpgaVersion(void);
    int numFrameRates(void) { return NUM_FRAMERATES; }
    double getFrameRateHz(void);
    double getFrameRateHz(int selection);
    int setFrameRate(int selection);
    void readSensors(int *numSensors_p, char ***sensorNames_p,
	double **sensorValues_p, int **sensorRanges_p);
    u_int fpgaPPSCount(void);
    void readFPGARegs(int *fpgaFrameCount_p, int *fpgaPPSCount_p,
	int *fpga81ffCount_p, int *fpgaFramesDropped_p, int *serialErrors_p,
	int *serialPortCtl_p);
    void clearSerialErrors(void);
};

class CompressionCam: public ACamera
{
public:
    CompressionCam(ACameraCard *pCameraCard, UINT32 nCameraIndex) :
	    ACamera(pCameraCard, nCameraIndex) {
        SetPixelFormat(AlphaData::CLink::DATA_FMT_L16);
        SetCLinkFormat(BASE_1X16BITS);
        SetTotalPixels(640);
        SetTotalLines(481);
        SetInvalidPixels(0x0000);
        SetInvalidLines(0x0000);
        SetSamplePixels(640);
        SetSampleLines(481);
        SetPixelOffset(0x0000);
        SetLineOffset(0x0000);
        SetHasSerial(true);
        SetDVALOptional(true);
        SetFramesInOne(32);
    }
    virtual ~CompressionCam() { }
};
