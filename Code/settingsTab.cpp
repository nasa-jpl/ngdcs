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
#include "lists.h"
#include "main.h"
#include "framebuf.h"
#include "plotting.h"
#include "appFrame.h"
#include "displayTab.h"
#include "diagTab.h"
#include "plotsTab.h"
#include "ecsDataTab.h"
#include "settingsBlock.h"
#include "settingsTab.h"

extern const char *const settingsTabDate = "$Date: 2015/12/03 21:36:17 $";

// #define DEBUG


SettingsTab::SettingsTab(SettingsBlock *b, appFrame *af, DisplayTab *dd,
	DiagTab *dt, PlotsTab *pt, ECSDataTab *tdd, FrameBuffer *fb) :
    m_modeTable(1, 2, false),
    m_acquisitionTable(5, 2, false),
    m_displayTable(11, 2, false),
    m_dataStorageTable(3, 2, false),
    m_calibrationTable(6, 2, false),
    m_shutterTable(1, 2, false),
    m_gpsTable(4, 2, false),
    m_ecsconnectionTable(6, 2, false),
    m_mountTable(2, 2, false),
    m_headlessTable(1, 2, false),
    m_simTable(3, 2, false)
{
    int i;

        /* note that we haven't filled the dialog in yet.  this prevents the
	   value retrieval routines from taking values from the dialog until
	   the dialog is actually initialized */

    dialogInitialized = 0;

        /* save pointer to the settings block */

    m_block = b;

	/* save pointer to the app frame */

    m_app = af;

	/* save pointer to the other dialogs */

    m_displayTab = dd;
    m_diagTab = dt;
    m_plotsTab = pt;
    m_ecsDataTab = tdd;

	/* save pointer to the framebuffer and make list of currently
           available frame rates */

    m_framebuf = fb;

    m_availableFrameRates = new char *[m_framebuf->numFrameRates() + 1];
    for (i=0;i < m_framebuf->numFrameRates();i++) {
	m_availableFrameRates[i] = new char[20];
	(void)sprintf(m_availableFrameRates[i], "%.1f Hz",
	    m_framebuf->getFrameRateHz(i));
    }
    m_availableFrameRates[m_framebuf->numFrameRates()] = NULL;

	/* other inits */

    m_silentChange = false;

	/* set up spacing */

    m_hbox.set_spacing(15);
    m_v1box.set_spacing(15);
    m_v2box.set_spacing(15);
    m_v3box.set_spacing(15);

    m_modeTable.set_border_width(10);
    m_acquisitionTable.set_border_width(10);
    m_displayTable.set_border_width(10);
    m_dataStorageTable.set_border_width(10);
    m_calibrationTable.set_border_width(10);
    m_shutterTable.set_border_width(10);
    m_gpsTable.set_border_width(10);
    m_ecsconnectionTable.set_border_width(10);
    m_mountTable.set_border_width(10);
    m_headlessTable.set_border_width(10);
    m_simTable.set_border_width(10);

    set_border_width(15);
    set_spacing(15);

	/* set up mode area */

    m_modeTable.set_row_spacings(5);
    m_modeTable.set_col_spacings(15);

    addComboSetting(m_modeTable, 0, "Program mode",
	m_modeCombo, m_block->availableModes(),
	m_block->currentMode(), &SettingsBlock::setMode, true);
    (void)m_modeCombo.signal_changed().connect(sigc::mem_fun(*this,
        &SettingsTab::onModeChange));

    m_modeFrame.add(m_modeTable);
    m_modeFrame.set_label("General");
    m_v1box.pack_start(m_modeFrame, Gtk::PACK_SHRINK);

	/* set up acquisition area */

    m_acquisitionTable.set_row_spacings(5);
    m_acquisitionTable.set_col_spacings(15);

    addComboSetting(m_acquisitionTable, 0, "Frame Rate",
	m_frameRateCombo, m_availableFrameRates,
	m_block->currentFrameRate(), &SettingsBlock::setFrameRate, true);
    (void)m_frameRateCombo.signal_changed().connect(sigc::mem_fun(*this,
        &SettingsTab::onFrameRateChange));

    addComboSetting(m_acquisitionTable, 1, "Frame Height",
	m_frameHeightCombo, m_block->availableFrameHeights(),
	m_block->currentFrameHeight(), &SettingsBlock::setFrameHeight, true);
    (void)m_frameHeightCombo.signal_changed().connect(sigc::mem_fun(*this,
        &SettingsTab::onFrameHeightChange));

    addComboSetting(m_acquisitionTable, 2, "Frame Width",
	m_frameWidthCombo, m_block->availableFrameWidths(),
	m_block->currentFrameWidth(), &SettingsBlock::setFrameWidth, true);
    (void)m_frameWidthCombo.signal_changed().connect(sigc::mem_fun(*this,
        &SettingsTab::onFrameWidthChange));

    addComboSetting(m_acquisitionTable, 3, "Compression",
	m_compressionOptionCombo, m_block->availableCompressionOptions(),
	m_block->currentCompressionOption(),
	&SettingsBlock::setCompressionOption, true);
    (void)m_compressionOptionCombo.signal_changed().connect(sigc::mem_fun(*this,
        &SettingsTab::onCompressionOptionChange));

    addComboSetting(m_acquisitionTable, 4, "FMS Control",
	m_fmsControlOptionCombo, m_block->availableFMSControlOptions(),
	m_block->currentFMSControlOption(),
	&SettingsBlock::setFMSControlOption, true);
    (void)m_fmsControlOptionCombo.signal_changed().connect(sigc::mem_fun(*this,
        &SettingsTab::onFMSControlOptionChange));

    m_acquisitionFrame.add(m_acquisitionTable);
    m_acquisitionFrame.set_label("Acquisition");
    m_v1box.pack_start(m_acquisitionFrame, Gtk::PACK_SHRINK);

	/* set up display area */

    m_displayTable.set_row_spacings(5);
    m_displayTable.set_col_spacings(15);

    addComboSetting(m_displayTable, 0, "Viewer Type",
	m_viewerTypeCombo, m_block->availableViewerTypes(),
	m_block->currentViewerType(), &SettingsBlock::setViewerType, true);
    (void)m_viewerTypeCombo.signal_changed().connect(sigc::mem_fun(*this,
        &SettingsTab::onViewerTypeChange));

    addIntSpinSetting(m_displayTable, 1, "Waterfall Red Band",
	m_redBandSpinButton,
	1, m_block->currentFrameHeightLines() - 1, m_block->currentRedBand(),
	&SettingsBlock::setRedBand,
	(m_block->currentViewerType() == VIEWERTYPE_WATERFALL ||
	    m_block->currentViewerType() == VIEWERTYPE_COMBO));
    (void)m_redBandSpinButton.signal_value_changed().connect(
	sigc::mem_fun(*this, &SettingsTab::onRedBandChange));
    m_redBandSpinButton.set_increments(1.0, 100.0);

    addIntSpinSetting(m_displayTable, 2, "Waterfall Green Band",
	m_greenBandSpinButton,
	1, m_block->currentFrameHeightLines() - 1, m_block->currentGreenBand(),
	&SettingsBlock::setGreenBand,
	(m_block->currentViewerType() == VIEWERTYPE_WATERFALL ||
	    m_block->currentViewerType() == VIEWERTYPE_COMBO));
    (void)m_greenBandSpinButton.signal_value_changed().connect(
	sigc::mem_fun(*this, &SettingsTab::onGreenBandChange));
    m_greenBandSpinButton.set_increments(1.0, 100.0);

    addIntSpinSetting(m_displayTable, 3, "Waterfall Blue Band",
	m_blueBandSpinButton,
	1, m_block->currentFrameHeightLines() - 1, m_block->currentBlueBand(),
	&SettingsBlock::setBlueBand,
	(m_block->currentViewerType() == VIEWERTYPE_WATERFALL ||
	    m_block->currentViewerType() == VIEWERTYPE_COMBO));
    (void)m_blueBandSpinButton.signal_value_changed().connect(
	sigc::mem_fun(*this, &SettingsTab::onBlueBandChange));
    m_blueBandSpinButton.set_increments(1.0, 100.0);

    addIntSpinSetting(m_displayTable, 4, "Downtrack Skip",
	m_downtrackSkipSpinButton,
	0, 9, m_block->currentDowntrackSkip(), &SettingsBlock::setDowntrackSkip,
	(m_block->currentViewerType() == VIEWERTYPE_WATERFALL ||
	    m_block->currentViewerType() == VIEWERTYPE_COMBO));
    (void)m_downtrackSkipSpinButton.signal_value_changed().connect(
	sigc::mem_fun(*this, &SettingsTab::onDowntrackSkipChange));
    m_downtrackSkipSpinButton.set_increments(1.0, 1.0);

    addIntSpinSetting(m_displayTable, 5, "Stretch Min",
	m_stretchMinSpinButton,
	0, MAX_DN, m_block->currentStretchMin(), &SettingsBlock::setStretchMin,
	true);
    (void)m_stretchMinSpinButton.signal_value_changed().connect(
	sigc::mem_fun(*this, &SettingsTab::onStretchMinChange));
    m_stretchMinSpinButton.set_increments(1.0, 1.0);

    addIntSpinSetting(m_displayTable, 6, "Stretch Max",
	m_stretchMaxSpinButton,
	0, MAX_DN, m_block->currentStretchMax(), &SettingsBlock::setStretchMax,
	true);
    (void)m_stretchMaxSpinButton.signal_value_changed().connect(
	sigc::mem_fun(*this, &SettingsTab::onStretchMaxChange));
    m_stretchMaxSpinButton.set_increments(1.0, 1.0);

    addFloatSpinSetting(m_displayTable, 7, "Green Gain",
	m_greenGainSpinButton,
	0.0, 10.0, m_block->currentGreenGain(), &SettingsBlock::setGreenGain,
	true);
    (void)m_greenGainSpinButton.signal_value_changed().connect(
	sigc::mem_fun(*this, &SettingsTab::onGreenGainChange));
    m_greenGainSpinButton.set_increments(0.01, 0.1);

    addFloatSpinSetting(m_displayTable, 8, "Blue Gain",
	m_blueGainSpinButton,
	0.0, 10.0, m_block->currentBlueGain(), &SettingsBlock::setBlueGain,
	true);
    (void)m_blueGainSpinButton.signal_value_changed().connect(
	sigc::mem_fun(*this, &SettingsTab::onBlueGainChange));
    m_blueGainSpinButton.set_increments(0.01, 0.1);

    addComboSetting(m_displayTable, 9, "Dark Subtraction",
	m_darkSubOptionCombo, m_block->availableDarkSubOptions(),
	m_block->currentDarkSubOption(), &SettingsBlock::setDarkSubOption,
	true);
    (void)m_darkSubOptionCombo.signal_changed().connect(sigc::mem_fun(*this,
        &SettingsTab::onDarkSubOptionChange));

    addDataLocationSetting(m_displayTable, 10, "Dark-frame Dir",
	m_darkSubDirCombo, m_darkSubDirButton,
	m_block->currentDarkSubDirMRU(),
	(m_block->currentDarkSubOption() == DARKSUBOPTION_YES));
    (void)m_darkSubDirCombo.signal_changed().connect(sigc::mem_fun(*this,
        &SettingsTab::onDarkSubDirChange));
    (void)m_darkSubDirButton.signal_clicked().connect(sigc::mem_fun(*this,
	&SettingsTab::onDarkSubDirBrowseButton));

    addComboSetting(m_displayTable, 11, "Nav-plot duration",
	m_navDurationCombo, m_block->availableNavDurations(),
	m_block->currentNavDuration(), &SettingsBlock::setNavDuration,
	true);
    (void)m_navDurationCombo.signal_changed().connect(sigc::mem_fun(*this,
        &SettingsTab::onNavDurationChange));

    addComboSetting(m_displayTable, 12, "Reflection",
	m_reflectionOptionCombo, m_block->availableReflectionOptions(),
	m_block->currentReflectionOption(),
	&SettingsBlock::setReflectionOption, true);
    (void)m_reflectionOptionCombo.signal_changed().connect(sigc::mem_fun(*this,
        &SettingsTab::onReflectionOptionChange));

    m_displayFrame.add(m_displayTable);
    m_displayFrame.set_label("Display");
    m_v2box.pack_start(m_displayFrame, Gtk::PACK_SHRINK);

	/* set up the data-storage area */

    m_dataStorageTable.set_row_spacings(5);
    m_dataStorageTable.set_col_spacings(15);

    addComboSetting(m_dataStorageTable, 0, "Recording Margin",
	m_recMarginCombo, m_block->availableRecMargins(),
	m_block->currentRecMargin(), &SettingsBlock::setRecMargin, true);
    (void)m_recMarginCombo.signal_changed().connect(sigc::mem_fun(*this,
        &SettingsTab::onRecMarginChange));

    addEntrySetting(m_dataStorageTable, 1, "Product Prefix",
	m_prefixEntry, m_block->currentPrefix(), true);
    (void)m_prefixEntry.signal_focus_out_event().connect(
	sigc::mem_fun(*this, &SettingsTab::onPrefixChange));

    addDataLocationSetting(m_dataStorageTable, 2, "Product Root Dir",
	m_productRootDirCombo, m_productRootDirButton,
	m_block->currentProductRootDirMRU(), true);
    (void)m_productRootDirCombo.signal_changed().connect(sigc::mem_fun(*this,
        &SettingsTab::onProductRootDirChange));
    (void)m_productRootDirButton.signal_clicked().connect(sigc::mem_fun(*this,
	&SettingsTab::onProductRootDirBrowseButton));

    m_dataStorageFrame.add(m_dataStorageTable);
    m_dataStorageFrame.set_label("Data Storage");
    m_v2box.pack_start(m_dataStorageFrame, Gtk::PACK_SHRINK);

	/* make calibration area */

    m_calibrationTable.set_row_spacings(5);
    m_calibrationTable.set_col_spacings(15);

    addComboSetting(m_calibrationTable, 0, "OBC Interface",
	m_obcInterfaceCombo, m_block->availableOBCInterfaces(),
	m_block->currentOBCInterface(), &SettingsBlock::setOBCInterface, true);
    (void)m_obcInterfaceCombo.signal_changed().connect(sigc::mem_fun(*this,
        &SettingsTab::onOBCInterfaceChange));

    addComboSetting(m_calibrationTable, 1, "Starting Dark Period",
	m_dark1CalPeriodCombo, m_block->availableCalPeriods(),
	m_block->currentDark1CalPeriod(), &SettingsBlock::setDark1CalPeriod,
	(m_block->currentOBCInterface() != OBCINTERFACE_NONE));
    (void)m_dark1CalPeriodCombo.signal_changed().connect(sigc::mem_fun(*this,
        &SettingsTab::onDark1CalPeriodChange));

    addComboSetting(m_calibrationTable, 2, "Ending Dark Period",
	m_dark2CalPeriodCombo, m_block->availableCalPeriods(),
	m_block->currentDark2CalPeriod(), &SettingsBlock::setDark2CalPeriod,
	(m_block->currentOBCInterface() != OBCINTERFACE_NONE));
    (void)m_dark2CalPeriodCombo.signal_changed().connect(sigc::mem_fun(*this,
        &SettingsTab::onDark2CalPeriodChange));

    addComboSetting(m_calibrationTable, 3, "Ending Medium Period",
	m_mediumCalPeriodCombo, m_block->availableCalPeriods(),
	m_block->currentMediumCalPeriod(), &SettingsBlock::setMediumCalPeriod,
	(m_block->currentOBCInterface() != OBCINTERFACE_NONE));
    (void)m_mediumCalPeriodCombo.signal_changed().connect(sigc::mem_fun(*this,
        &SettingsTab::onMediumCalPeriodChange));

    addComboSetting(m_calibrationTable, 4, "Ending Bright Period",
	m_brightCalPeriodCombo, m_block->availableCalPeriods(),
	m_block->currentBrightCalPeriod(), &SettingsBlock::setBrightCalPeriod,
	(m_block->currentOBCInterface() != OBCINTERFACE_NONE));
    (void)m_brightCalPeriodCombo.signal_changed().connect(sigc::mem_fun(*this,
        &SettingsTab::onBrightCalPeriodChange));

    addComboSetting(m_calibrationTable, 5, "Ending Laser Period",
	m_laserCalPeriodCombo, m_block->availableCalPeriods(),
	m_block->currentLaserCalPeriod(), &SettingsBlock::setLaserCalPeriod,
	(m_block->currentOBCInterface() != OBCINTERFACE_NONE));
    (void)m_laserCalPeriodCombo.signal_changed().connect(sigc::mem_fun(*this,
        &SettingsTab::onLaserCalPeriodChange));

    m_calibrationFrame.add(m_calibrationTable);
    m_calibrationFrame.set_label("Calibration");
    m_v3box.pack_start(m_calibrationFrame, Gtk::PACK_SHRINK);

	/* set up shutter area */

    m_shutterTable.set_row_spacings(5);
    m_shutterTable.set_col_spacings(15);

    addComboSetting(m_shutterTable, 0, "Shutter Interface",
	m_shutterInterfaceCombo, m_block->availableShutterInterfaces(),
	m_block->currentShutterInterface(), &SettingsBlock::setShutterInterface,
        true);
    (void)m_shutterInterfaceCombo.signal_changed().connect(sigc::mem_fun(*this,
        &SettingsTab::onShutterInterfaceChange));

    m_shutterFrame.add(m_shutterTable);
    m_shutterFrame.set_label("Shutter");
    m_v1box.pack_start(m_shutterFrame, Gtk::PACK_SHRINK);

	/* set up GPS area */

    m_gpsTable.set_row_spacings(5);
    m_gpsTable.set_col_spacings(15);

    addComboSetting(m_gpsTable, 0, "Serial Baud",
	m_gpsSerialBaudCombo, m_block->availableGPSSerialBauds(),
	m_block->currentGPSSerialBaud(), &SettingsBlock::setGPSSerialBaud,
	true);
    (void)m_gpsSerialBaudCombo.signal_changed().connect(sigc::mem_fun(*this,
        &SettingsTab::onGPSSerialBaudChange));

    addComboSetting(m_gpsTable, 1, "Serial Parity",
	m_gpsSerialParityCombo, m_block->availableGPSSerialParities(),
	m_block->currentGPSSerialParity(), &SettingsBlock::setGPSSerialParity,
        true);
    (void)m_gpsSerialParityCombo.signal_changed().connect(sigc::mem_fun(*this,
        &SettingsTab::onGPSSerialParityChange));

    addComboSetting(m_gpsTable, 2, "Serial TX invert",
	m_gpsSerialTXInvertCombo, m_block->availableGPSSerialTXInverts(),
	m_block->currentGPSSerialTXInvert(),
	&SettingsBlock::setGPSSerialTXInvert, true);
    (void)m_gpsSerialTXInvertCombo.signal_changed().connect(sigc::mem_fun(*this,
        &SettingsTab::onGPSSerialTXInvertChange));

    addComboSetting(m_gpsTable, 3, "Init Mode",
	m_gpsInitModeCombo, m_block->availableGPSInitModes(),
	m_block->currentGPSInitMode(), &SettingsBlock::setGPSInitMode,
        true);
    (void)m_gpsInitModeCombo.signal_changed().connect(sigc::mem_fun(*this,
        &SettingsTab::onGPSInitModeChange));

    m_gpsFrame.add(m_gpsTable);
    m_gpsFrame.set_label("GPS");
    m_v1box.pack_start(m_gpsFrame, Gtk::PACK_SHRINK);

	/* set up ECS-connection area */

    m_ecsconnectionTable.set_row_spacings(5);
    m_ecsconnectionTable.set_col_spacings(15);

    addComboSetting(m_ecsconnectionTable, 0, "ECS Connected",
	m_ecsConnectedCombo, m_block->availableECSOptions(),
	m_block->currentECSConnected(), &SettingsBlock::setECSConnected, true);
    (void)m_ecsConnectedCombo.signal_changed().connect(sigc::mem_fun(*this,
        &SettingsTab::onECSConnectedChange));

    addEntrySetting(m_ecsconnectionTable, 1, "ECS IP Address",
	m_ecsHostEntry, m_block->currentECSHost(),
	(m_block->currentECSConnected() == ECSCONNECTED_YES));
    (void)m_ecsHostEntry.signal_focus_out_event().connect(
	sigc::mem_fun(*this, &SettingsTab::onECSHostChange));

    addIntSpinSetting(m_ecsconnectionTable, 2, "ECS Port Number",
	m_ecsPortSpinButton,
	MIN_ECS_PORT, MAX_ECS_PORT, m_block->currentECSPort(),
	&SettingsBlock::setECSPort,
	(m_block->currentECSConnected() == ECSCONNECTED_YES));
    (void)m_ecsPortSpinButton.signal_value_changed().connect(
	sigc::mem_fun(*this, &SettingsTab::onECSPortChange));
    m_ecsPortSpinButton.set_increments(1.0, 1000.0);

    addIntSpinSetting(m_ecsconnectionTable, 3, "ECS Timeout (ms)",
	m_ecsTimeoutInMsSpinButton,
	MIN_ECS_TIMEOUT, MAX_ECS_TIMEOUT, m_block->currentECSTimeoutInMs(),
	&SettingsBlock::setECSTimeoutInMs,
	(m_block->currentECSConnected() == ECSCONNECTED_YES));
    (void)m_ecsTimeoutInMsSpinButton.signal_value_changed().connect(
	sigc::mem_fun(*this, &SettingsTab::onECSTimeoutInMsChange));
    m_ecsTimeoutInMsSpinButton.set_increments(100.0, 1000.0);

    addEntrySetting(m_ecsconnectionTable, 4, "ECS Username",
	m_ecsUsernameEntry, m_block->currentECSUsername(),
	(m_block->currentECSConnected() == ECSCONNECTED_YES));
    (void)m_ecsUsernameEntry.signal_focus_out_event().connect(
	sigc::mem_fun(*this, &SettingsTab::onECSUsernameChange));

    addEntrySetting(m_ecsconnectionTable, 5, "ECS Password",
	m_ecsPasswordEntry, m_block->currentECSPassword(),
	(m_block->currentECSConnected() == ECSCONNECTED_YES));
    (void)m_ecsPasswordEntry.signal_focus_out_event().connect(
	sigc::mem_fun(*this, &SettingsTab::onECSPasswordChange));
    m_ecsPasswordEntry.set_visibility(false);

    m_ecsconnectionFrame.add(m_ecsconnectionTable);
    m_ecsconnectionFrame.set_label("ECS Connection");
    m_v3box.pack_start(m_ecsconnectionFrame, Gtk::PACK_SHRINK);

	/* set up mount area */

    m_mountTable.set_row_spacings(5);
    m_mountTable.set_col_spacings(15);

    addComboSetting(m_mountTable, 0, "Mount Used",
	m_mountUsedCombo, m_block->availableMountOptions(),
	m_block->currentMountUsed(), &SettingsBlock::setMountUsed, true);
    (void)m_mountUsedCombo.signal_changed().connect(sigc::mem_fun(*this,
        &SettingsTab::onMountUsedChange));

    addEntrySetting(m_mountTable, 1, "Serial Device",
	m_mountDevEntry, m_block->currentMountDev(),
	(m_block->currentMountUsed() == MOUNTUSED_YES));
    (void)m_mountDevEntry.signal_focus_out_event().connect(
	sigc::mem_fun(*this, &SettingsTab::onMountDevChange));

    m_mountFrame.add(m_mountTable);
    m_mountFrame.set_label("Mount");
    m_v3box.pack_start(m_mountFrame, Gtk::PACK_SHRINK);

	/* set up headless area */

    m_headlessTable.set_row_spacings(5);
    m_headlessTable.set_col_spacings(15);

    addEntrySetting(m_headlessTable, 0, "Serial Device",
	m_headlessDevEntry, m_block->currentHeadlessDev(), true);
    (void)m_headlessDevEntry.signal_focus_out_event().connect(
	sigc::mem_fun(*this, &SettingsTab::onHeadlessDevChange));

    m_headlessFrame.add(m_headlessTable);
    m_headlessFrame.set_label("Headless operation");
    m_v3box.pack_start(m_headlessFrame, Gtk::PACK_SHRINK);

	/* set up sim area */

    m_simTable.set_row_spacings(5);
    m_simTable.set_col_spacings(15);

    addComboSetting(m_simTable, 0, "Image Sim",
	m_imageSimCombo, m_block->availableImageSims(),
	m_block->currentImageSim(), &SettingsBlock::setImageSim, true);
    (void)m_imageSimCombo.signal_changed().connect(sigc::mem_fun(*this,
        &SettingsTab::onImageSimChange));

    addComboSetting(m_simTable, 1, "GPS Sim",
	m_gpsSimCombo, m_block->availableGPSSims(),
	m_block->currentGPSSim(), &SettingsBlock::setGPSSim, true);
    (void)m_gpsSimCombo.signal_changed().connect(sigc::mem_fun(*this,
        &SettingsTab::onGPSSimChange));

    addComboSetting(m_simTable, 2, "Diagnostics",
	m_diagOptionCombo, m_block->availableDiagOptions(),
	m_block->currentDiagOption(), &SettingsBlock::setDiagOption, true);
    (void)m_diagOptionCombo.signal_changed().connect(sigc::mem_fun(*this,
        &SettingsTab::onDiagOptionChange));

    m_simFrame.add(m_simTable);
    m_simFrame.set_label("Simulation and Diagnostics");
    m_v1box.pack_start(m_simFrame, Gtk::PACK_SHRINK);

	/* add v-boxes to h-box, and h-box to main box */

    m_hbox.pack_start(m_v1box, Gtk::PACK_SHRINK);
    m_hbox.pack_start(m_v2box, Gtk::PACK_SHRINK);
    m_hbox.pack_start(m_v3box, Gtk::PACK_SHRINK);
    pack_start(m_hbox, Gtk::PACK_SHRINK);

        /* note that we've initialized the dialog */

    dialogInitialized = 1;
}


