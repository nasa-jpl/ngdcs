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

extern const char *const plotsTabDate = "$Date: 2014/05/29 22:49:43 $";


PlotsTab::PlotsTab(SettingsBlock *sb)
{
    const char *latAndLonNames[2] = { "Lat", "Lon" };
    const char *altitudeName[1] = { "Altitude" };
    const char *velocityName[1] = { "Velocity" };

	/* save pointer to the settings block */

    m_block = sb;

	/* construct plotting objects */

    m_latAndLonPlots = new PlotDisplay("Position", 2,
	latAndLonNames, -180.0, 180.0, m_block->currentNavDurationSecs(), 30);
    m_latAndLonPlots->setColor(0,PlotDisplay::COLOR_RED);
    m_latAndLonPlots->setColor(1,PlotDisplay::COLOR_BLACK);

    m_altitudePlot = new PlotDisplay("Altitude", 1,
	altitudeName, 0.0, 25000.0, m_block->currentNavDurationSecs(), 30);
    m_altitudePlot->setColor(0, PlotDisplay::COLOR_BLUE);

    m_velocityPlot = new PlotDisplay("Velocity", 1,
	velocityName, 0.0, 250.0, m_block->currentNavDurationSecs(), 30);
    m_velocityPlot->setColor(0, PlotDisplay::COLOR_BLUE);

	/* place plots on tab */

    m_row1.set_border_width(10);
    m_row1.set_spacing(15);
    m_row2.set_border_width(10);
    m_row2.set_spacing(15);

    m_row1.pack_start(*m_latAndLonPlots, Gtk::PACK_SHRINK);
    m_row1.pack_start(*m_altitudePlot, Gtk::PACK_SHRINK);
    pack_start(m_row1, Gtk::PACK_SHRINK);

    m_row2.pack_start(*m_velocityPlot, Gtk::PACK_SHRINK);
    pack_start(m_row2, Gtk::PACK_SHRINK);
}


PlotsTab::~PlotsTab()
{
    m_block = NULL;

    delete m_latAndLonPlots;
    delete m_altitudePlot;
    delete m_velocityPlot;
}


void PlotsTab::addLatAndLon(double lat, double lon)
{
    double latAndLon[2];

    latAndLon[0] = lat;
    latAndLon[1] = lon;

    m_latAndLonPlots->addPoint(latAndLon);
}


void PlotsTab::addAltitude(double altitude)
{
    m_altitudePlot->addPoint(&altitude);
}


void PlotsTab::addVelocity(double velocity)
{
    m_velocityPlot->addPoint(&velocity);
}


void PlotsTab::draw(void)
{
    m_latAndLonPlots->draw();
    m_altitudePlot->draw();
    m_velocityPlot->draw();
}


void PlotsTab::resetNavPlots(void)
{
    m_latAndLonPlots->reset();
    m_altitudePlot->reset();
    m_velocityPlot->reset();
}


void PlotsTab::enablePlotChanges(void)
{
}


void PlotsTab::disablePlotChanges(void)
{
}
