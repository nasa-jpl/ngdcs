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
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/time.h>
#include <sys/param.h>
#if !defined(__APPLE__)
#include <sys/vfs.h>
#endif
#include <math.h>
#include <gtkmm.h>
#include "main.h"
#include "framebuf.h"
#include "plotting.h"
#include "appFrame.h"
#include "displayTab.h"
#include "diagTab.h"
#include "plotsTab.h"
#include "recordingsTab.h"
#include "ecsDataTab.h"
#include "settingsBlock.h"
#include "settingsTab.h"
#include "maxon.h"

extern const char *const diagTabDate = "$Date: 2015/02/24 20:19:58 $";


DiagTab::DiagTab(SettingsBlock *sb, appFrame *app) :
    m_framebufferStatsTable(4, 2, false),
    m_metadataStatsTable(2, 2, false),
    m_firstLineDataTable(1, 2, false),
    m_tempsTable(5, 2, false),
    m_fpgaRegsTable(8, 4, false)
{
        /* save pointer to the settings block */

    m_block = sb;

	/* save pointer to the app */

    m_app = app;

	/* init */

    m_leftBox.set_border_width(10);
    m_leftBox.set_spacing(15);
    m_rightBox.set_border_width(10);
    m_rightBox.set_spacing(15);
    set_border_width(10);
    set_spacing(15);

	/* set up frame stats */

    m_framebufferStatsTable.set_row_spacings(5);
    m_framebufferStatsTable.set_col_spacings(15);
    m_framebufferStatsTable.set_border_width(10);

    m_framebufferStatsFrame.add(m_framebufferStatsTable);
    m_framebufferStatsFrame.set_label("Framebuffer stats");

    addDisplay(m_framebufferStatsTable, 0, "Count", m_frameCountEntry, "0",
	(m_block->currentDiagOption() == DIAGOPTION_ON? true:false));
    addDisplay(m_framebufferStatsTable, 1, "Dropped", m_framesDroppedEntry, "0",
	(m_block->currentDiagOption() == DIAGOPTION_ON? true:false));
    addDisplay(m_framebufferStatsTable, 2, "Backlog", m_frameBacklogEntry, "0",
	(m_block->currentDiagOption() == DIAGOPTION_ON? true:false));
    addDisplay(m_framebufferStatsTable, 3, "Max backlog",
	m_maxFrameBacklogEntry, "0",
	(m_block->currentDiagOption() == DIAGOPTION_ON? true:false));

    m_leftBox.pack_start(m_framebufferStatsFrame, Gtk::PACK_SHRINK);

	/* set up metadata stats */

    m_metadataStatsTable.set_row_spacings(5);
    m_metadataStatsTable.set_col_spacings(15);
    m_metadataStatsTable.set_border_width(10);

    m_metadataStatsFrame.add(m_metadataStatsTable);
    m_metadataStatsFrame.set_label("Metadata stats");

    addDisplay(m_metadataStatsTable, 0, "Msg3 Count", m_msg3CountEntry, "0",
	(m_block->currentDiagOption() == DIAGOPTION_ON? true:false));
    addDisplay(m_metadataStatsTable, 1, "GPS Count", m_gpsCountEntry, "0",
	(m_block->currentDiagOption() == DIAGOPTION_ON? true:false));

    m_leftBox.pack_start(m_metadataStatsFrame, Gtk::PACK_SHRINK);

	/* set up first line data */

    m_firstLineDataTable.set_row_spacings(5);
    m_firstLineDataTable.set_col_spacings(15);
    m_firstLineDataTable.set_border_width(10);

    m_firstLineDataFrame.add(m_firstLineDataTable);
    m_firstLineDataFrame.set_label("First-line data");

    addDisplay(m_firstLineDataTable, 0, "Local Frame Count",
	m_localFrameCountEntry, "0",
	(m_block->currentDiagOption() == DIAGOPTION_ON? true:false));

    m_leftBox.pack_start(m_firstLineDataFrame, Gtk::PACK_SHRINK);

	/* set up temp displays */

    m_tempsTable.set_row_spacings(5);
    m_tempsTable.set_col_spacings(15);
    m_tempsTable.set_border_width(10);

    m_tempsFrame.add(m_tempsTable);
    m_tempsFrame.set_label("Temps");

    m_temp1Label.set_label("Unused");
    m_temp2Label.set_label("Unused");
    m_temp3Label.set_label("Unused");
    m_temp4Label.set_label("Unused");
    m_temp5Label.set_label("Unused");
    addDisplay(m_tempsTable, 0, m_temp1Label, m_temp1Entry, "0",
	(m_block->currentDiagOption() == DIAGOPTION_ON? true:false));
    addDisplay(m_tempsTable, 1, m_temp2Label, m_temp2Entry, "0",
	(m_block->currentDiagOption() == DIAGOPTION_ON? true:false));
    addDisplay(m_tempsTable, 2, m_temp3Label, m_temp3Entry, "0",
	(m_block->currentDiagOption() == DIAGOPTION_ON? true:false));
    addDisplay(m_tempsTable, 3, m_temp4Label, m_temp4Entry, "0",
	(m_block->currentDiagOption() == DIAGOPTION_ON? true:false));
    addDisplay(m_tempsTable, 4, m_temp5Label, m_temp5Entry, "0",
	(m_block->currentDiagOption() == DIAGOPTION_ON? true:false));

    m_leftBox.pack_start(m_tempsFrame, Gtk::PACK_SHRINK);

	/* set up FPGA register displays */

    m_fpgaRegsTable.set_row_spacings(5);
    m_fpgaRegsTable.set_col_spacings(15);
    m_fpgaRegsTable.set_border_width(10);

    m_fpgaRegsFrame.add(m_fpgaRegsTable);
    m_fpgaRegsFrame.set_label("FPGA Regs");

    addDisplay(m_fpgaRegsTable, 0, "Frame Count", m_fpgaFrameCountEntry,
	"0", (m_block->currentDiagOption() == DIAGOPTION_ON? true:false));
    addDisplay(m_fpgaRegsTable, 1, "PPS Count", m_fpgaPPSCountEntry,
	"0", (m_block->currentDiagOption() == DIAGOPTION_ON? true:false));
    addDisplay(m_fpgaRegsTable, 2, "81FF Count", m_fpga81ffCountEntry,
	"0", (m_block->currentDiagOption() == DIAGOPTION_ON? true:false));
    addDisplay(m_fpgaRegsTable, 3, "Frames Dropped", m_fpgaFramesDroppedEntry,
	"0", (m_block->currentDiagOption() == DIAGOPTION_ON? true:false));

    addDisplay(m_fpgaRegsTable, 4, "Serial Errors", m_fpgaSerialErrorsEntry,
	"0", (m_block->currentDiagOption() == DIAGOPTION_ON? true:false));
    m_fpgaSerialErrorsButton.set_label("Clear");
    m_fpgaRegsTable.attach(m_fpgaSerialErrorsButton, 2, 3, 4, 5,
	(Gtk::AttachOptions)0, (Gtk::AttachOptions)0);
    m_fpgaSerialErrorsButton.set_sensitive(true);
    (void)m_fpgaSerialErrorsButton.signal_clicked().connect(sigc::mem_fun(*this,
	&DiagTab::onFPGASerialErrorsButton));

    addDisplay(m_fpgaRegsTable, 5, "Buffer Depth", m_fpgaBufferDepthEntry,
	"0", (m_block->currentDiagOption() == DIAGOPTION_ON? true:false));

    addDisplay(m_fpgaRegsTable, 6, "Max Buffers Used", m_fpgaMaxBuffersUsedEntry,
	"0", (m_block->currentDiagOption() == DIAGOPTION_ON? true:false));
    m_fpgaMaxBuffersUsedButton.set_label("Clear");
    m_fpgaRegsTable.attach(m_fpgaMaxBuffersUsedButton, 2, 3, 6, 7,
	(Gtk::AttachOptions)0, (Gtk::AttachOptions)0);
    m_fpgaMaxBuffersUsedButton.set_sensitive(true);
    (void)m_fpgaMaxBuffersUsedButton.signal_clicked().connect(sigc::mem_fun(*this,
	&DiagTab::onFPGAMaxBuffersUsedButton));

    addDisplay(m_fpgaRegsTable, 7, "Serial Port Ctl", m_fpgaSerialPortCtlEntry,
	"0", (m_block->currentDiagOption() == DIAGOPTION_ON? true:false));

    m_rightBox.pack_start(m_fpgaRegsFrame, Gtk::PACK_SHRINK);

	/* just place left and right columns and we're done */

    m_screenBox.pack_start(m_leftBox);
    m_screenBox.pack_start(m_rightBox);
    pack_start(m_screenBox);
}