void SettingsTab::addComboSetting(Gtk::Table &table, u_int row,
    const char *descr, Gtk::ComboBoxText &combo, const char *const options[],
    int defaultValue, void (SettingsBlock::*setter)(int value),
    bool enabled) const
{
    int i;
    Gtk::Alignment *a;
    Gtk::Label *l;

    l = new Gtk::Label;
    l->set_text(descr);
    for (i=0;options[i] != NULL;i++)
        combo.append_text(options[i]);
    if (defaultValue >= i) {
        defaultValue = 0; /* shouldn't happen */
	(m_block->*setter)(defaultValue);
    }
    combo.set_active_text(options[defaultValue]);
    a = new Gtk::Alignment(Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, (float)0.0,
	(float)0.0);
    a->add(*l);
    table.attach(*a, 0, 1, row, row+1, (Gtk::AttachOptions)Gtk::FILL,
	(Gtk::AttachOptions)0);
    table.attach(combo, 1, 2, row, row+1, (Gtk::AttachOptions)Gtk::FILL,
	(Gtk::AttachOptions)0);
    combo.set_sensitive(enabled);
}


void SettingsTab::comboEntryToIndex(const Gtk::ComboBoxText *combo,
    const char *const strings[], void (SettingsBlock::*setter)(int value),
    const char *description)
{
    int value;
    char logmsg[200];

    m_block->stringToIndex(combo->get_active_text().c_str(),
	strings, &value, description);

    if (m_silentChange)
	m_silentChange = false;
    else {
	sprintf(logmsg, "User changed %s to %s.", description, strings[value]);
	m_displayTab->log(logmsg);
    }

    (m_block->*setter)(value);
}


