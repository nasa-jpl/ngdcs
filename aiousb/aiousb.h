/**
 * @file   aiousb.h
 * @author  Jimi Damon <jdamon@accesio.com>
 * @date    Sun Aug 11 22:29:47 2013 -0700
 * @release  725402a
 * @brief  General header files for the AIOUSB library
 *
 */

#if !defined( aiousb_h )
#define aiousb_h

#include <stdlib.h>



#ifdef __aiousb_cplusplus
namespace AIOUSB
{
#endif

#define CREATE_ENUM(name, ...) typedef enum { name ## _begin, __VA_ARGS__, name ## _end } name;
#define CREATE_ENUM_W_START(name,num, ...) typedef enum { name ## _begin = (num-1), __VA_ARGS__, name ## _end } name;
#define LAST_ENUM(name ) (name ## _end-1 )
#define FIRST_ENUM(name ) (name ## _begin+1)
#define MIN_VALUE(name ) (name ## _begin+1)
#define MAX_VALUE(name ) (name ## _end-1)
#define VALID_ENUM(name,value ) ( value >= FIRST_ENUM(name) && value <= LAST_ENUM(name ))

#define VALID_PRODUCT(product) ( VALID_ENUM(  ProductIDS, product ) )

typedef int AIORET_TYPE;        /* New return type is signed, negative indicates error */

 CREATE_ENUM_W_START(ProductIDS,0,
                     ACCES_VENDOR_ID    = 0x1605,
                     /**
                      * these product IDs are constant
                      *
                      */
                     USB_DA12_8A_REV_A  = 0x4001,
                     USB_DA12_8A        = 0x4002,
                     USB_DA12_8E        = 0x4003,
                     /**
                      * these are the product IDs after firmware is uploaded to the device; prior
                      * to uploading the firmware, the product ID is that shown below minus 0x8000
                      */
                     USB_DIO_32         = 0x8001,
                     USB_DIO_48         = 0x8002,
                     USB_DIO_96         = 0x8003,
                     USB_DI16A_REV_A1   = 0x8008,
                     USB_DO16A_REV_A1   = 0x8009,
                     USB_DI16A_REV_A2   = 0x800a,
                     USB_DIO_16H        = 0x800c,
                     USB_DI16A          = 0x800d,
                     USB_DO16A          = 0x800e,
                     USB_DIO_16A        = 0x800f,

                     USB_IIRO_16        = 0x8010,
                     USB_II_16          = 0x8011,
                     USB_RO_16          = 0x8012,
                     USB_IIRO_8         = 0x8014,
                     USB_II_8           = 0x8015,
                     USB_IIRO_4         = 0x8016,
                     USB_IDIO_16        = 0x8018,
                     USB_II_16_OLD      = 0x8019,
                     USB_IDO_16         = 0x801a,
                     USB_IDIO_8         = 0x801c,
                     USB_II_8_OLD       = 0x801d,
                     USB_IDIO_4         = 0x801e,
                     USB_CTR_15         = 0x8020,
                     USB_IIRO4_2SM      = 0x8030,
                     USB_IIRO4_COM      = 0x8031,
                     USB_DIO16RO8       = 0x8032,
                     USB_DIO48DO24      = 0x803C,
                     USB_DIO24DO12      = 0x803D,
                     USB_DO24           = 0x803E,
                     PICO_DIO16RO8      = 0x8033,
                     USB_AI16_16A       = 0x8040,
                     USB_AI16_16E       = 0x8041,
                     USB_AI12_16A       = 0x8042,
                     USB_AI12_16        = 0x8043,
                     USB_AI12_16E       = 0x8044,
                     USB_AI16_64MA      = 0x8045,
                     USB_AI16_64ME      = 0x8046,
                     USB_AI12_64MA      = 0x8047,
                     USB_AI12_64M       = 0x8048,
                     USB_AI12_64ME      = 0x8049,
                     USB_AI16_32A       = 0x804a,
                     USB_AI16_32E       = 0x804b,
                     USB_AI12_32A       = 0x804c,
                     USB_AI12_32        = 0x804d,
                     USB_AI12_32E       = 0x804e,
                     USB_AI16_64A       = 0x804f,
                     USB_AI16_64E       = 0x8050,
                     USB_AI12_64A       = 0x8051,
                     USB_AI12_64        = 0x8052,
                     USB_AI12_64E       = 0x8053,
                     USB_AI16_96A       = 0x8054,
                     USB_AI16_96E       = 0x8055,
                     USB_AI12_96A       = 0x8056,
                     USB_AI12_96        = 0x8057,
                     USB_AI12_96E       = 0x8058,
                     USB_AI16_128A      = 0x8059,
                     USB_AI16_128E      = 0x805a,
                     USB_AI12_128A      = 0x805b,
                     USB_AI12_128       = 0x805c,
                     USB_AI12_128E      = 0x805d,
                     USB_AO16_16A       = 0x8070,
                     USB_AO16_16        = 0x8071,
                     USB_AO16_12A       = 0x8072,
                     USB_AO16_12        = 0x8073,
                     USB_AO16_8A        = 0x8074,
                     USB_AO16_8         = 0x8075,
                     USB_AO16_4A        = 0x8076,
                     USB_AO16_4         = 0x8077,
                     USB_AO12_16A       = 0x8078,
                     USB_AO12_16        = 0x8079,
                     USB_AO12_12A       = 0x807a,
                     USB_AO12_12        = 0x807b,
                     USB_AO12_8A        = 0x807c,
                     USB_AO12_8         = 0x807d,
                     USB_AO12_4A        = 0x807e,
                     USB_AO12_4         = 0x807f,
                     USB_AIO16_16A      = 0x8140,
                     USB_AIO16_16E      = 0x8141,
                     USB_AIO12_16A      = 0x8142,
                     USB_AIO12_16       = 0x8143,
                     USB_AIO12_16E      = 0x8144,
                     USB_AIO16_64MA     = 0x8145,
                     USB_AIO16_64ME     = 0x8146,
                     USB_AIO12_64MA     = 0x8147,
                     USB_AIO12_64M      = 0x8148,
                     USB_AIO12_64ME     = 0x8149,
                     USB_AIO16_32A      = 0x814a,
                     USB_AIO16_32E      = 0x814b,
                     USB_AIO12_32A      = 0x814c,
                     USB_AIO12_32       = 0x814d,
                     USB_AIO12_32E      = 0x814e,
                     USB_AIO16_64A      = 0x814f,
                     USB_AIO16_64E      = 0x8150,
                     USB_AIO12_64A      = 0x8151,
                     USB_AIO12_64       = 0x8152,
                     USB_AIO12_64E      = 0x8153,
                     USB_AIO16_96A      = 0x8154,
                     USB_AIO16_96E      = 0x8155,
                     USB_AIO12_96A      = 0x8156,
                     USB_AIO12_96       = 0x8157,
                     USB_AIO12_96E      = 0x8158,
                     USB_AIO16_128A     = 0x8159,
                     USB_AIO16_128E     = 0x815a,
                     USB_AIO12_128A     = 0x815b,
                     USB_AIO12_128      = 0x815c,
                     USB_AIO12_128E     = 0x815d
                     );




/*
 * generic device indexes
 */
enum {
    /* passed to ResolveDeviceIndex() */
    diFirst                       = 0xFFFFFFFEul,      /* -2 */
    diOnly                        = 0xFFFFFFFDul,      /* -3 */

