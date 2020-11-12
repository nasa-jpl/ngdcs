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


#include <math.h>
#include <string.h>
#include "framebuf.h"

//#define TRACE_CTL_REG_WRITES

extern const char *const framebufDate = "$Date: 2015/12/03 22:43:55 $";

double AlphaDataFrameBuffer::multipliers[NUM_FRAMERATES] = {
    0.20, 0.25, 1.0/3.0, 0.4, 0.5, 2.0/3.0, 1.0
};

char AlphaDataFrameBuffer::frameRateCodes[NUM_FRAMERATES] = {
    'v', 'x', 'z', '{', '|',  '}', '~'
};


AlphaDataFrameBuffer::AlphaDataFrameBuffer(int fh, int fw,
	void (*er)(const char *s)) :
    FrameBuffer(fh, fw, er)
{
    unsigned int nCardCount;
    int i;
    ADMXRC3_STATUS status;
    ADMXRC3_SENSOR_INFO sensorInfo;
    ADMXRC3_SENSOR_VALUE sensorValue;
    u_int maxNumSensors, fpgaBuffers;
    UINT32 nBankSize, nMaxFrame;
    EFSCResult eRes;

	/* note that we haven't had an error yet */

    failed = 0;
    *last_error = '\0';

	/* init member variables */

    m_card = NULL;
    m_camera = NULL;
    m_cameraIndex = 0;
    m_cardIndex = 0;
    m_appRunning = true;
    m_bitDir = ADFB_BITFILE_PATH;
    m_alarmThread = NULL;
    m_nextFreeImageBuffer = 0;
    m_nextPopulatedImageBuffer = 0;
    m_readyImageBuffers = 0;
    memset(m_imageBuffers, 0, sizeof(m_imageBuffers));
    m_fetchThread = NULL;
    m_frameServer = NULL;
    m_serverHandle = NULL;

    m_framesPerBuffer = MAX_BUFFER_HEIGHT / fh;
    if (m_framesPerBuffer < 1)
	m_framesPerBuffer = 1;
    m_currentImageBuffer = NULL;
    m_framesLeftInBuffer = 0;

    if (fh != 1024)
	for (i=0;i < NUM_FRAMERATES;i++)
	    m_availableFrameRatesInHz[i] = 
		(34.0e6 / ((fw+64)*(fh+4))) * multipliers[i];
    else for (i=0;i < NUM_FRAMERATES;i++)
	m_availableFrameRatesInHz[i] = 
	    (25.0e6 / ((fw+64)*(fh+4))) * multipliers[i];

	/* init the card */

    CPC pcTheComputer;
    nCardCount = pcTheComputer.GetNumberOfXRCs();
    if (nCardCount == 0) {
	strcpy(last_error, "Can't see interface card -- run modprobe?");
	failed = 1;
	return;
    }
    m_card = new CMFBADB3CameraCard(m_cardIndex, XRM_CLink_DB);

	/* configure the card */

    if (!m_card->ConfigureCard(m_bitDir, true) ||
	    m_card->GetStatus() != CARD_OK) {
	strcpy(last_error,
	    "Card configuration failed.  Has some other app got device?");
	m_card = NULL;
	failed = 1;
	return;
    }

    setGPIO(0xffff, 0x0000);

	/* create camera object */

    m_camera = new CGenericCamera(m_card, m_cameraIndex,
	AlphaData::CLink::DATA_FMT_L16, BASE_1X16BITS, m_frameWidthSamples,
	m_frameHeightLines, 0, 0, m_frameWidthSamples, m_frameHeightLines, 0,
	0);
    m_camera->SetDVALOptional(true);
    m_camera->SetInvertDVAL(false);
    m_camera->SetInvertLVAL(false);
    m_camera->SetInvertFVAL(false);
    m_camera->SetInvertClk(false);
    m_camera->SetFramesInOne(m_framesPerBuffer);
    if (!m_camera->ResetLink()) {
	strcpy(last_error, "Can't see camera.  Is it attached?");
	m_camera = NULL;
	m_card = NULL;
	failed = 1;
	return;
    }
    m_camera->RegisterAlarm(&m_alarmEvent);

	/* configure FPGA buffer size */

    nBankSize = (UINT32)m_card->GetMemoryBankSize((UINT8)m_cameraIndex);
    nMaxFrame = nBankSize / m_camera->GetFrameSize();
    nMaxFrame /= m_framesPerBuffer;
    m_card->SetBufferDepth(nMaxFrame);

	/* set up CameraLink serial connection */

    if (m_camera->GetHasSerial()) {
	m_camera->SetSerialBaud(FB_SERIAL_BAUD);
	m_camera->ResetSerial();
	m_camera->WriteSerial("");
    }

	/* init FPIE -- seems like this should be done before we start
  	   acquiring frames in the thread below */

    setFrameRate(NUM_FRAMERATES-1);

	/* set up FPGA serial connection, 115200 baud, odd-parity, by default */

    m_card->WriteReg32(ADFB_FPGA_SERIAL_DIVIDER_REG,
	ADFB_FPGA_SERIAL_DIVIDER_115200);

    m_serialPortCtlReg = ADFB_FPGA_SERIAL_CONFIG_ODD;
    m_card->WriteReg32(ADFB_FPGA_SERIAL_CONFIG_REG, m_serialPortCtlReg);
    m_serialPortCtlReg &= ADFB_FPGA_SERIAL_RESET_MASK;

	/* create image buffer and DMA descriptor -- the memset is just
           for valgrind */

    for (i=0;i < NUM_IMAGE_BUFFERS;i++) {
	m_imageBuffers[i] = new u_char[m_camera->GetFrameSize() *
	    m_camera->GetFramesInOne()];
	(void)memset(m_imageBuffers[i], 0, m_camera->GetFrameSize() *
	    m_camera->GetFramesInOne());
    }

	/* create frame server object */

    m_frameServer = new CFrameServer();
    fpgaBuffers = (256 * 1024 * 1024) / m_camera->GetFramesInOne() /
	m_camera->GetFrameSize();
    m_frameServer->AddImageSource(m_card, m_camera, fpgaBuffers);
    m_frameServer->Start(false);

	/* connect to frame server */

    eRes = m_clientHandle.Connect();
    if (eRes != FSCRes_OK) {
	strcpy(last_error, "Client can't connect to server!");
	m_camera = NULL;
	m_card = NULL;
	failed = 1;
	return;
    }
    eRes = m_clientHandle.OpenCamera(0, 0, m_serverHandle);
    if (eRes != FSCRes_OK) {
	strcpy(last_error, "Client can't open camera!");
	m_camera = NULL;
	m_card = NULL;
	failed = 1;
	return;
    }

	/* set up helper threads */

    if (sem_init(&m_fetchSem, 0, 1) == -1) {
	strcpy(last_error, "Internal error: Can't init semaphore!");
	for (i=0;i < NUM_IMAGE_BUFFERS;i++) {
	    delete[] (u_char *)m_imageBuffers[i];
	    m_imageBuffers[i] = NULL;
	}
	m_camera = NULL;
	m_card = NULL;
	failed = 1;
	return;
    }
    m_fetchThread = new CThread(FetchThread, this);
    m_alarmThread = new CThread(AlarmThread, this);

	/* determine what sensors we have available to read */

    m_numSensors = 0;
    m_cardHandle = NULL;
    (void)memset(&m_cardInfo, 0, sizeof(m_cardInfo));
    m_sensorIndices = NULL;
    m_sensorNames = NULL;
    m_sensorValues = NULL;
    m_sensorRanges = NULL;

    status = ADMXRC3_OpenEx(0, true, 0, &m_cardHandle);
    if (status == ADMXRC3_SUCCESS) {
	status = ADMXRC3_GetCardInfoEx(m_cardHandle, &m_cardInfo);
	if (status == ADMXRC3_SUCCESS) {
	    maxNumSensors = m_cardInfo.NumSensor;
	    m_sensorIndices = new int[maxNumSensors];
	    (void)memset(m_sensorIndices, 0, maxNumSensors * sizeof(int));
	    m_sensorNames = new char *[maxNumSensors];
	    (void)memset(m_sensorNames, 0, maxNumSensors * sizeof(char *));
	    m_sensorValues = new double[maxNumSensors];
	    (void)memset(m_sensorValues, 0, maxNumSensors * sizeof(double));
	    m_sensorRanges = new int[maxNumSensors];
	    (void)memset(m_sensorRanges, 0, maxNumSensors * sizeof(int));
	    for (i=0;i < (int)maxNumSensors;i++) {
		if (ADMXRC3_GetSensorInfo(m_cardHandle, i, &sensorInfo) ==
			    ADMXRC3_SUCCESS &&
			strcasestr(sensorInfo.Description, "temp") != NULL && 
			ADMXRC3_ReadSensor(m_cardHandle, i, &sensorValue) ==
			    ADMXRC3_SUCCESS) {
		    m_sensorIndices[m_numSensors] = i;
		    m_sensorNames[m_numSensors] = new char[200];
		    strncpy(m_sensorNames[m_numSensors], sensorInfo.Description,
		        199);
		    m_sensorNames[m_numSensors][199] = '\0';
		    m_sensorValues[m_numSensors] = 0.0;
		    m_sensorRanges[m_numSensors] = SENSOR_NOMINAL;
		    m_numSensors++;
		}
	    }
	}
	else {
	    ADMXRC3_Close(m_cardHandle);
	    m_cardHandle = NULL;
	}
    }
}