void SettingsTab::addEntrySetting(Gtk::Table &table, u_int row,
    const char *descr, Gtk::Entry &entry, const char *initialValue,
    bool enabled) const
{
    Gtk::Alignment *a;
    Gtk::Label *l;

    l = new Gtk::Label;
    l->set_text(descr);
    entry.set_text(initialValue);
    a = new Gtk::Alignment(Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, (float)0.0,
	(float)0.0);
    a->add(*l);
    table.attach(*a, 0, 1, row, row+1, (Gtk::AttachOptions)Gtk::FILL,
	(Gtk::AttachOptions)0);
    table.attach(entry, 1, 2, row, row+1, (Gtk::AttachOptions)0,
	(Gtk::AttachOptions)0);
    entry.set_sensitive(enabled);
}


void SettingsTab::addIntSpinSetting(Gtk::Table &table, u_int row,
    const char *descr, Gtk::SpinButton &button, int min, int max,
    int initialValue, void (SettingsBlock::*setter)(int value),
    bool enabled) const
{
    Gtk::Alignment *a;
    Gtk::Label *l;

    l = new Gtk::Label;
    l->set_text(descr);
    button.set_range((double)min, (double)max);
    button.set_increments(1.0, 20.0);
    if (initialValue < min) {
        initialValue = min; /* shouldn't happen */
	(m_block->*setter)(initialValue);
    }
    else if (initialValue > max) {
        initialValue = max; /* shouldn't happen */
	(m_block->*setter)(initialValue);
    }
    button.set_value((double)initialValue);
    a = new Gtk::Alignment(Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, (float)0.0,
	(float)0.0);
    a->add(*l);
    table.attach(*a, 0, 1, row, row+1, (Gtk::AttachOptions)Gtk::FILL,
	(Gtk::AttachOptions)0);
    table.attach(button, 1, 2, row, row+1, (Gtk::AttachOptions)Gtk::FILL,
	(Gtk::AttachOptions)0);
    button.set_sensitive(enabled);
}