    /* returned by ResolveDeviceIndex() */
    diNone                        = 0xFFFFFFFFul       /* -1 */
};

/*
 * range codes passed to DACSetBoardRange()
 */
CREATE_ENUM_W_START(DACRange,0,
                    DAC_RANGE_0_5V,
                    DAC_RANGE_5V,
                    DAC_RANGE_0_10V,
                    DAC_RANGE_10V
                    );



/*
 * FIFO clearing methods passed to AIOUSB_ClearFIFO()
 */
CREATE_ENUM_W_START(FIFO_Method,0,
                    CLEAR_FIFO_METHOD_IMMEDIATE,
                    CLEAR_FIFO_METHOD_AUTO,
                    CLEAR_FIFO_METHOD_IMMEDIATE_AND_ABORT = 5,
                    CLEAR_FIFO_METHOD_WAIT = 86
                    )



/**
 * The AIOUSB function result codes are a bit confusing; the result codes used in the Windows
 * implementation of the API are defined in a system file, winerror.h; these result codes
 * are generic and can apply to many applications; the very first result code, ERROR_SUCCESS,
 * sounds like an oxymoron; the result codes used in libusb, on the other hand, are a lot
 * more appealing; the result code for success is LIBUSB_SUCCESS; the result codes for errors
 * are LIBUSB_ERROR_xxx; further complicating matters is that the AIOUSB result codes must be
 * non-negative since all the functions return an unsigned result, whereas the LIBUSB result
 * codes are negative in the case of errors; both schemes use zero to denote success; it would
 * also be nice to return the original libusb result code in cases where a libusb error causes
 * an AIOUSB API function to fail; so to satisfy all these requirements, we've employed the
 * following scheme:
 *
 * - AIOUSB result codes in Linux start with "AIOUSB_"; the result code for success is
 *   AIOUSB_SUCCESS, which has a value of zero; the result codes for errors are AIOUSB_ERROR_xxx,
 *   which have positive values, starting with one (1)
 *
 * - in order to offer users the option of using result codes whose names are similar to those
 *   cited in the AIOUSB API specification, we define a second set of result codes with names
 *   similar to those in API specification but which map to the same values as the AIOUSB_xxx
 *   result codes; these alternate result code names can be enabled by defining the macro
 *   ENABLE_WINDOWS_RESULT_CODES, which is not enabled by default
 *
 * - in order to preserve the original libusb result codes and pass them back from an AIOUSB API
 *   function, we translate the libusb result codes to a format that conforms to the one
 *   AIOUSB employs and provide macros for converting the AIOUSB result code back to a libusb
 *   result code; LIBUSB_RESULT_TO_AIOUSB_RESULT() converts a libusb result code to an AIOUSB
 *   result code; AIOUSB_RESULT_TO_LIBUSB_RESULT() does the reverse; these macros cannot be
 *   used with LIBUSB_SUCCESS
 *
 * - we provide an extended AIOUSB API function named AIOUSB_GetResultCodeAsString() that returns a
 *   string representation of the result code, including those derived from a libusb result code
 */
CREATE_ENUM_W_START( ResultCode, 0,
                     AIOUSB_SUCCESS,
                     AIOUSB_ERROR_DEVICE_NOT_CONNECTED,
                     AIOUSB_ERROR_DUP_NAME,
                     AIOUSB_ERROR_FILE_NOT_FOUND,
                     AIOUSB_ERROR_INVALID_DATA,
                     AIOUSB_ERROR_INVALID_INDEX,
                     AIOUSB_ERROR_INVALID_MUTEX,
                     AIOUSB_ERROR_INVALID_PARAMETER,
                     AIOUSB_ERROR_INVALID_THREAD,
                     AIOUSB_ERROR_NOT_ENOUGH_MEMORY,
                     AIOUSB_ERROR_NOT_SUPPORTED,
                     AIOUSB_ERROR_OPEN_FAILED,
                     AIOUSB_ERROR_BAD_TOKEN_TYPE,
                     AIOUSB_ERROR_TIMEOUT,
                     AIOUSB_ERROR_HANDLE_EOF,
                     AIOUSB_ERROR_DEVICE_NOT_FOUND,
                     AIOUSB_ERROR_INVALID_ADCONFIG_SETTING,
                     AIOUSB_ERROR_INVALID_ADCONFIG_CAL_SETTING,
                     AIOUSB_ERROR_INVALID_ADCONFIG_CHANNEL_SETTING,
                     AIOUSB_ERROR_LIBUSB /* Always make the LIBUSB the last element */
                     );

#define AIOUSB_ERROR_OFFSET 100


#define LIBUSB_RESULT_TO_AIOUSB_RESULT( code )  ( unsigned long )( AIOUSB_ERROR_OFFSET + -( int )( code ) )
#define AIOUSB_RESULT_TO_LIBUSB_RESULT( code )  ( -( ( int )( code ) - AIOUSB_ERROR_OFFSET ) )


#if defined( ENABLE_WINDOWS_RESULT_CODES )
typedef enum {
    ERROR_SUCCESS = AIOUSB_SUCCESS,
    ERROR_DEVICE_NOT_CONNECTED = AIOUSB_ERROR_DEVICE_NOT_CONNECTED,
    ERROR_DUP_NAME = AIOUSB_ERROR_DUP_NAME,
    ERROR_FILE_NOT_FOUND = AIOUSB_ERROR_FILE_NOT_FOUND,
    ERROR_INVALID_DATA = AIOUSB_ERROR_INVALID_DATA,
    ERROR_INVALID_INDEX = AIOUSB_ERROR_INVALID_INDEX,
    ERROR_INVALID_MUTEX = AIOUSB_ERROR_INVALID_MUTEX,
    ERROR_INVALID_PARAMETER = AIOUSB_ERROR_INVALID_PARAMETER,
    ERROR_INVALID_THREAD = AIOUSB_ERROR_INVALID_THREAD,
    ERROR_NOT_ENOUGH_MEMORY = AIOUSB_ERROR_NOT_ENOUGH_MEMORY,
    ERROR_NOT_SUPPORTED = AIOUSB_ERROR_NOT_SUPPORTED,
    ERROR_OPEN_FAILED = AIOUSB_ERROR_OPEN_FAILED,
    ERROR_TIMEOUT = AIOUSB_ERROR_TIMEOUT
}  WindowsResultCode;
#endif


CREATE_ENUM_W_START( ADRegister, 16,
                     AD_REGISTER_CAL_MODE,
                     AD_REGISTER_TRIG_COUNT,
                     AD_REGISTER_START_END,
                     AD_REGISTER_OVERSAMPLE,
                     AD_REGISTER_MUX_START_END
                     );


/*
 * general constants
 */
enum {
    AD_MAX_CONFIG_REGISTERS             = 21, /* maximum number of "registers" (bytes) in A/D config. block */
    AD_NUM_GAIN_CODE_REGISTERS          = 16, /* number of gain code registers in A/D config. block */