void AlphaDataFrameBuffer::readSensors(int *numSensors_p, char ***sensorNames_p,
    double **sensorValues_p, int **sensorRanges_p)
{
    int i;
    ADMXRC3_SENSOR_INFO sensorInfo;
    ADMXRC3_SENSOR_VALUE sensorValue;
    double limit;

    for (i=0;i < m_numSensors;i++) {
	if (ADMXRC3_ReadSensor(m_cardHandle, m_sensorIndices[i],
		    &sensorValue) == ADMXRC3_SUCCESS &&
		ADMXRC3_GetSensorInfo(m_cardHandle, m_sensorIndices[i],
		    &sensorInfo) == ADMXRC3_SUCCESS && 
		sensorInfo.DataType == ADMXRC3_DATA_DOUBLE) {
	    m_sensorValues[i] = sensorValue.Double;
	    if (strcasestr(m_sensorNames[i], "FPGA") != NULL)
		limit = 90;
	    else limit = 75;
	    if (m_sensorValues[i] < limit-5)
		m_sensorRanges[i] = SENSOR_NOMINAL;
	    else if (m_sensorValues[i] >= limit)
		m_sensorRanges[i] = SENSOR_HOT;
	    else m_sensorRanges[i] = SENSOR_WARM;
	}
	else {
	    m_sensorValues[i] = 0;
	    m_sensorRanges[i] = SENSOR_NOMINAL;
	}
    }
    *numSensors_p = m_numSensors;
    *sensorNames_p = m_sensorNames;
    *sensorValues_p = m_sensorValues;
    *sensorRanges_p = m_sensorRanges;
}