void SettingsTab::addFloatSpinSetting(Gtk::Table &table, u_int row,
    const char *descr, Gtk::SpinButton &button, double min, double max,
    double initialValue, void (SettingsBlock::*setter)(double value),
    bool enabled) const
{
    Gtk::Alignment *a;
    Gtk::Label *l;

    l = new Gtk::Label;
    l->set_text(descr);
    button.set_range(min, max);
    button.set_digits(2);
    button.set_increments(0.01, 0.1);
    if (initialValue < min) {
        initialValue = min; /* shouldn't happen */
	(m_block->*setter)(initialValue);
    }
    else if (initialValue > max) {
        initialValue = max; /* shouldn't happen */
	(m_block->*setter)(initialValue);
    }
    button.set_value(initialValue);
    a = new Gtk::Alignment(Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, (float)0.0,
	(float)0.0);
    a->add(*l);
    table.attach(*a, 0, 1, row, row+1, (Gtk::AttachOptions)Gtk::FILL,
	(Gtk::AttachOptions)0);
    table.attach(button, 1, 2, row, row+1, (Gtk::AttachOptions)Gtk::FILL,
	(Gtk::AttachOptions)0);
    button.set_sensitive(enabled);
}


void SettingsTab::addDataLocationSetting(Gtk::Table &table, u_int row,
    const char *descr, Gtk::ComboBoxText &combo, Gtk::Button &button,
    const char *mru, bool enabled) const
{
    int i;
    Gtk::Alignment *a;
    Gtk::Label *l;

    l = new Gtk::Label;
    l->set_text(descr);
    for (i=0;i < 4;i++)
        if (*(mru+i*MAXPATHLEN) != '\0')
	    combo.append_text(mru+i*MAXPATHLEN);
    if (*mru != '\0')
        combo.set_active_text(mru);
    a = new Gtk::Alignment(Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, (float)0.0,
	(float)0.0);
    a->add(*l);
    table.attach(*a, 0, 1, row, row+1, (Gtk::AttachOptions)Gtk::FILL,
	(Gtk::AttachOptions)0);
    table.attach(combo, 1, 4, row, row+1, (Gtk::AttachOptions)Gtk::FILL,
	(Gtk::AttachOptions)0);
    button.set_label("Browse");
    table.attach(button, 4, 5, row, row+1, (Gtk::AttachOptions)0,
	(Gtk::AttachOptions)0);
    combo.set_sensitive(enabled);
    button.set_sensitive(enabled);
}


void SettingsTab::onModeChange(void)
{
    comboEntryToIndex(&m_modeCombo,
	m_block->availableModes(), &SettingsBlock::setMode,
	"Mode");
}


void SettingsTab::onFrameRateChange(void)
{
    comboEntryToIndex(&m_frameRateCombo,
	m_availableFrameRates, &SettingsBlock::setFrameRate,
	"frame rate");

    m_displayTab->updateResourcesDisplay();

    m_framebuf->setFrameRate(m_block->currentFrameRate());
}


void SettingsTab::forceDefaultFrameRate(void)
{
    m_silentChange = true;
    m_frameRateCombo.set_active_text(
	m_availableFrameRates[m_framebuf->numFrameRates()-1]);
}


void SettingsTab::onFrameHeightChange(void)
{
    static int changeWasForced = 0;
    int value;
    char logmsg[200];

	/* if this is a force, just return.  we don't want to bother the user
           with any of this */

    if (changeWasForced) {
	changeWasForced = 0;
	return;
    }

	/* we need to restart for frame-height changes because they require us
           to reinit the Alpha Data board through the creation of a new
	   camera object.  warn the user that change is deferred */

    Gtk::MessageDialog d(
	"New frame height will take effect after program is restarted.",
	false /* no markup */, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK);
    d.set_secondary_text(
	"Available frame rates will adjust to match frame height.");
    (void)d.run();
   
        /* log change, and pass requested frame height to appFrame object for
           later */

    m_block->stringToIndex(m_frameHeightCombo.get_active_text().c_str(),
	m_block->availableFrameHeights(), &value, "frame height");

    sprintf(logmsg, "User changed frame height to %s.",
	m_block->availableFrameHeights()[value]);
    m_displayTab->log(logmsg);

    m_app->setPendingFrameHeight(value);

	/* restore original selection on GUI */

    changeWasForced = 1;
    m_frameHeightCombo.set_active_text(
	m_block->availableFrameHeights()[m_block->currentFrameHeight()]);
}