void DiagTab::addDisplay(Gtk::Table &table, u_int row, const char *descr,
    Gtk::Entry &entry, const char *initialValue, bool enabled) const
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
    entry.set_editable(false);
}


void DiagTab::addDisplay(Gtk::Table &table, u_int row, Gtk::Label &label,
    Gtk::Entry &entry, const char *initialValue, bool enabled) const
{
    Gtk::Alignment *a;

    entry.set_text(initialValue);
    a = new Gtk::Alignment(Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, (float)0.0,
	(float)0.0);
    a->add(label);
    table.attach(*a, 0, 1, row, row+1, (Gtk::AttachOptions)Gtk::FILL,
	(Gtk::AttachOptions)0);
    table.attach(entry, 1, 2, row, row+1, (Gtk::AttachOptions)0,
	(Gtk::AttachOptions)0);
    entry.set_sensitive(enabled);
    entry.set_editable(false);
}


DiagTab::~DiagTab()
{
	/* misc stuff */

    m_block = NULL;
    m_app = NULL;
}


void DiagTab::disableDiagChanges(void)
{
    m_frameCountEntry.set_sensitive(false);
    m_framesDroppedEntry.set_sensitive(false);
    m_frameBacklogEntry.set_sensitive(false);
    m_maxFrameBacklogEntry.set_sensitive(false);
    m_msg3CountEntry.set_sensitive(false);
    m_gpsCountEntry.set_sensitive(false);
    m_localFrameCountEntry.set_sensitive(false);
    m_temp1Entry.set_sensitive(false);
    m_temp2Entry.set_sensitive(false);
    m_temp3Entry.set_sensitive(false);
    m_temp4Entry.set_sensitive(false);
    m_temp5Entry.set_sensitive(false);
    m_fpgaFrameCountEntry.set_sensitive(false);
    m_fpgaPPSCountEntry.set_sensitive(false);
    m_fpga81ffCountEntry.set_sensitive(false);
    m_fpgaFramesDroppedEntry.set_sensitive(false);
    m_fpgaSerialErrorsEntry.set_sensitive(false);
    m_fpgaSerialErrorsButton.set_sensitive(false);
    m_fpgaSerialPortCtlEntry.set_sensitive(false);
    m_fpgaBufferDepthEntry.set_sensitive(false);
    m_fpgaMaxBuffersUsedEntry.set_sensitive(false);
    m_fpgaMaxBuffersUsedButton.set_sensitive(false);
}