void AlphaDataFrameBuffer::readFPGARegs(int *fpgaFrameCount_p,
    int *fpgaPPSCount_p, int *fpga81ffCount_p, int *fpgaFramesDropped_p,
    int *fpgaSerialErrors_p, int *fpgaSerialPortCtl_p, int *fpgaBufferDepth_p,
    int *fpgaMaxBuffersUsed_p)
{
    int temp;

    *fpgaFrameCount_p = m_card->ReadReg32(ADFB_FPGA_FRAME_COUNT_ADDR);
    *fpgaPPSCount_p = m_card->ReadReg32(ADFB_FPGA_PPS_COUNT_ADDR);
    *fpga81ffCount_p = m_card->ReadReg32(ADFB_FPGA_81FF_COUNT_ADDR);
    *fpgaFramesDropped_p = m_card->ReadReg32(ADFB_FPGA_FRAMES_DROPPED_ADDR);
    *fpgaSerialErrors_p = m_card->ReadReg32(ADFB_FPGA_SERIAL_ERRORS_REG);
    *fpgaSerialPortCtl_p = m_card->ReadReg32(ADFB_FPGA_SERIAL_CONFIG_REG);
    *fpgaBufferDepth_p = m_card->ReadReg32(ADFB_FPGA_BUFFER_DEPTH_ADDR);
    temp = m_card->ReadReg32(ADFB_FPGA_BUFFERS_USED_ADDR);
    if (temp > m_fpgaMaxBuffersUsed)
	m_fpgaMaxBuffersUsed = temp;
    *fpgaMaxBuffersUsed_p = m_fpgaMaxBuffersUsed;
}