void SettingsTab::onFrameWidthChange(void)
{
    static int changeWasForced = 0;
    int value;
    char logmsg[200];

	/* if this is a force, just return.  we don't want to bother the user
           with any of this */

    if (changeWasForced) {
	changeWasForced = 0;
	return;
    }

	/* we need to restart for frame-width changes because they require us
           to reinit the Alpha Data board through the creation of a new
	   camera object.  warn the user that change is deferred */

    Gtk::MessageDialog d(
	"New frame width will take effect after program is restarted.",
	false /* no markup */, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK);
    d.set_secondary_text(
	"Available frame rates will adjust to match frame width.");
    (void)d.run();
   
        /* log change, and pass requested frame width to appFrame object for
           later */

    m_block->stringToIndex(m_frameWidthCombo.get_active_text().c_str(),
	m_block->availableFrameWidths(), &value, "frame width");

    sprintf(logmsg, "User changed frame width to %s.",
	m_block->availableFrameWidths()[value]);
    m_displayTab->log(logmsg);

    m_app->setPendingFrameWidth(value);

	/* restore original selection on GUI */

    changeWasForced = 1;
    m_frameWidthCombo.set_active_text(
	m_block->availableFrameWidths()[m_block->currentFrameWidth()]);
}


void SettingsTab::onCompressionOptionChange(void)
{
    static int changeWasForced = 0;
    int originalOption, newOption;

	/* if this is a force, just return.  we don't want to bother the user
           with any of this */

    if (changeWasForced) {
	changeWasForced = 0;
	return;
    }

	/* note previous configuration */

    originalOption = m_block->currentCompressionOption();

        /* get new compression option -- when this gets handled will depend
   	   on the specific transition we're making */

    comboEntryToIndex(&m_compressionOptionCombo,
	m_block->availableCompressionOptions(),
	&SettingsBlock::setCompressionOption, "compression option");
    newOption = m_block->currentCompressionOption();

	/* if no change, ignore.  this shouldn't happen */

    if (originalOption == newOption)
	/* do nothing */ ;

	/* else if going to or from COMPRESSION_UNSUPPORTED, defer until next 
           restart, because we need to reinit the Alpha Data board to make
	   the change */

    else if (originalOption == COMPRESSION_UNSUPPORTED ||
	    newOption == COMPRESSION_UNSUPPORTED) {

	    /* - notify user - */

	Gtk::MessageDialog d(
	    "Compression change will take effect after program is restarted.",
	    false /* no markup */, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK);
	(void)d.run();
   
	    /* - pass compression option to appFrame object for later - */

	m_app->setPendingCompressionOption(newOption);

	    /* - restore original selection on GUI - */

	changeWasForced = 1;
	m_compressionOptionCombo.set_active_text(
	    m_block->availableCompressionOptions()[originalOption]);
    }

	/* otherwise we can handle the change immediately, but we still need
           to set the "pending" variable to make sure that the change gets
           saved */

    else m_app->setPendingCompressionOption(newOption);
}


void SettingsTab::onFMSControlOptionChange(void)
{
    comboEntryToIndex(&m_fmsControlOptionCombo,
	m_block->availableFMSControlOptions(),
	&SettingsBlock::setFMSControlOption, "FMS control option");

    m_displayTab->fmsControlChanged();
}


void SettingsTab::onViewerTypeChange(void)
{
        /* get new viewer type -- the display tab will pick this up with the
	   next data display */

    comboEntryToIndex(&m_viewerTypeCombo,
	m_block->availableViewerTypes(), &SettingsBlock::setViewerType,
	"viewer type");

        /* update band-combo enable */

    if (m_block->currentViewerType() == VIEWERTYPE_WATERFALL ||
	    m_block->currentViewerType() == VIEWERTYPE_COMBO) {
        m_redBandSpinButton.set_sensitive(true);
        m_greenBandSpinButton.set_sensitive(true);
        m_blueBandSpinButton.set_sensitive(true);
        m_downtrackSkipSpinButton.set_sensitive(true);
    }
    else /* VIEWERTYPE_FRAME */ {
	m_redBandSpinButton.set_sensitive(false);
	m_greenBandSpinButton.set_sensitive(false);
	m_blueBandSpinButton.set_sensitive(false);
        m_downtrackSkipSpinButton.set_sensitive(false);
    }

	/* reset the display */

    if (!appFrame::headless)
	m_displayTab->resetImageDisplay();
}


void SettingsTab::onRedBandChange(void)
{
    char logmsg[200];

    if (m_silentChange)
	m_silentChange = false;
    else {
	sprintf(logmsg, "User changed waterfall red band to %d.",
	    m_redBandSpinButton.get_value_as_int());
	m_displayTab->log(logmsg);
    }

    m_block->setRedBand(m_redBandSpinButton.get_value_as_int());
}


void SettingsTab::onGreenBandChange(void)
{
    char logmsg[200];

    if (m_silentChange)
	m_silentChange = false;
    else {
	sprintf(logmsg, "User changed waterfall green band to %d.",
	    m_greenBandSpinButton.get_value_as_int());
	m_displayTab->log(logmsg);
    }

    m_block->setGreenBand(m_greenBandSpinButton.get_value_as_int());
}


void SettingsTab::onBlueBandChange(void)
{
    char logmsg[200];

    if (m_silentChange)
	m_silentChange = false;
    else {
	sprintf(logmsg, "User changed waterfall blue band to %d.",
	    m_blueBandSpinButton.get_value_as_int());
	m_displayTab->log(logmsg);
    }

    m_block->setBlueBand(m_blueBandSpinButton.get_value_as_int());
}


void SettingsTab::onDowntrackSkipChange(void)
{
    char logmsg[200];

    if (m_silentChange)
	m_silentChange = false;
    else {
	sprintf(logmsg, "User changed downtrack skip to %d.",
	    m_downtrackSkipSpinButton.get_value_as_int());
	m_displayTab->log(logmsg);
    }

    m_block->setDowntrackSkip(m_downtrackSkipSpinButton.get_value_as_int());
}


void SettingsTab::onStretchMinChange(void)
{
    char logmsg[200];

    if (m_silentChange)
	m_silentChange = false;
    else {
	sprintf(logmsg, "User changed stretch min to %d.",
	    m_stretchMinSpinButton.get_value_as_int());
	m_displayTab->log(logmsg);
    }

    m_block->setStretchMin(m_stretchMinSpinButton.get_value_as_int());
    m_displayTab->updateStretch();
}


void SettingsTab::onStretchMaxChange(void)
{
    char logmsg[200];

    if (m_silentChange)
	m_silentChange = false;
    else {
	sprintf(logmsg, "User changed stretch max to %d.",
	    m_stretchMaxSpinButton.get_value_as_int());
	m_displayTab->log(logmsg);
    }

    m_block->setStretchMax(m_stretchMaxSpinButton.get_value_as_int());
    m_displayTab->updateStretch();
}


void SettingsTab::onGreenGainChange(void)
{
    char logmsg[200];

    if (m_silentChange)
	m_silentChange = false;
    else {
	sprintf(logmsg, "User changed green gain to %g.",
	    m_greenGainSpinButton.get_value());
	m_displayTab->log(logmsg);
    }

    m_block->setGreenGain(m_greenGainSpinButton.get_value());
    m_displayTab->updateStretch();
}


void SettingsTab::onBlueGainChange(void)
{
    char logmsg[200];

    if (m_silentChange)
	m_silentChange = false;
    else {
	sprintf(logmsg, "User changed blue gain to %g.",
	    m_blueGainSpinButton.get_value());
	m_displayTab->log(logmsg);
    }

    m_block->setBlueGain(m_blueGainSpinButton.get_value());
    m_displayTab->updateStretch();
}