void DiagTab::enableDiagChanges(void)
{
    if (m_block->currentDiagOption() == DIAGOPTION_ON) {
	m_frameCountEntry.set_sensitive(true);
	m_framesDroppedEntry.set_sensitive(true);
	m_frameBacklogEntry.set_sensitive(true);
	m_maxFrameBacklogEntry.set_sensitive(true);
	m_msg3CountEntry.set_sensitive(true);
	m_gpsCountEntry.set_sensitive(true);
	m_localFrameCountEntry.set_sensitive(true);
	m_temp1Entry.set_sensitive(true);
	m_temp2Entry.set_sensitive(true);
	m_temp3Entry.set_sensitive(true);
	m_temp4Entry.set_sensitive(true);
	m_temp5Entry.set_sensitive(true);
	m_fpgaFrameCountEntry.set_sensitive(true);
	m_fpgaPPSCountEntry.set_sensitive(true);
	m_fpga81ffCountEntry.set_sensitive(true);
	m_fpgaFramesDroppedEntry.set_sensitive(true);
	m_fpgaSerialErrorsEntry.set_sensitive(true);
	m_fpgaSerialErrorsButton.set_sensitive(true);
	m_fpgaSerialPortCtlEntry.set_sensitive(true);
	m_fpgaBufferDepthEntry.set_sensitive(true);
	m_fpgaMaxBuffersUsedEntry.set_sensitive(true);
	m_fpgaMaxBuffersUsedButton.set_sensitive(true);
    }
}


void DiagTab::setFrameCount(int n)
{
    char buf[30];
    if (m_block->currentDiagOption() == DIAGOPTION_ON) {
	(void)sprintf(buf, "%d", n);
	m_frameCountEntry.set_text(buf);
    }
}


void DiagTab::setFramesDropped(int n)
{
    char buf[30];
    if (m_block->currentDiagOption() == DIAGOPTION_ON) {
	(void)sprintf(buf, "%d", n);
	m_framesDroppedEntry.set_text(buf);
    }
}


void DiagTab::setFrameBacklog(int n)
{
    char buf[30];
    if (m_block->currentDiagOption() == DIAGOPTION_ON) {
	(void)sprintf(buf, "%d", n);
	m_frameBacklogEntry.set_text(buf);
    }
}


void DiagTab::setMaxFrameBacklog(int n)
{
    char buf[30];
    if (m_block->currentDiagOption() == DIAGOPTION_ON) {
	(void)sprintf(buf, "%d", n);
	m_maxFrameBacklogEntry.set_text(buf);
    }
}