    /* A/D configuration block indexes */
    AD_CONFIG_GAIN_CODE                 = 0,  /* gain code for A/D channel 0 (one of GAIN_CODE_* settings below) */
                                              /* gain codes for channels 1-15 occupy configuration block indexes 1-15 */
    AD_REGISTER_GAIN_CODE               = 0,

    AD_CONFIG_CAL_MODE                  = 16, /* calibration mode (one of AD_CAL_MODE_* settings below) */
    AD_CONFIG_TRIG_COUNT                = 17, /* trigger and counter clock (one of AD_TRIG_* settings below) */
    AD_CONFIG_START_END                 = 18, /* start and end channels for scan (bits 7-4 contain end channel, bits 3-0 contain start channel) */
    AD_CONFIG_OVERSAMPLE                = 19, /* oversample setting (0-255 samples in addition to single sample) */
    AD_CONFIG_MUX_START_END             = 20, /* MUX start and end channels for scan (bits 7-4 contain end channel MS-nibble, bits 3-0 contain start channel MS-nibble) */

    /* A/D gain codes */
    AD_NUM_GAIN_CODES                   = 8,
    AD_DIFFERENTIAL_MODE                = 8, /* OR with gain code to enable differential mode for channels 0-7 */

    /* A/D trigger and counter bits */
    AD_TRIGGER_CTR0_EXT                 = 0x10, /* 1==counter 0 externally triggered */
    AD_TRIGGER_FALLING_EDGE             = 0x08, /* 1==triggered by falling edge */
    AD_TRIGGER_SCAN                     = 0x04, /* 1==scan all channels, 0==single channel */