void SettingsTab::onDarkSubOptionChange(void)
{
    comboEntryToIndex(&m_darkSubOptionCombo,
	m_block->availableDarkSubOptions(), &SettingsBlock::setDarkSubOption,
	"dark-subtraction option");

    if (m_block->currentDarkSubOption() == DARKSUBOPTION_YES) {
	m_darkSubDirCombo.set_sensitive(true);
	m_darkSubDirButton.set_sensitive(true);
    }
    else {
	m_darkSubDirCombo.set_sensitive(false);
	m_darkSubDirButton.set_sensitive(false);
    }
}


void SettingsTab::onDarkSubDirChange(void)
{
    char errorMsg[MAXPATHLEN + 60];
    char logmsg[200];

    if (m_silentChange)
	m_silentChange = false;
    else {
	sprintf(logmsg, "User changed dark-frame location to %s.",
	    m_block->currentDarkSubDir());
	m_displayTab->log(logmsg);
    }

    if (appFrame::ensureDirectoryExists(m_block->currentDarkSubDir()) == -1) {
	(void)sprintf(errorMsg,
	    "Can't find or create dark-subtraction file directory \"%s\".",
	    m_block->currentDarkSubDir());
	m_displayTab->warn(errorMsg);
    }
    else {
	m_block->insertInDarkSubDirMRU(
	    m_darkSubDirCombo.get_active_text().c_str());

	m_displayTab->readDarkFrame();
    }
}


void SettingsTab::onDarkSubDirBrowseButton()
{
    handleDarkSubDirBrowse("Select directory for dark-subtraction file",
	&m_darkSubDirCombo);
}


void SettingsTab::onNavDurationChange(void)
{
    comboEntryToIndex(&m_navDurationCombo,
	m_block->availableNavDurations(), &SettingsBlock::setNavDuration,
	"nav-plot duration");

    m_displayTab->resetNavPlots();
    m_plotsTab->resetNavPlots();
}


void SettingsTab::onReflectionOptionChange(void)
{
        /* get new reflection option -- the display tab will pick this up with
   	   the next data display */

    comboEntryToIndex(&m_reflectionOptionCombo,
	m_block->availableReflectionOptions(),
	&SettingsBlock::setReflectionOption, "reflection option");

	/* reset the display */

    if (!appFrame::headless)
	m_displayTab->resetImageDisplay();
}


void SettingsTab::onRecMarginChange(void)
{
    comboEntryToIndex(&m_recMarginCombo,
	m_block->availableRecMargins(), &SettingsBlock::setRecMargin,
	"recording margin");

    m_displayTab->updateResourcesDisplay();
}


bool SettingsTab::onPrefixChange(GdkEventFocus *event)
{
    char logmsg[200];

    if (m_silentChange)
	m_silentChange = false;
    else {
	sprintf(logmsg, "User changed data product prefix to %s.",
	    m_prefixEntry.get_text().c_str());
	m_displayTab->log(logmsg);
    }

    m_block->setPrefix(m_prefixEntry.get_text().c_str());
    return false;
}


void SettingsTab::onProductRootDirChange(void)
{
    char errorMsg[MAXPATHLEN + 60];
    char logmsg[200];

    if (m_silentChange)
	m_silentChange = false;
    else {
	sprintf(logmsg, "User changed product root dir to %s.",
	    m_block->currentProductRootDir());
	m_displayTab->log(logmsg);
    }

    if (appFrame::ensureDirectoryExists(
	    m_block->currentProductRootDir()) == -1) {
	sprintf(errorMsg, "Can't access data directory \"%s\".",
	    m_block->currentProductRootDir());
	m_displayTab->warn(errorMsg);
    }
    else {
	m_block->insertInProductRootDirMRU(
	    m_productRootDirCombo.get_active_text().c_str());

	if (appFrame::makeDailyDir(m_block->currentProductRootDir(), NULL) ==
		-1) {
	    (void)sprintf(errorMsg,
		"Can't find or create daily-data dir under \"%s\".",
		m_block->currentProductRootDir());
	    m_displayTab->warn(errorMsg);
	}

	m_displayTab->updateResourcesDisplay();
    }
}


void SettingsTab::onProductRootDirBrowseButton()
{
    handleProductRootDirBrowse("Select root directory for data products",
	&m_productRootDirCombo);
}


void SettingsTab::onOBCInterfaceChange(void)
{
    comboEntryToIndex(&m_obcInterfaceCombo,
	m_block->availableOBCInterfaces(),
	&SettingsBlock::setOBCInterface, "OBC interface");

    switch (m_block->currentOBCInterface()) {
        case OBCINTERFACE_FPGA:
            m_dark1CalPeriodCombo.set_sensitive(true);
            m_dark2CalPeriodCombo.set_sensitive(true);
            m_mediumCalPeriodCombo.set_sensitive(true);
            m_brightCalPeriodCombo.set_sensitive(true);
            m_laserCalPeriodCombo.set_sensitive(true);

            m_displayTab->enableControlChanges();
            break;
        case OBCINTERFACE_NONE:
        default:
            m_dark1CalPeriodCombo.set_sensitive(false);
            m_dark2CalPeriodCombo.set_sensitive(false);
            m_mediumCalPeriodCombo.set_sensitive(false);
            m_brightCalPeriodCombo.set_sensitive(false);
            m_laserCalPeriodCombo.set_sensitive(false);

            m_displayTab->disableControlChanges(0, 0, 1, 0);
            break;
    }
}


void SettingsTab::onDark1CalPeriodChange(void)
{
    comboEntryToIndex(&m_dark1CalPeriodCombo,
	m_block->availableCalPeriods(),
	&SettingsBlock::setDark1CalPeriod, "first dark cal period");
}


void SettingsTab::onDark2CalPeriodChange(void)
{
    comboEntryToIndex(&m_dark2CalPeriodCombo,
	m_block->availableCalPeriods(),
	&SettingsBlock::setDark2CalPeriod, "second dark cal period");
}


void SettingsTab::onMediumCalPeriodChange(void)
{
    comboEntryToIndex(&m_mediumCalPeriodCombo,
	m_block->availableCalPeriods(),
	&SettingsBlock::setMediumCalPeriod, "medium cal period");
}


void SettingsTab::onBrightCalPeriodChange(void)
{
    comboEntryToIndex(&m_brightCalPeriodCombo,
	m_block->availableCalPeriods(),
	&SettingsBlock::setBrightCalPeriod, "bright cal period");
}


void SettingsTab::onLaserCalPeriodChange(void)
{
    comboEntryToIndex(&m_laserCalPeriodCombo,
	m_block->availableCalPeriods(),
	&SettingsBlock::setLaserCalPeriod, "laser cal period");
}


void SettingsTab::onShutterInterfaceChange(void)
{
    comboEntryToIndex(&m_shutterInterfaceCombo,
	m_block->availableShutterInterfaces(),
	&SettingsBlock::setShutterInterface, "shutter interface");

    if (m_block->currentShutterInterface() == SHUTTERINTERFACE_NONE)
        m_displayTab->disableControlChanges(0, 1, 0, 0);
    else m_displayTab->enableControlChanges();
}


void SettingsTab::onGPSSerialBaudChange(void)
{
    comboEntryToIndex(&m_gpsSerialBaudCombo,
	m_block->availableGPSSerialBauds(),
	&SettingsBlock::setGPSSerialBaud, "GPS serial baudrate");

    m_framebuf->setExtSerialBaud(m_block->currentGPSSerialBaudBPS());
}


void SettingsTab::onGPSSerialParityChange(void)
{
    comboEntryToIndex(&m_gpsSerialParityCombo,
	m_block->availableGPSSerialParities(),
	&SettingsBlock::setGPSSerialParity, "GPS serial parity");

    switch (m_block->currentGPSSerialParity()) {
	case GPSSERIALPARITY_NONE:
	    m_framebuf->setExtSerialParityNone();
	    break;
	case GPSSERIALPARITY_EVEN:
	    m_framebuf->setExtSerialParityEven();
	    break;
	case GPSSERIALPARITY_ODD:
	default:
	    m_framebuf->setExtSerialParityOdd();
	    break;
    }
}


void SettingsTab::onGPSSerialTXInvertChange(void)
{
    comboEntryToIndex(&m_gpsSerialTXInvertCombo,
	m_block->availableGPSSerialTXInverts(),
	&SettingsBlock::setGPSSerialTXInvert, "GPS serial TX-invert");

    switch (m_block->currentGPSSerialTXInvert()) {
	case GPSSERIALTXINVERT_ON:
	    m_framebuf->setExtSerialTXInvertOn();
	    break;
	case GPSSERIALTXINVERT_OFF:
	default:
	    m_framebuf->setExtSerialTXInvertOff();
	    break;
    }
}