void DiagTab::setMsg3Count(int n)
{
    char buf[30];
    if (m_block->currentDiagOption() == DIAGOPTION_ON) {
	(void)sprintf(buf, "%d", n);
	m_msg3CountEntry.set_text(buf);
    }
}


void DiagTab::setGPSCount(int n)
{
    char buf[30];
    if (m_block->currentDiagOption() == DIAGOPTION_ON) {
	(void)sprintf(buf, "%d", n);
	m_gpsCountEntry.set_text(buf);
    }
}


void DiagTab::setLocalFrameCount(int n)
{
    char buf[30];
    if (m_block->currentDiagOption() == DIAGOPTION_ON) {
	(void)sprintf(buf, "%d", n);
	m_localFrameCountEntry.set_text(buf);
    }
}


void DiagTab::setTemps(int numSensors, char **sensorNames, double *sensorValues)
{
    char buf[30];

    if (m_block->currentDiagOption() == DIAGOPTION_ON) {
	if (numSensors >= 1) {
	    m_temp1Label.set_label(sensorNames[0]);
	    (void)sprintf(buf, "%.1f", sensorValues[0]);
	    m_temp1Entry.set_text(buf);
	}
	if (numSensors >= 2) {
	    m_temp2Label.set_label(sensorNames[1]);
	    (void)sprintf(buf, "%.1f", sensorValues[1]);
	    m_temp2Entry.set_text(buf);
	}
	if (numSensors >= 3) {
	    m_temp3Label.set_label(sensorNames[2]);
	    (void)sprintf(buf, "%.1f", sensorValues[2]);
	    m_temp3Entry.set_text(buf);
	}
	if (numSensors >= 4) {
	    m_temp4Label.set_label(sensorNames[3]);
	    (void)sprintf(buf, "%.1f", sensorValues[3]);
	    m_temp4Entry.set_text(buf);
	}
	if (numSensors >= 5) {
	    m_temp5Label.set_label(sensorNames[4]);
	    (void)sprintf(buf, "%.1f", sensorValues[4]);
	    m_temp5Entry.set_text(buf);
	}
    }
}


void DiagTab::setFPGAFrameCount(int n)
{
    char buf[30];
    if (m_block->currentDiagOption() == DIAGOPTION_ON) {
	(void)sprintf(buf, "%d", n);
	m_fpgaFrameCountEntry.set_text(buf);
    }
}


void DiagTab::setFPGAPPSCount(int n)
{
    char buf[30];
    if (m_block->currentDiagOption() == DIAGOPTION_ON) {
	(void)sprintf(buf, "%d", n);
	m_fpgaPPSCountEntry.set_text(buf);
    }
}


void DiagTab::setFPGA81ffCount(int n)
{
    char buf[30];
    if (m_block->currentDiagOption() == DIAGOPTION_ON) {
	(void)sprintf(buf, "%d", n);
	m_fpga81ffCountEntry.set_text(buf);
    }
}


void DiagTab::setFPGAFramesDropped(int n)
{
    char buf[30];
    if (m_block->currentDiagOption() == DIAGOPTION_ON) {
	(void)sprintf(buf, "%d", n);
	m_fpgaFramesDroppedEntry.set_text(buf);
    }
}


void DiagTab::setFPGASerialErrors(int n)
{
    char buf[30];
    if (m_block->currentDiagOption() == DIAGOPTION_ON) {
	(void)sprintf(buf, "%d", n);
	m_fpgaSerialErrorsEntry.set_text(buf);
    }
}


void DiagTab::onFPGASerialErrorsButton()
{
    appFrame::fb->clearSerialErrors();
}


void DiagTab::setFPGASerialPortCtl(int n)
{
    char buf[30];
    if (m_block->currentDiagOption() == DIAGOPTION_ON) {
	(void)sprintf(buf, "%X", n);
	m_fpgaSerialPortCtlEntry.set_text(buf);
    }
}


void DiagTab::setFPGABufferDepth(int n)
{
    char buf[30];
    if (m_block->currentDiagOption() == DIAGOPTION_ON) {
	(void)sprintf(buf, "%d", n);
	m_fpgaBufferDepthEntry.set_text(buf);
    }
}


void DiagTab::setFPGAMaxBuffersUsed(int n)
{
    char buf[30];
    if (m_block->currentDiagOption() == DIAGOPTION_ON) {
	(void)sprintf(buf, "%d", n);
	m_fpgaMaxBuffersUsedEntry.set_text(buf);
    }
}


void DiagTab::onFPGAMaxBuffersUsedButton()
{
    appFrame::fb->clearFPGAMaxBuffersUsed();
}