    /* mutually exclusive for now */
    AD_TRIGGER_EXTERNAL                 = 0x02,              /* 1==triggered by external pin on board */
    AD_TRIGGER_TIMER                    = 0x01,              /* 1==triggered by counter 2 */

    AD_TRIGGER_VALID_MASK               = ( AD_TRIGGER_CTR0_EXT
                                            | AD_TRIGGER_FALLING_EDGE
                                            | AD_TRIGGER_SCAN
                                            | AD_TRIGGER_EXTERNAL
                                            | AD_TRIGGER_TIMER )
};

/*
 * A/D gain codes
 */
CREATE_ENUM_W_START( ADGainCode, 0,
                     AD_GAIN_CODE_0_10V,    /* 0-10V */
                     AD_GAIN_CODE_10V,      /* +/-10V */
                     AD_GAIN_CODE_0_5V,     /* 0-5V */
                     AD_GAIN_CODE_5V,       /* +/-5V */
                     AD_GAIN_CODE_0_2V,     /* 0-2V */
                     AD_GAIN_CODE_2V,       /* +/-2V */
                     AD_GAIN_CODE_0_1V,     /* 0-1V */
                     AD_GAIN_CODE_1V        /* +/-1V */
                     )

/*
 * A/D calibration modes; if ground or reference mode is selected, only one A/D
 * sample may be taken at a time; that means, one channel and no oversampling
 * attempting to read more than one channel or use an oversample setting of more
 * than zero will result in a timeout error
 */
typedef enum  {
  ADCalMode_begin         = -1,
  AD_CAL_MODE_NORMAL      = 0, /* normal measurement */
  AD_CAL_MODE_GROUND      = 1, /* measure ground */
  AD_CAL_MODE_REFERENCE   = 3, /* measure reference */
  AD_CAL_MODE_BIP_GROUND  = 5,
  ADCalMode_end           = 6
} ADCalMode;

typedef struct  {
    const void *device;
    unsigned long size;
    unsigned char registers[ AD_MAX_CONFIG_REGISTERS ];
} ADConfigBlock;


/** 
 * @desc Allows us to keep track of streaming (bulk) acquires
 * without making the user keep track of the memory management
 */

typedef struct {
  int bufsize;
  unsigned short *buffer;
  unsigned long bytes_remaining;
} AIOBuf ;



/* #ifndef __aiousb_cplusplus */
/* typedef struct ADConfigBlock ADConfigBlock; */
/* #endif */





typedef struct  {
    const char *Name;                /* null-terminated device name or 0 */
    __uint64_t SerialNumber;         /* 64-bit serial number or 0 */
    unsigned ProductID;              /* 16-bit product ID */
    unsigned DIOPorts;               /* number of digital I/O ports (bytes) */
    unsigned Counters;               /* number of 8254 counter blocks */
    unsigned Tristates;              /* number of tristates */
    long RootClock;                  /* base clock frequency */
    unsigned DACChannels;            /* number of D/A channels */
    unsigned ADCChannels;            /* number of A/D channels */
    unsigned ADCMUXChannels;         /* number of MUXed A/D channels */
    unsigned ADCChannelsPerGroup;    /* number of A/D channels in each config. group */
} DeviceProperties;
/* #ifndef __aiousb_cplusplus */
/* typedef struct DeviceProperties DeviceProperties; */
/* #endif */

/*
 * other libraries often declare BOOL, TRUE and FALSE, and worse, they declare these
 * using #defines; so we sidestep that potential conflict by declaring the same types
 * prefixed with AIOUSB_; it's ugly, but if people want to use the shorter names and
 * they are certain they won't conflict with anything else, they can define the
 * ENABLE_BOOL_TYPE macro
 */
#ifdef __cplusplus
typedef bool AIOUSB_BOOL;
const bool AIOUSB_FALSE = false;
const bool AIOUSB_TRUE = true;
#if defined( ENABLE_BOOL_TYPE )
typedef bool BOOL;
const bool FALSE = false;
const bool TRUE = true;
#endif
#else
enum AIOUSB_BOOL_VAL {
    AIOUSB_FALSE                        = 0,
    AIOUSB_TRUE                         = 1
};     /* enum AIOUSB_BOOL */
typedef enum AIOUSB_BOOL_VAL AIOUSB_BOOL;
#if defined( ENABLE_BOOL_TYPE )
enum BOOL {
    FALSE                               = 0,
    TRUE                                = 1
};     /* enum BOOL */
typedef enum BOOL BOOL;
#endif
#endif





extern unsigned long GetDevices( void );

extern unsigned long QueryDeviceInfo(
    unsigned long DeviceIndex,
    unsigned long *pPID,
    unsigned long *pNameSize,
    char *pName,
    unsigned long *pDIOBytes,
    unsigned long *pCounters );

extern unsigned long ClearDevices( void );

extern unsigned long ResolveDeviceIndex( unsigned long DeviceIndex );

extern unsigned long DIO_Configure(
    unsigned long DeviceIndex,
    unsigned char bTristate,
    void *pOutMask,
    void *pData );

extern unsigned long DIO_ConfigureEx(
    unsigned long DeviceIndex,
    void *pOutMask,
    void *pData,
    void *pTristateMask );

extern unsigned long DIO_ConfigurationQuery(
    unsigned long DeviceIndex,
    void *pOutMask,
    void *pTristateMask );

extern unsigned long DIO_WriteAll(
    unsigned long DeviceIndex,
    void *pData );

extern unsigned long DIO_Write8(
    unsigned long DeviceIndex,
    unsigned long ByteIndex,
    unsigned char Data );

extern unsigned long DIO_Write1(
    unsigned long DeviceIndex,
    unsigned long BitIndex,
    unsigned char bData );

extern unsigned long DIO_ReadAll(
    unsigned long DeviceIndex,
    void *Buffer );

extern unsigned long DIO_Read8(
    unsigned long DeviceIndex,
    unsigned long ByteIndex,
    unsigned char *pBuffer );

extern unsigned long DIO_Read1(
    unsigned long DeviceIndex,
    unsigned long BitIndex,
    unsigned char *pBuffer );

extern unsigned long DIO_StreamOpen(
    unsigned long DeviceIndex,
    unsigned long bIsRead );

extern unsigned long DIO_StreamClose(
    unsigned long DeviceIndex );

extern unsigned long DIO_StreamSetClocks(
    unsigned long DeviceIndex,
    double *ReadClockHz,
    double *WriteClockHz );

extern unsigned long DIO_StreamFrame(
    unsigned long DeviceIndex,
    unsigned long FramePoints,
    unsigned short *pFrameData,
    unsigned long *BytesTransferred );

extern unsigned long CTR_8254Mode(
    unsigned long DeviceIndex,
    unsigned long BlockIndex,
    unsigned long CounterIndex,
    unsigned long Mode );

extern unsigned long CTR_8254Load(
    unsigned long DeviceIndex,
    unsigned long BlockIndex,
    unsigned long CounterIndex,
    unsigned short LoadValue );

extern unsigned long CTR_8254ModeLoad(
    unsigned long DeviceIndex,
    unsigned long BlockIndex,
    unsigned long CounterIndex,
    unsigned long Mode,
    unsigned short LoadValue );

extern unsigned long CTR_8254ReadModeLoad(
    unsigned long DeviceIndex,
    unsigned long BlockIndex,
    unsigned long CounterIndex,
    unsigned long Mode,
    unsigned short LoadValue,
    unsigned short *pReadValue );

extern unsigned long CTR_8254Read(
    unsigned long DeviceIndex,
    unsigned long BlockIndex,
    unsigned long CounterIndex,
    unsigned short *pReadValue );

extern unsigned long CTR_8254ReadAll(
    unsigned long DeviceIndex,
    unsigned short *pData );

extern unsigned long CTR_8254ReadStatus(
    unsigned long DeviceIndex,
    unsigned long BlockIndex,
    unsigned long CounterIndex,
    unsigned short *pReadValue,
    unsigned char *pStatus );

extern unsigned long CTR_StartOutputFreq(
    unsigned long DeviceIndex,
    unsigned long BlockIndex,
    double *pHz );

extern unsigned long CTR_8254SelectGate(
    unsigned long DeviceIndex,
    unsigned long GateIndex );

extern unsigned long CTR_8254ReadLatched(
    unsigned long DeviceIndex,
    unsigned short *pData );

extern unsigned long ADC_GetChannelV(
    unsigned long DeviceIndex,
    unsigned long ChannelIndex,
    double *pBuf );

extern unsigned long ADC_GetScanV(
    unsigned long DeviceIndex,
    double *pBuf );

extern unsigned long ADC_GetScan(
    unsigned long DeviceIndex,
    unsigned short *pBuf );

extern unsigned long ADC_GetConfig(
    unsigned long DeviceIndex,
    unsigned char *pConfigBuf,
    unsigned long *ConfigBufSize );

extern unsigned long ADC_SetConfig(
    unsigned long DeviceIndex,
    unsigned char *pConfigBuf,
    unsigned long *ConfigBufSize );

extern unsigned long ADC_RangeAll(
    unsigned long DeviceIndex,
    unsigned char *pGainCodes,
    unsigned long bSingleEnded );

extern unsigned long ADC_Range1(
    unsigned long DeviceIndex,
    unsigned long ADChannel,
    unsigned char GainCode,
    unsigned long bSingleEnded );

extern unsigned long ADC_ADMode(
    unsigned long DeviceIndex,
    unsigned char TriggerMode,
    unsigned char CalMode );

extern unsigned long ADC_SetOversample(
    unsigned long DeviceIndex,
    unsigned char Oversample );

extern unsigned ADC_GetOversample(
    unsigned long DeviceIndex);



extern unsigned long ADC_SetScanLimits(
    unsigned long DeviceIndex,
    unsigned long StartChannel,
    unsigned long EndChannel );

extern unsigned long ADC_SetCal(
    unsigned long DeviceIndex,
    const char *CalFileName );

extern unsigned long ADC_QueryCal(
    unsigned long DeviceIndex );

extern unsigned long ADC_Initialize(
    unsigned long DeviceIndex,
    unsigned char *pConfigBuf,
    unsigned long *ConfigBufSize,
    const char *CalFileName );

extern unsigned long ADC_BulkAcquire(
    unsigned long DeviceIndex,
    unsigned long BufSize,
    void *pBuf );

extern unsigned long ADC_BulkPoll(
    unsigned long DeviceIndex,
    unsigned long *BytesLeft
    );


extern AIOBuf *NewBuffer( unsigned int bufsize );

extern void DeleteBuffer( AIOBuf *buf );

extern AIOBuf *CreateSmartBuffer( unsigned long DeviceIndex );

extern ADConfigBlock *AIOUSB_GetConfigBlock( unsigned long DeviceIndex );
extern unsigned long AIOUSB_SetConfigBlock( unsigned long DeviceIndex , ADConfigBlock *entry );
extern AIORET_TYPE  BulkAcquire(
    unsigned long DeviceIndex,
    AIOBuf *buf,
    int size
                                );

extern AIORET_TYPE  BulkPoll(
    unsigned long DeviceIndex,
    AIOBuf *
                                );



extern unsigned char *ADC_GetADConfigBlock_Registers(
    ADConfigBlock *config
    );



/* FastScan Functions */
extern unsigned long ADC_InitFastITScanV( unsigned long DeviceIndex );

extern unsigned long ADC_CreateFastITConfig(
    unsigned long DeviceIndex,
    int size
    );

extern unsigned long ADC_ResetFastITScanV(
    unsigned long DeviceIndex
    );

extern unsigned long ADC_SetFastITScanVChannels(
    unsigned long DeviceIndex,
    unsigned long NewChannels
    );

extern unsigned long ADC_GetFastITScanV(
    unsigned long DeviceIndex,
    double *pData
    );

extern unsigned long ADC_GetITScanV(
    unsigned long DeviceIndex,
    double *pBuf
    );

extern unsigned long DACDirect(
    unsigned long DeviceIndex,
    unsigned short Channel,
    unsigned short Value );

extern unsigned long DACMultiDirect(
    unsigned long DeviceIndex,
    unsigned short *pDACData,
    unsigned long DACDataCount );

extern unsigned long DACSetBoardRange(
    unsigned long DeviceIndex,
    unsigned long RangeCode );

extern unsigned long DACOutputOpen(
    unsigned long DeviceIndex,
    double *pClockHz );

extern unsigned long DACOutputClose(
    unsigned long DeviceIndex,
    unsigned long bWait );

extern unsigned long DACOutputCloseNoEnd(
    unsigned long DeviceIndex,
    unsigned long bWait );

extern unsigned long DACOutputSetCount(
    unsigned long DeviceIndex,
    unsigned long NewCount );

extern unsigned long DACOutputFrame(
    unsigned long DeviceIndex,
    unsigned long FramePoints,
    unsigned short *FrameData );

extern unsigned long DACOutputFrameRaw(
    unsigned long DeviceIndex,
    unsigned long FramePoints,
    unsigned short *FrameData );

extern unsigned long DACOutputStart(
    unsigned long DeviceIndex );

extern unsigned long DACOutputSetInterlock(
    unsigned long DeviceIndex,
    unsigned long bInterlock );

extern unsigned long GetDeviceSerialNumber(
    unsigned long DeviceIndex,
    __uint64_t *pSerialNumber );

extern unsigned long GetDeviceBySerialNumber( const __uint64_t *pSerialNumber );

extern unsigned long CustomEEPROMWrite(
    unsigned long DeviceIndex,
    unsigned long StartAddress,
    unsigned long DataSize,
    void *Data );

extern unsigned long CustomEEPROMRead(
    unsigned long DeviceIndex,
    unsigned long StartAddress,
    unsigned long *DataSize,
    void *Data );

extern long AIOUSB_GetStreamingBlockSize(
    unsigned long DeviceIndex
    );


extern unsigned long AIOUSB_SetStreamingBlockSize(
    unsigned long DeviceIndex,
    unsigned long BlockSize );

extern unsigned long AIOUSB_ClearFIFO(
    unsigned long DeviceIndex,
    FIFO_Method Method
    );



extern const char *AIOUSB_GetVersion(void);                /* returns AIOUSB module version number as a string */
extern const char *AIOUSB_GetVersionDate(void);            /* returns AIOUSB module version date as a string */
extern const char *AIOUSB_GetResultCodeAsString( unsigned long result );       /* gets string representation of AIOUSB_xxx result code */
extern void AIOUSB_ListDevices(void);                      /* prints list of USB devices to standard output (useful for debugging) */
extern unsigned long AIOUSB_Init(void);                          /* must be called before use of other functions in AIOUSB */
extern void AIOUSB_Exit(void);                                   /* must be called after last use of other functions in AIOUSB */
extern unsigned long AIOUSB_Reset(
    unsigned long DeviceIndex );

extern unsigned long AIOUSB_GetDeviceProperties(
    unsigned long DeviceIndex,
    DeviceProperties *properties );

extern unsigned long AIOUSB_GetDeviceByProductID(
    int minProductID,
    int maxProductID,
    int maxDevices,
    int *deviceList );

extern double AIOUSB_GetMiscClock(
    unsigned long DeviceIndex );

extern unsigned long AIOUSB_SetMiscClock(
    unsigned long DeviceIndex,
    double clockHz );

extern unsigned AIOUSB_GetCommTimeout(
    unsigned long DeviceIndex );

extern unsigned long AIOUSB_SetCommTimeout(
    unsigned long DeviceIndex,
    unsigned timeout );

extern AIOUSB_BOOL AIOUSB_IsDiscardFirstSample(
    unsigned long DeviceIndex );

extern unsigned long AIOUSB_SetDiscardFirstSample(
    unsigned long DeviceIndex,
    AIOUSB_BOOL discard );

extern double AIOUSB_CountsToVolts(
    unsigned long DeviceIndex,
    unsigned channel,
    unsigned short counts );

extern unsigned long AIOUSB_MultipleCountsToVolts(
    unsigned long DeviceIndex,
    unsigned startChannel,
    unsigned endChannel,
    const unsigned short counts[],
    double volts[] );

extern unsigned short AIOUSB_VoltsToCounts(
    unsigned long DeviceIndex,
    unsigned channel,
    double volts );

extern unsigned long AIOUSB_MultipleVoltsToCounts(
    unsigned long DeviceIndex,
    unsigned startChannel,
    unsigned endChannel,
    const double volts[],
    unsigned short counts[] );

extern unsigned long AIOUSB_ADC_LoadCalTable(
    unsigned long DeviceIndex,
    const char *fileName );

extern unsigned long AIOUSB_ADC_SetCalTable(
    unsigned long DeviceIndex,
    const unsigned short calTable[] );

extern unsigned long AIOUSB_ADC_InternalCal(
    unsigned long DeviceIndex,
    AIOUSB_BOOL autoCal,
    unsigned short returnCalTable[],
    const char *saveFileName );

extern void AIOUSB_SetRegister(
    ADConfigBlock *cb,
    unsigned int Register,
    unsigned char value );

extern unsigned char
AIOUSB_GetRegister( ADConfigBlock *cb,
                    unsigned int Register );


extern unsigned long AIOUSB_ADC_ExternalCal(
    unsigned long DeviceIndex,
    const double points[],
    int numPoints,
    unsigned short returnCalTable[],
    const char *saveFileName );


/*
 * ADConfigBlock was created to act like a C++ "class" in order to facilitate configuring
 * the A/D subsystem and do so in a reliable manner; the functions below should be thought
 * of as class "methods" that operate on an "instance" of ADConfigBlock; the method named
 * AIOUSB_InitConfigBlock() is akin to a class "constructor"; below is an example of how
 * to use these functions/methods
 *
 * ADConfigBlock configBlock;
 * AIOUSB_InitConfigBlock( &configBlock, DeviceIndex, AIOUSB_TRUE );		 //call "constructor"
 *   ... set up other properties ...
 * ADC_SetConfig( DeviceIndex, configBlock.registers, &configBlock.size );	 //send configuration block to device
 */
extern void AIOUSB_InitConfigBlock( ADConfigBlock *config, unsigned long DeviceIndex, AIOUSB_BOOL defaults );
extern void AIOUSB_SetAllGainCodeAndDiffMode( ADConfigBlock *config, unsigned gainCode, AIOUSB_BOOL differentialMode );
extern unsigned AIOUSB_GetGainCode( const ADConfigBlock *config, unsigned channel );
extern void AIOUSB_SetGainCode( ADConfigBlock *config, unsigned channel, unsigned gainCode );
extern AIOUSB_BOOL AIOUSB_IsDifferentialMode( const ADConfigBlock *config, unsigned channel );
extern void AIOUSB_SetDifferentialMode( ADConfigBlock *config, unsigned channel, AIOUSB_BOOL differentialMode );
extern unsigned AIOUSB_GetCalMode( const ADConfigBlock *config );
extern void AIOUSB_SetCalMode( ADConfigBlock *config, unsigned calMode );
extern unsigned AIOUSB_GetTriggerMode( const ADConfigBlock *config );
extern void AIOUSB_SetTriggerMode( ADConfigBlock *config, unsigned triggerMode );
extern unsigned AIOUSB_GetStartChannel( const ADConfigBlock *config );
extern unsigned AIOUSB_GetEndChannel( const ADConfigBlock *config );
extern void AIOUSB_SetScanRange( ADConfigBlock *config, unsigned startChannel, unsigned endChannel );
extern unsigned AIOUSB_GetOversample( const ADConfigBlock *config );
extern void AIOUSB_SetOversample( ADConfigBlock *config, unsigned overSample );


#ifdef __aiousb_cplusplus
}
#endif


#endif


/* end of file */