u_int AlphaDataFrameBuffer::fpgaPPSCount(void)
{
    u_int result;

    result = m_card->ReadReg32(ADFB_FPGA_PPS_COUNT_ADDR);
    return result;
}


u_int AlphaDataFrameBuffer::fpgaBufferDepth(void)
{
    u_int result;

    result = m_card->ReadReg32(ADFB_FPGA_BUFFER_DEPTH_ADDR);
    return result;
}


u_int AlphaDataFrameBuffer::fpgaBuffersUsed(void)
{
    u_int result;

    result = m_card->ReadReg32(ADFB_FPGA_BUFFERS_USED_ADDR);
    return result;
}


void AlphaDataFrameBuffer::clearSerialErrors(void)
{
    m_card->WriteReg32(ADFB_FPGA_SERIAL_ERRORS_REG, 0);
}


void AlphaDataFrameBuffer::clearFPGAMaxBuffersUsed(void)
{
    m_fpgaMaxBuffersUsed = 0;
}


void AlphaDataFrameBuffer::setExtSerialBaud(int baud)
{
    if (baud == 38400)
	m_card->WriteReg32(ADFB_FPGA_SERIAL_DIVIDER_REG,
	    ADFB_FPGA_SERIAL_DIVIDER_38400);
    else /* baud == 115200 */
	m_card->WriteReg32(ADFB_FPGA_SERIAL_DIVIDER_REG,
	    ADFB_FPGA_SERIAL_DIVIDER_115200);
}


void AlphaDataFrameBuffer::setExtSerialParityOdd(void)
{
    m_serialPortCtlReg = (m_serialPortCtlReg & ADFB_FPGA_SERIAL_PARITY_MASK) |
	ADFB_FPGA_SERIAL_PARITY_ODD;

    m_card->WriteReg32(ADFB_FPGA_SERIAL_CONFIG_REG, m_serialPortCtlReg);
#if defined(TRACE_CTL_REG_WRITES)
    printf("%08x\n", m_serialPortCtlReg);
#endif
}


void AlphaDataFrameBuffer::setExtSerialParityEven(void)
{
    m_serialPortCtlReg = (m_serialPortCtlReg & ADFB_FPGA_SERIAL_PARITY_MASK) |
	ADFB_FPGA_SERIAL_PARITY_EVEN;

    m_card->WriteReg32(ADFB_FPGA_SERIAL_CONFIG_REG, m_serialPortCtlReg);
#if defined(TRACE_CTL_REG_WRITES)
    printf("%08x\n", m_serialPortCtlReg);
#endif
}


void AlphaDataFrameBuffer::setExtSerialParityNone(void)
{
    m_serialPortCtlReg = (m_serialPortCtlReg & ADFB_FPGA_SERIAL_PARITY_MASK) |
	ADFB_FPGA_SERIAL_PARITY_NONE;

    m_card->WriteReg32(ADFB_FPGA_SERIAL_CONFIG_REG, m_serialPortCtlReg);
#if defined(TRACE_CTL_REG_WRITES)
    printf("%08x\n", m_serialPortCtlReg);
#endif
}


