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


class SettingsTab : public Gtk::VBox
{
protected:

	/* pointer to the settings block */

    SettingsBlock *m_block;

	/* pointer to the appFrame */

    appFrame *m_app;

	/* pointers to other tabs */

    DisplayTab *m_displayTab;
    DiagTab *m_diagTab;
    PlotsTab *m_plotsTab;
    ECSDataTab *m_ecsDataTab;

	/* other variables */

    int dialogInitialized;
    FrameBuffer *m_framebuf;
    bool m_silentChange;
    char **m_availableFrameRates;

	/* GUI elements */

    Gtk::VBox m_v1box;
    Gtk::VBox m_v2box;
    Gtk::VBox m_v3box;
    Gtk::HBox m_hbox;
    Gtk::Frame m_modeFrame;
    Gtk::Table m_modeTable;
    Gtk::Frame m_acquisitionFrame;
    Gtk::Table m_acquisitionTable;
    Gtk::Frame m_displayFrame;
    Gtk::Table m_displayTable;
    Gtk::Frame m_dataStorageFrame;
    Gtk::Table m_dataStorageTable;
    Gtk::Frame m_calibrationFrame;
    Gtk::Table m_calibrationTable;
    Gtk::Frame m_shutterFrame;
    Gtk::Table m_shutterTable;
    Gtk::Frame m_gpsFrame;
    Gtk::Table m_gpsTable;
    Gtk::Frame m_ecsconnectionFrame;
    Gtk::Table m_ecsconnectionTable;
    Gtk::Frame m_mountFrame;
    Gtk::Table m_mountTable;
    Gtk::Frame m_headlessFrame;
    Gtk::Table m_headlessTable;
    Gtk::Frame m_simFrame;
    Gtk::Table m_simTable;

    Gtk::ComboBoxText m_modeCombo;

    Gtk::ComboBoxText m_frameRateCombo;
    Gtk::ComboBoxText m_frameHeightCombo;
    Gtk::ComboBoxText m_frameWidthCombo;
    Gtk::ComboBoxText m_compressionOptionCombo;
    Gtk::ComboBoxText m_fmsControlOptionCombo;

    Gtk::ComboBoxText m_viewerTypeCombo;
    Gtk::SpinButton m_redBandSpinButton;
    Gtk::SpinButton m_greenBandSpinButton;
    Gtk::SpinButton m_blueBandSpinButton;
    Gtk::SpinButton m_downtrackSkipSpinButton;
    Gtk::SpinButton m_stretchMinSpinButton;
    Gtk::SpinButton m_stretchMaxSpinButton;
    Gtk::SpinButton m_greenGainSpinButton;
    Gtk::SpinButton m_blueGainSpinButton;
    Gtk::ComboBoxText m_darkSubOptionCombo;
    Gtk::ComboBoxText m_darkSubDirCombo;
    Gtk::Button m_darkSubDirButton;
    Gtk::ComboBoxText m_navDurationCombo;
    Gtk::ComboBoxText m_reflectionOptionCombo;

    Gtk::ComboBoxText m_recMarginCombo;
    Gtk::Entry m_prefixEntry;
    Gtk::ComboBoxText m_productRootDirCombo;
    Gtk::Button m_productRootDirButton;

    Gtk::ComboBoxText m_obcInterfaceCombo;
    Gtk::ComboBoxText m_dark1CalPeriodCombo;
    Gtk::ComboBoxText m_dark2CalPeriodCombo;
    Gtk::ComboBoxText m_mediumCalPeriodCombo;
    Gtk::ComboBoxText m_brightCalPeriodCombo;
    Gtk::ComboBoxText m_laserCalPeriodCombo;

    Gtk::ComboBoxText m_shutterInterfaceCombo;

    Gtk::ComboBoxText m_gpsSerialBaudCombo;
    Gtk::ComboBoxText m_gpsSerialParityCombo;
    Gtk::ComboBoxText m_gpsSerialTXInvertCombo;
    Gtk::ComboBoxText m_gpsInitModeCombo;

    Gtk::ComboBoxText m_ecsConnectedCombo;
    Gtk::Entry m_ecsHostEntry;
    Gtk::SpinButton m_ecsPortSpinButton;
    Gtk::SpinButton m_ecsTimeoutInMsSpinButton;
    Gtk::Entry m_ecsUsernameEntry;
    Gtk::Entry m_ecsPasswordEntry;

    Gtk::ComboBoxText m_mountUsedCombo;
    Gtk::Entry m_mountDevEntry;

