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


class DiagTab: public Gtk::VBox
{
protected:

	/* pointer to settings block */

    SettingsBlock *m_block;

	/* pointer to app */

    appFrame *m_app;

	/* GUI elements */

    Gtk::VBox m_leftBox;
    Gtk::VBox m_rightBox;
    Gtk::HBox m_screenBox;

    Gtk::Frame m_framebufferStatsFrame;
    Gtk::Table m_framebufferStatsTable;
    Gtk::Entry m_frameCountEntry;
    Gtk::Entry m_framesDroppedEntry;
    Gtk::Entry m_frameBacklogEntry;
    Gtk::Entry m_maxFrameBacklogEntry;

    Gtk::Frame m_metadataStatsFrame;
    Gtk::Table m_metadataStatsTable;
    Gtk::Entry m_msg3CountEntry;
    Gtk::Entry m_gpsCountEntry;

    Gtk::Frame m_firstLineDataFrame;
    Gtk::Table m_firstLineDataTable;
    Gtk::Entry m_localFrameCountEntry;

    Gtk::Frame m_tempsFrame;
    Gtk::Table m_tempsTable;
    Gtk::Label m_temp1Label;
    Gtk::Entry m_temp1Entry;
    Gtk::Label m_temp2Label;
    Gtk::Entry m_temp2Entry;
    Gtk::Label m_temp3Label;
    Gtk::Entry m_temp3Entry;
    Gtk::Label m_temp4Label;
    Gtk::Entry m_temp4Entry;
    Gtk::Label m_temp5Label;
    Gtk::Entry m_temp5Entry;

    Gtk::Frame m_fpgaRegsFrame;
    Gtk::Table m_fpgaRegsTable;
    Gtk::Entry m_fpgaFrameCountEntry;
    Gtk::Entry m_fpgaPPSCountEntry;
    Gtk::Entry m_fpga81ffCountEntry;
    Gtk::Entry m_fpgaFramesDroppedEntry;
    Gtk::Entry m_fpgaSerialErrorsEntry;
    Gtk::Button m_fpgaSerialErrorsButton;
    Gtk::Entry m_fpgaSerialPortCtlEntry;
    Gtk::Entry m_fpgaBufferDepthEntry;
    Gtk::Entry m_fpgaMaxBuffersUsedEntry;
    Gtk::Button m_fpgaMaxBuffersUsedButton;

	/* support routines */

    void addDisplay(Gtk::Table &table, u_int row, const char *descr,
	Gtk::Entry &entry, const char *initialValue, bool enabled) const;
    void addDisplay(Gtk::Table &table, u_int row, Gtk::Label &label,
	Gtk::Entry &entry, const char *initialValue, bool enabled) const;
    void onFPGASerialErrorsButton(void);
    void onFPGAMaxBuffersUsedButton(void);
public:
    DiagTab(SettingsBlock *sb, appFrame *app);

    void setFrameCount(int n);
    void setFramesDropped(int n);
    void setFrameBacklog(int n);
    void setMaxFrameBacklog(int n);

    void setMsg3Count(int n);
    void setGPSCount(int n);

    void setLocalFrameCount(int n);

    void setTemps(int numSensors, char **sensorNames, double *sensorValues);

    void setFPGAFrameCount(int n);
    void setFPGAPPSCount(int n);
    void setFPGA81ffCount(int n);
    void setFPGAFramesDropped(int n);
    void setFPGASerialErrors(int n);
    void setFPGASerialPortCtl(int n);
    void setFPGABufferDepth(int n);
    void setFPGAMaxBuffersUsed(int n);

    void disableDiagChanges(void);
    void enableDiagChanges(void);

    ~DiagTab();
};