void AlphaDataFrameBuffer::setExtSerialTXInvertOff(void)
{
    m_serialPortCtlReg = (m_serialPortCtlReg & ADFB_FPGA_SERIAL_TXINVERT_MASK) |
        ADFB_FPGA_SERIAL_TXINVERT_OFF;

    m_card->WriteReg32(ADFB_FPGA_SERIAL_CONFIG_REG, m_serialPortCtlReg);
#if defined(TRACE_CTL_REG_WRITES)
    printf("%08x\n", m_serialPortCtlReg);
#endif
}


void AlphaDataFrameBuffer::setExtSerialTXInvertOn(void)
{
    m_serialPortCtlReg = (m_serialPortCtlReg & ADFB_FPGA_SERIAL_TXINVERT_MASK) |
        ADFB_FPGA_SERIAL_TXINVERT_ON;

    m_card->WriteReg32(ADFB_FPGA_SERIAL_CONFIG_REG, m_serialPortCtlReg);
#if defined(TRACE_CTL_REG_WRITES)
    printf("%08x\n", m_serialPortCtlReg);
#endif
}


int AlphaDataFrameBuffer::writeExtSerial(u_char *bytes, int numBytes)
{
    int i;
    u_int fifoStatus;

    for (i=0;i < numBytes;i++) {
if (i == 1) break; /* temporary */
	fifoStatus = m_card->ReadReg32(ADFB_FPGA_SERIAL_TXSTATUS_REG);
	if (fifoStatus & (1<<11)) /* FIFO nearly full */
	    break;
	m_card->WriteReg32(ADFB_FPGA_SERIAL_TXFIFO_REG, (u_int)bytes[i]);
    }
    return i;
}


AlphaDataFrameBuffer::~AlphaDataFrameBuffer()
{
    int i;

    m_appRunning = false;
    if (m_alarmThread) {
	m_alarmEvent.Set();
	m_alarmThread->Wait();
	delete m_alarmThread;
    }
    if (m_fetchThread) {
	m_fetchThread->Wait();
	delete m_fetchThread;
    }
    (void)sem_destroy(&m_fetchSem);

    if (m_clientHandle.IsConnected())
	m_clientHandle.Disconnect();
    m_serverHandle->Release();

    if (m_frameServer)
	m_frameServer->Stop();
    if (m_cardHandle)
	ADMXRC3_Close(m_cardHandle);
    if (m_sensorIndices)
	delete[] m_sensorIndices;
    if (m_sensorNames) {
	for (i=0;i < m_numSensors;i++)
	    delete[] m_sensorNames[i];
	delete[] m_sensorNames;
    }
    if (m_sensorValues)
	delete[] m_sensorValues;
    if (m_sensorRanges)
	delete[] m_sensorRanges;

    if (m_camera) {
	m_camera->Stop();
	for (i=0;i < NUM_IMAGE_BUFFERS;i++) {
	    if (m_imageBuffers[i])
		delete[] (u_char *)m_imageBuffers[i];
	}
    }
}


int AlphaDataFrameBuffer::frameIsAvailable(void)
{
    int result;

	/* return immediately if we're not working */

    if (failed) return 0;

	/* use semaphores to create critical section */

    (void)sem_wait(&m_fetchSem);
    result = (m_readyImageBuffers > 0 || m_framesLeftInBuffer);
    (void)sem_post(&m_fetchSem);
    return result;
}