    Gtk::Entry m_headlessDevEntry;

    Gtk::ComboBoxText m_imageSimCombo;
    Gtk::ComboBoxText m_gpsSimCombo;
    Gtk::ComboBoxText m_diagOptionCombo;

	/* handlers */

    void onModeChange(void);

    void onFrameRateChange(void);
    void onFrameHeightChange(void);
    void onFrameWidthChange(void);
    void onCompressionOptionChange(void);
    void onFMSControlOptionChange(void);

    void onViewerTypeChange(void);
    void onRedBandChange(void);
    void onGreenBandChange(void);
    void onBlueBandChange(void);
    void onDowntrackSkipChange(void);
    void onStretchMinChange(void);
    void onStretchMaxChange(void);
    void onGreenGainChange(void);
    void onBlueGainChange(void);
    void onDarkSubOptionChange(void);
    void onDarkSubDirChange(void);
    void onDarkSubDirBrowseButton(void);
    void onNavDurationChange(void);
    void onReflectionOptionChange(void);

    void onRecMarginChange(void);
    bool onPrefixChange(GdkEventFocus *event);
    void onProductRootDirChange(void);
    void onProductRootDirBrowseButton(void);

    void onOBCInterfaceChange(void);
    void onDark1CalPeriodChange(void);
    void onDark2CalPeriodChange(void);
    void onMediumCalPeriodChange(void);
    void onBrightCalPeriodChange(void);
    void onLaserCalPeriodChange(void);

    void onShutterInterfaceChange(void);

    void onGPSSerialBaudChange(void);
    void onGPSSerialParityChange(void);
    void onGPSSerialTXInvertChange(void);
    void onGPSInitModeChange(void);

    void onECSConnectedChange(void);
    bool onECSHostChange(GdkEventFocus *event);
    void onECSPortChange(void);
    void onECSTimeoutInMsChange(void);
    bool onECSUsernameChange(GdkEventFocus *event);
    bool onECSPasswordChange(GdkEventFocus *event);

    void onMountUsedChange(void);
    bool onMountDevChange(GdkEventFocus *event);

    bool onHeadlessDevChange(GdkEventFocus *event);

    void onImageSimChange(void);
    void onGPSSimChange(void);
    void onDiagOptionChange(void);

	/* misc */

    void addComboSetting(Gtk::Table &table, u_int row, const char *descr,
	Gtk::ComboBoxText &combo, const char *const options[], int defaultValue,
	void (SettingsBlock::*setter)(int value), bool enabled) const;
    void addEntrySetting(Gtk::Table &table, u_int row, const char *descr,
	Gtk::Entry &entry, const char *initialValue, bool enabled) const;
    void addIntSpinSetting(Gtk::Table &table, u_int row, const char *descr,
	Gtk::SpinButton &button, int min, int max, int initialValue,
	void (SettingsBlock::*setter)(int value), bool enabled) const;
    void addFloatSpinSetting(Gtk::Table &table, u_int row, const char *descr,
	Gtk::SpinButton &button, double min, double max, double initialValue,
	void (SettingsBlock::*setter)(double value), bool enabled) const;
    void addDataLocationSetting(Gtk::Table &table, u_int row, const char *descr,
	Gtk::ComboBoxText &combo, Gtk::Button &button, const char *mru,
	bool enabled) const;
    void handleDarkSubDirBrowse(const char *prompt,
	Gtk::ComboBoxText *combo) const;
    void handleProductRootDirBrowse(const char *prompt,
	Gtk::ComboBoxText *combo) const;
    void comboEntryToIndex(const Gtk::ComboBoxText *combo,
	const char *const strings[], void (SettingsBlock::*setter)(int value),
	const char *description);
public:
    SettingsTab(SettingsBlock *b, appFrame *af, DisplayTab *dd, DiagTab *dt,
	PlotsTab *pt, ECSDataTab *tdd, FrameBuffer *fb);
    void disableSettingsChanges(void);
    void enableSettingsChanges(void);
    void updateRedBandSelection(int red, bool updateControl);
    void updateGreenBandSelection(int green, bool updateControl);
    void updateBlueBandSelection(int blue, bool updateControl);
    void updateDowntrackSkipSelection(int skip, bool updateControl);
    void updateStretchMinSelection(int min, bool updateControl);
    void updateStretchMaxSelection(int max, bool updateControl);
    void updateGreenGainSelection(double value, bool updateControl);
    void updateBlueGainSelection(double value, bool updateControl);
    void forceDefaultFrameRate(void);
    ~SettingsTab();
};