void SettingsTab::onGPSInitModeChange(void)
{
    comboEntryToIndex(&m_gpsInitModeCombo,
	m_block->availableGPSInitModes(),
	&SettingsBlock::setGPSInitMode, "GPS init mode");
}


void SettingsTab::onECSConnectedChange(void)
{
    comboEntryToIndex(&m_ecsConnectedCombo,
	m_block->availableECSOptions(), &SettingsBlock::setECSConnected,
	"ECS connection");

    if (m_block->currentECSConnected() == ECSCONNECTED_YES) {
        m_ecsHostEntry.set_sensitive(true);
        m_ecsPortSpinButton.set_sensitive(true);
        m_ecsTimeoutInMsSpinButton.set_sensitive(true);
        m_ecsUsernameEntry.set_sensitive(true);
        m_ecsPasswordEntry.set_sensitive(true);

        m_ecsDataTab->enableECSDataChanges();
    }
    else {
        m_ecsHostEntry.set_sensitive(false);
        m_ecsPortSpinButton.set_sensitive(false);
        m_ecsTimeoutInMsSpinButton.set_sensitive(false);
        m_ecsUsernameEntry.set_sensitive(false);
        m_ecsPasswordEntry.set_sensitive(false);

        m_ecsDataTab->disableECSDataChanges();
    }
}


bool SettingsTab::onECSHostChange(GdkEventFocus *event)
{
    char logmsg[200];

    if (m_silentChange)
	m_silentChange = false;
    else {
	sprintf(logmsg, "User changed ECS host to %s.",
	    m_ecsHostEntry.get_text().c_str());
	m_displayTab->log(logmsg);
    }

    m_block->setECSHost(m_ecsHostEntry.get_text().c_str());
    return false;
}


void SettingsTab::onECSPortChange(void)
{
    char logmsg[200];

    if (m_silentChange)
	m_silentChange = false;
    else {
	sprintf(logmsg, "User changed ECS port to %d.",
	    m_ecsPortSpinButton.get_value_as_int());
	m_displayTab->log(logmsg);
    }

    m_block->setECSPort(m_ecsPortSpinButton.get_value_as_int());
}


bool SettingsTab::onECSUsernameChange(GdkEventFocus *event)
{
    char logmsg[200];

    if (m_silentChange)
	m_silentChange = false;
    else {
	sprintf(logmsg, "User changed ECS username to %s.",
	    m_ecsUsernameEntry.get_text().c_str());
	m_displayTab->log(logmsg);
    }

    m_block->setECSUsername(m_ecsUsernameEntry.get_text().c_str());
    return false;
}


bool SettingsTab::onECSPasswordChange(GdkEventFocus *event)
{
    char logmsg[200];

    if (m_silentChange)
	m_silentChange = false;
    else {
	strcpy(logmsg, "User changed ECS password.");
	m_displayTab->log(logmsg);
    }

    m_block->setECSPassword(m_ecsPasswordEntry.get_text().c_str());
    return false;
}


void SettingsTab::onECSTimeoutInMsChange(void)
{
    char logmsg[200];

    if (m_silentChange)
	m_silentChange = false;
    else {
	sprintf(logmsg, "User changed ECS timeout to %d ms.",
	    m_ecsTimeoutInMsSpinButton.get_value_as_int());
	m_displayTab->log(logmsg);
    }

    m_block->setECSTimeoutInMs(m_ecsTimeoutInMsSpinButton.get_value_as_int());
}


void SettingsTab::onMountUsedChange(void)
{
    comboEntryToIndex(&m_mountUsedCombo,
	m_block->availableMountOptions(), &SettingsBlock::setMountUsed,
	"Mount usage");

    if (m_block->currentMountUsed() == MOUNTUSED_NO) {
        m_mountDevEntry.set_sensitive(false);

        m_displayTab->disableControlChanges(0, 0, 0, 1);
        m_displayTab->makeMountUnused();
    }
    else {
        m_mountDevEntry.set_sensitive(true);

	m_displayTab->enableControlChanges();
        m_displayTab->makeMountUsed();
    }
}


bool SettingsTab::onMountDevChange(GdkEventFocus *event)
{
    char logmsg[200];

    if (m_silentChange)
	m_silentChange = false;
    else {
	sprintf(logmsg, "User changed mount serial device to %s.",
	    m_mountDevEntry.get_text().c_str());
	m_displayTab->log(logmsg);
    }

    m_block->setMountDev(m_mountDevEntry.get_text().c_str());
    return false;
}


bool SettingsTab::onHeadlessDevChange(GdkEventFocus *event)
{
    char logmsg[200];

    if (m_silentChange)
	m_silentChange = false;
    else {
	sprintf(logmsg, "User changed headless controlling device to %s.",
	    m_headlessDevEntry.get_text().c_str());
	m_displayTab->log(logmsg);
    }

    m_block->setHeadlessDev(m_headlessDevEntry.get_text().c_str());
    return false;
}


void SettingsTab::onImageSimChange(void)
{
        /* get new sim type -- the display tab will pick this up with the
	   next data display */

    comboEntryToIndex(&m_imageSimCombo,
	m_block->availableImageSims(), &SettingsBlock::setImageSim,
	"image simulation type");

	/* reset the display */

    if (!appFrame::headless)
	m_displayTab->resetImageDisplay();
}


void SettingsTab::onGPSSimChange(void)
{
        /* get new sim type */

    comboEntryToIndex(&m_gpsSimCombo,
	m_block->availableGPSSims(), &SettingsBlock::setGPSSim,
	"GPS simulation type");
}


void SettingsTab::onDiagOptionChange(void)
{
    comboEntryToIndex(&m_diagOptionCombo,
	m_block->availableDiagOptions(), &SettingsBlock::setDiagOption,
	"diagnostics option");

    if (m_block->currentDiagOption() == DIAGOPTION_OFF)
        m_diagTab->disableDiagChanges();
    else m_diagTab->enableDiagChanges();
}


void SettingsTab::disableSettingsChanges(void)
{
    if (dialogInitialized) {

	    /* mode */

        m_modeCombo.set_sensitive(false);

	    /* acquisition */

        m_frameRateCombo.set_sensitive(false);
        m_frameHeightCombo.set_sensitive(false);
        m_frameWidthCombo.set_sensitive(false);
        m_compressionOptionCombo.set_sensitive(false);
        m_fmsControlOptionCombo.set_sensitive(false);

	    /* display -- some of these are always on */

	m_viewerTypeCombo.set_sensitive(true);
	if (m_block->currentViewerType() == VIEWERTYPE_WATERFALL ||
		m_block->currentViewerType() == VIEWERTYPE_COMBO) {
	    m_redBandSpinButton.set_sensitive(true);
	    m_greenBandSpinButton.set_sensitive(true);
	    m_blueBandSpinButton.set_sensitive(true);
	    m_downtrackSkipSpinButton.set_sensitive(true);
	}
	m_stretchMinSpinButton.set_sensitive(true);
	m_stretchMaxSpinButton.set_sensitive(true);
	m_greenGainSpinButton.set_sensitive(true);
	m_blueGainSpinButton.set_sensitive(true);
        m_darkSubOptionCombo.set_sensitive(false);
        m_darkSubDirCombo.set_sensitive(false);
        m_darkSubDirButton.set_sensitive(false);
        m_navDurationCombo.set_sensitive(true);
	m_reflectionOptionCombo.set_sensitive(true);

	    /* data storage */

        m_recMarginCombo.set_sensitive(false);
        m_prefixEntry.set_sensitive(false);
        m_productRootDirCombo.set_sensitive(false);
        m_productRootDirButton.set_sensitive(false);

	    /* calibration */

        m_obcInterfaceCombo.set_sensitive(false);
        m_dark1CalPeriodCombo.set_sensitive(false);
        m_dark2CalPeriodCombo.set_sensitive(false);
        m_mediumCalPeriodCombo.set_sensitive(false);
        m_brightCalPeriodCombo.set_sensitive(false);
        m_laserCalPeriodCombo.set_sensitive(false);

	    /* shutter */

        m_shutterInterfaceCombo.set_sensitive(false);

	    /* GPS */

        m_gpsSerialBaudCombo.set_sensitive(false);
        m_gpsSerialParityCombo.set_sensitive(false);
        m_gpsSerialTXInvertCombo.set_sensitive(false);
        m_gpsInitModeCombo.set_sensitive(false);

	    /* ECS connection */

        m_ecsConnectedCombo.set_sensitive(false);
        m_ecsHostEntry.set_sensitive(false);
        m_ecsPortSpinButton.set_sensitive(false);
        m_ecsTimeoutInMsSpinButton.set_sensitive(false);
        m_ecsUsernameEntry.set_sensitive(false);
        m_ecsPasswordEntry.set_sensitive(false);

	    /* mount */

        m_mountUsedCombo.set_sensitive(false);
        m_mountDevEntry.set_sensitive(false);

	    /* headless operation */

        m_headlessDevEntry.set_sensitive(false);

	    /* sim -- placeholder only.  doesn't change
	       sensitivity since we want to keep these
	       usable in all contexts */

	m_imageSimCombo.set_sensitive(true);
	m_gpsSimCombo.set_sensitive(true);
        m_diagOptionCombo.set_sensitive(true);
    }
}