void *AlphaDataFrameBuffer::getFrame(void)
{
    void *result;

    if (failed) return NULL;

    (void)sem_wait(&m_fetchSem);
    if (m_readyImageBuffers == 0 && m_framesLeftInBuffer == 0) {
	(void)sem_post(&m_fetchSem);
	return NULL;
    }
    else if (m_framesLeftInBuffer != 0) {
	result = m_currentImageBuffer;

	m_currentImageBuffer = (char *)m_currentImageBuffer +
	    m_camera->GetFrameSize();
	m_framesLeftInBuffer--;

	(void)sem_post(&m_fetchSem);
	return result;
    }
    else {
	result = m_imageBuffers[m_nextPopulatedImageBuffer];
	m_nextPopulatedImageBuffer++;
	if (m_nextPopulatedImageBuffer == NUM_IMAGE_BUFFERS)
	    m_nextPopulatedImageBuffer = 0;
	m_readyImageBuffers--;

	m_currentImageBuffer = (char *)result + m_camera->GetFrameSize();
	m_framesLeftInBuffer = m_framesPerBuffer - 1;

	(void)sem_post(&m_fetchSem);
	return result;
    }
}


void AlphaDataFrameBuffer::getStats(u_int *dropped_p, u_int *backlog_p)
{
    if (failed) {
	if (dropped_p)
	    *dropped_p = 0;
	if (backlog_p)
	    *backlog_p = 0;
    }
    else {
	if (dropped_p)
	    *dropped_p = m_camera->GetDroppedFrameCount();
	if (backlog_p) {
	    (void)sem_wait(&m_fetchSem);
	    *backlog_p = m_readyImageBuffers;
	    (void)sem_post(&m_fetchSem);
	}
    }
}


void AlphaDataFrameBuffer::FetchThread(void *pArg)
{
    EFSCResult eRes;
    FrameDataPtrT pFrameData;
    u_int nBytes;
    AlphaDataFrameBuffer *me = (AlphaDataFrameBuffer *)pArg;
    u_int thisFrame, incr;

    nBytes = me->m_camera->GetFrameSize() * me->m_camera->GetFramesInOne();

    thisFrame = 0;
    incr = me->m_camera->GetFrameSize();
    while (me->m_appRunning && me->m_clientHandle.IsConnected()) {
	eRes = me->m_serverHandle->GetFrame(pFrameData, GFB_Normal);
	if (eRes == FSCRes_OK) {
	    memcpy((u_char *)me->m_imageBuffers[me->m_nextFreeImageBuffer] +
		    thisFrame * incr,
		pFrameData, incr);

	    eRes = me->m_serverHandle->ReleaseFrameChk(pFrameData);
	    if (eRes == FSCRes_OK) {
		thisFrame++;
		if (thisFrame == me->m_camera->GetFramesInOne()) {
		    thisFrame = 0;
		    me->m_nextFreeImageBuffer++;
		    if (me->m_nextFreeImageBuffer == NUM_IMAGE_BUFFERS)
			me->m_nextFreeImageBuffer = 0;

		    (void)sem_wait(&me->m_fetchSem);
		    me->m_readyImageBuffers++;
		    while (me->m_appRunning &&
			    me->m_readyImageBuffers == NUM_IMAGE_BUFFERS) {
			(void)sem_post(&me->m_fetchSem);
			usleep(10000); /* 10 ms */
			(void)sem_wait(&me->m_fetchSem);
		    }
		    (void)sem_post(&me->m_fetchSem);
		}
	    }
	}
    }
}


void AlphaDataFrameBuffer::AlarmThread(void *pArg)
{
    AlphaDataFrameBuffer *me = (AlphaDataFrameBuffer *)pArg;

    while (me->m_appRunning) {
	me->m_alarmEvent.Wait();
	if (me->m_appRunning && me->m_camera != NULL) {
	    ECameraAlarm eStatus = me->m_camera->GetCameraAlarm();
	    if (me->m_errorReporter != NULL) {
		switch (eStatus) {
		    case ALARM_NONE: 
			if (me->m_errorReporter)
			    (*me->m_errorReporter)(
				"Alpha Data card reporting unknown problem!"); 
			else (void)fprintf(stderr,
			    "Alpha Data card reporting unknown problem!\n"); 
			break;
		    case ALARM_RST_DONE: 
			if (me->m_errorReporter)
			    (*me->m_errorReporter)(
				"Alpha Data card is reporting reset done.");
			else (void)fprintf(stderr,
			    "Alpha Data card is reporting reset done.\n");
			break;
		    case ALARM_LINK_LOST: 
			if (me->m_errorReporter)
			    (*me->m_errorReporter)(
				"Link to Camera Lost!");
			else (void)fprintf(stderr,
			    "Link to Camera Lost!\n");
			break;
		    case ALARM_CLK_STOPPED: 
			if (me->m_errorReporter)
			    (*me->m_errorReporter)(
				"Alpha Data card says pixel clock stopped!");
			else (void)fprintf(stderr,
			    "Alpha Data card says pixel clock stopped!\n");
			break;
		    case ALARM_RST_TIMEOUT: 
			if (me->m_errorReporter)
			    (*me->m_errorReporter)(
				"Reset timed out!");
			else (void)fprintf(stderr,
			    "Reset timed out!\n");
			break;
		}
	    }
	    if (eStatus != ALARM_NONE)
		me->m_camera->ClearCameraAlarm(eStatus);
	}
    }
}


void AlphaDataFrameBuffer::setCCLines(int cc1, int cc2)
{
    unsigned int nBaseReg = BASE_CLINK0;
    unsigned int nData = m_card->ReadReg32(REG_CLNK_CONTROL + nBaseReg);

    if (failed)
	return;

    nData &= ~(CC1_OP | CC2_OP);
    nData |= (cc1? CC1_OP:0) | (cc2? CC2_OP:0);
    AlphaDataFrameBuffer::m_card->WriteReg32(REG_CLNK_CONTROL + nBaseReg,
	nData);
}


void AlphaDataFrameBuffer::writeCLSerial(char *string)
{
    if (failed)
	return;

    (void)AlphaDataFrameBuffer::m_camera->WriteSerial(string);
}


void AlphaDataFrameBuffer::readCLSerial(char *buffer, int maxChars)
{
    if (failed)
	return;

    (void)AlphaDataFrameBuffer::m_camera->ReadSerial(buffer, maxChars);
}


void AlphaDataFrameBuffer::setGPIO(u_int mask, u_int bits)
{
    u_int nData;

    if (failed)
	return;

    nData = m_card->ReadReg32(ADFB_FPGA_GPIO_ADDR);
    nData &= ~mask;
    nData |= (bits & mask);
    m_card->WriteReg32(ADFB_FPGA_GPIO_ADDR, nData);
}


int AlphaDataFrameBuffer::fpgaVersion(void)
{
    u_int nData;

    if (failed)
	return 0;

    nData = m_card->ReadReg32(ADFB_FPGA_GPIO_ADDR);
    return ADFB_FPGA_VERSION_FROM_C0000(nData);
}


int AlphaDataFrameBuffer::setFrameRate(int selection)
{
    char buf[2];

    if (selection < 0 || selection >= NUM_FRAMERATES)
	selection = NUM_FRAMERATES-1;
    buf[0] = frameRateCodes[selection];
    buf[1] = '\0';
    setCCLines(0,0);
    writeCLSerial(buf);
    if (selection == NUM_FRAMERATES-1) /* the nominal operational rate */
	setCCLines(1,1);
    m_currentFrameRate = selection;
    return 0;
}


double AlphaDataFrameBuffer::getFrameRateHz(void)
{
    return m_availableFrameRatesInHz[m_currentFrameRate];
}


double AlphaDataFrameBuffer::getFrameRateHz(int selection)
{
    if (selection < 0)
	return m_availableFrameRatesInHz[0];
    else if (selection >= NUM_FRAMERATES)
	return m_availableFrameRatesInHz[NUM_FRAMERATES-1];
    else return m_availableFrameRatesInHz[selection];
}