void SettingsTab::enableSettingsChanges(void)
{
    if (dialogInitialized) {

	    /* mode */

        m_modeCombo.set_sensitive(true);

	    /* acquisition */

	m_frameRateCombo.set_sensitive(true);
        m_frameHeightCombo.set_sensitive(true);
        m_frameWidthCombo.set_sensitive(true);
        m_compressionOptionCombo.set_sensitive(true);
        m_fmsControlOptionCombo.set_sensitive(true);

	    /* display */

	m_viewerTypeCombo.set_sensitive(true);
	if (m_block->currentViewerType() == VIEWERTYPE_WATERFALL ||
		m_block->currentViewerType() == VIEWERTYPE_COMBO) {
	    m_redBandSpinButton.set_sensitive(true);
	    m_greenBandSpinButton.set_sensitive(true);
	    m_blueBandSpinButton.set_sensitive(true);
	    m_downtrackSkipSpinButton.set_sensitive(true);
	}
	m_stretchMinSpinButton.set_sensitive(true);
	m_stretchMaxSpinButton.set_sensitive(true);
	m_greenGainSpinButton.set_sensitive(true);
	m_blueGainSpinButton.set_sensitive(true);
        m_darkSubOptionCombo.set_sensitive(true);
	if (m_block->currentDarkSubOption() == DARKSUBOPTION_YES) {
	    m_darkSubDirCombo.set_sensitive(true);
	    m_darkSubDirButton.set_sensitive(true);
	}
        m_navDurationCombo.set_sensitive(true);
	m_reflectionOptionCombo.set_sensitive(true);

	    /* data storage */

        m_recMarginCombo.set_sensitive(true);
        m_prefixEntry.set_sensitive(true);
        m_productRootDirCombo.set_sensitive(true);
        m_productRootDirButton.set_sensitive(true);

	    /* calibration */

	m_obcInterfaceCombo.set_sensitive(true);
	if (m_block->currentOBCInterface() != OBCINTERFACE_NONE) {
	    m_dark1CalPeriodCombo.set_sensitive(true);
	    m_dark2CalPeriodCombo.set_sensitive(true);
	    m_mediumCalPeriodCombo.set_sensitive(true);
	    m_brightCalPeriodCombo.set_sensitive(true);
	    m_laserCalPeriodCombo.set_sensitive(true);
	}

	    /* shutter */

	m_shutterInterfaceCombo.set_sensitive(true);

	    /* GPS */

	m_gpsSerialBaudCombo.set_sensitive(true);
        m_gpsSerialParityCombo.set_sensitive(true);
        m_gpsSerialTXInvertCombo.set_sensitive(true);
        m_gpsInitModeCombo.set_sensitive(true);

	    /* ECS connection */

	m_ecsConnectedCombo.set_sensitive(true);
	if (m_block->currentECSConnected() == ECSCONNECTED_YES) {
	    m_ecsHostEntry.set_sensitive(true);
	    m_ecsPortSpinButton.set_sensitive(true);
	    m_ecsTimeoutInMsSpinButton.set_sensitive(true);
	    m_ecsUsernameEntry.set_sensitive(true);
	    m_ecsPasswordEntry.set_sensitive(true);
	}

	    /* mount */

        m_mountUsedCombo.set_sensitive(true);
	if (m_block->currentMountUsed() == MOUNTUSED_YES)
	    m_mountDevEntry.set_sensitive(true);

	    /* headless operation */

        m_headlessDevEntry.set_sensitive(true);

	    /* sim */

	m_imageSimCombo.set_sensitive(true);
	m_gpsSimCombo.set_sensitive(true);
        m_diagOptionCombo.set_sensitive(true);
    }
}


SettingsTab::~SettingsTab()
{
    m_block = NULL;
    m_displayTab = NULL;
    m_diagTab = NULL;
    m_plotsTab = NULL;
    m_ecsDataTab = NULL;
}


void SettingsTab::handleDarkSubDirBrowse(const char *prompt,
    Gtk::ComboBoxText *combo) const
{
    u_int len;
    char *scopy;
    Gtk::ResponseType result;
  
    Gtk::FileChooserDialog d(prompt,
	Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
    (void)d.add_button(Gtk::Stock::CANCEL, (int)Gtk::RESPONSE_CANCEL);
    (void)d.add_button("Select", (int)Gtk::RESPONSE_OK);

    result = (Gtk::ResponseType)d.run();
    switch (result) {
	case Gtk::RESPONSE_OK:
	    len = strlen(d.get_filename().c_str());
	    scopy = new char[len+1];
	    strcpy(scopy, d.get_filename().c_str());

	    combo->remove_text(scopy);
	    combo->prepend_text(scopy);
	    combo->set_active_text(scopy);

	    m_block->insertInDarkSubDirMRU(combo->get_active_text().c_str());
	    break;
	case Gtk::RESPONSE_CANCEL:
	    break;
	default:
	    m_displayTab->warn("Internal error: Unexpected button click");
	    break;
    } /*lint !e788 ignore other possible responses */
}


void SettingsTab::handleProductRootDirBrowse(const char *prompt,
    Gtk::ComboBoxText *combo) const
{
    u_int len;
    char *scopy;
    Gtk::ResponseType result;
  
    Gtk::FileChooserDialog d(prompt,
	Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
    (void)d.add_button(Gtk::Stock::CANCEL, (int)Gtk::RESPONSE_CANCEL);
    (void)d.add_button("Select", (int)Gtk::RESPONSE_OK);

    result = (Gtk::ResponseType)d.run();
    switch (result) {
	case Gtk::RESPONSE_OK:
	    len = strlen(d.get_filename().c_str());
	    scopy = new char[len+1];
	    strcpy(scopy, d.get_filename().c_str());

	    combo->remove_text(scopy);
	    combo->prepend_text(scopy);
	    combo->set_active_text(scopy);

	    m_block->insertInProductRootDirMRU(
		combo->get_active_text().c_str());
	    break;
	case Gtk::RESPONSE_CANCEL:
	    break;
	default:
	    m_displayTab->warn("Internal error: Unexpected button click");
	    break;
    } /*lint !e788 ignore other possible responses */
}


void SettingsTab::updateRedBandSelection(int red, bool updateControl)
{
    m_block->setRedBand(red);
    if (updateControl)
	m_redBandSpinButton.set_value((double)red);
}


void SettingsTab::updateGreenBandSelection(int green, bool updateControl)
{
    m_block->setGreenBand(green);
    if (updateControl)
	m_greenBandSpinButton.set_value((double)green);
}


void SettingsTab::updateBlueBandSelection(int blue, bool updateControl)
{
    m_block->setBlueBand(blue);
    if (updateControl)
	m_blueBandSpinButton.set_value((double)blue);
}


void SettingsTab::updateDowntrackSkipSelection(int skip, bool updateControl)
{
    m_block->setDowntrackSkip(skip);
    if (updateControl)
	m_downtrackSkipSpinButton.set_value((double)skip);
}


void SettingsTab::updateStretchMinSelection(int min, bool updateControl)
{
    m_block->setStretchMin(min);
    if (updateControl)
	m_stretchMinSpinButton.set_value((double)min);
}


void SettingsTab::updateStretchMaxSelection(int max, bool updateControl)
{
    m_block->setStretchMax(max);
    if (updateControl)
	m_stretchMaxSpinButton.set_value((double)max);
}


void SettingsTab::updateGreenGainSelection(double value, bool updateControl)
{
    m_block->setGreenGain(value);
    if (updateControl)
	m_greenGainSpinButton.set_value(value);
}


void SettingsTab::updateBlueGainSelection(double value, bool updateControl)
{
    m_block->setBlueGain(value);
    if (updateControl)
	m_blueGainSpinButton.set_value(value);
}
