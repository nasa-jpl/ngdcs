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

extern const char *const plottingDate = "$Date: 2014/05/09 16:55:18 $";


PlotDisplay::PlotDisplay(const char *title, int numPlots,
    const char **plotNames, double min, double max, double xMax, double xIncr)
{
    int i;

    m_title = new char[strlen(title)+1];
    strcpy(m_title, title);

    m_numPlots = numPlots;

    m_color = new color_t[numPlots];
    for (i=0;i < m_numPlots;i++)
	m_color[i] = COLOR_BLACK;

    m_plotName = new char *[numPlots];
    for (i=0;i < m_numPlots;i++) {
	m_plotName[i] = new char[strlen(plotNames[i])+1];
	strcpy(m_plotName[i], plotNames[i]);
    }

    m_min = min;
    m_max = max;

    m_xMax = xMax;
    m_xIncr = xIncr;

    set_size_request(PLOT_AREA_WIDTH, PLOT_AREA_HEIGHT);

    m_pointsPlotted = 0;

    m_x = new int *[numPlots];
    m_y = new int *[numPlots];
    for (i=0;i < m_numPlots;i++) {
	m_x[i] = new int[PLOT_POINTS_MAX];
	(void)memset(m_x[i], 0, PLOT_POINTS_MAX * sizeof(int));
	m_y[i] = new int[PLOT_POINTS_MAX];
	(void)memset(m_y[i], 0, PLOT_POINTS_MAX * sizeof(int));
    }
}


PlotDisplay::~PlotDisplay()
{
    int i;

    delete[] m_title;
    delete[] m_color;
    for (i=0;i < m_numPlots;i++) {
	delete[] m_plotName[i];
	delete[] m_x[i];
	delete[] m_y[i];
    }
    delete[] m_plotName;
    delete[] m_x;
    delete[] m_y;
}


bool PlotDisplay::on_expose_event(GdkEventExpose *e)
{
    int i, p;
    Cairo::TextExtents te;
    char label[20];

    Glib::RefPtr<Gdk::Window> win = get_window();
    if (win) {
	Cairo::RefPtr<Cairo::Context> cr = win->create_cairo_context();

	    /* set clipping window based on what was exposed */

	cr->save();
	cr->set_line_width(1.0);
	cr->set_line_join(Cairo::LINE_JOIN_ROUND);
	cr->rectangle((double)e->area.x, (double)e->area.y,
	    (double)e->area.width, (double)e->area.height);
	cr->clip();

	    /* draw title */

	cr->set_source_rgb(0.0, 0.0, 0.0);
	cr->set_font_size(13);
	cr->get_text_extents(m_title, te);
	cr->move_to(PLOT_MARGIN_LEFT+1+PLOT_POINTS_MAX/2-te.width/2-1,
	    PLOT_MARGIN_TOP-10);
	cr->show_text(m_title);

	    /* draw axes */

	cr->set_source_rgb(0.0, 0.0, 0.0);
	cr->move_to(PLOT_MARGIN_LEFT, PLOT_MARGIN_TOP);
	cr->line_to(PLOT_MARGIN_LEFT, PLOT_MARGIN_TOP+PLOT_HEIGHT);
	cr->line_to(PLOT_MARGIN_LEFT+1+PLOT_POINTS_MAX,
	    PLOT_MARGIN_TOP+PLOT_HEIGHT);
	cr->stroke();
	cr->move_to(PLOT_MARGIN_LEFT, PLOT_MARGIN_TOP);
	cr->line_to(PLOT_MARGIN_LEFT+1+PLOT_POINTS_MAX, PLOT_MARGIN_TOP);
	cr->stroke();

	    /* draw y-axis ticks and reference lines */

	cr->set_source_rgb(0.0, 0.0, 0.0);
	cr->set_font_size(10);
	for (i=0;i <= PLOT_TICKS;i++) {
	    sprintf(label, "%.1f", m_max-i*(m_max-m_min)/PLOT_TICKS);
	    cr->get_text_extents(label, te);
	    cr->move_to(PLOT_MARGIN_LEFT-te.width-7,
		PLOT_MARGIN_TOP + PLOT_HEIGHT / PLOT_TICKS * i + te.height/2);
	    cr->show_text(label);
	    cr->move_to(PLOT_MARGIN_LEFT,
		PLOT_MARGIN_TOP + PLOT_HEIGHT / PLOT_TICKS * i);
	    cr->line_to(PLOT_MARGIN_LEFT-4,
		PLOT_MARGIN_TOP + PLOT_HEIGHT / PLOT_TICKS * i);
	    cr->stroke();
	}
	cr->set_source_rgb(0.6, 0.6, 0.6);
	for (i=0;i <= PLOT_TICKS;i++) {
	    cr->move_to(PLOT_MARGIN_LEFT+1,
		PLOT_MARGIN_TOP + PLOT_HEIGHT / PLOT_TICKS * i);
	    cr->line_to(PLOT_MARGIN_LEFT+1+PLOT_POINTS_MAX,
		PLOT_MARGIN_TOP + PLOT_HEIGHT / PLOT_TICKS * i);
	    cr->stroke();
	}

#if 0
	    /* draw x-axis ticks */

	cr->set_source_rgb(0.0, 0.0, 0.0);
	int xTicks = m_xMax / m_xIncr + 1;
	for (i=0;i < xTicks;i++) {
	    sprintf(label, "%g", m_xIncr * i);
	    cr->get_text_extents(label, te);
	    cr->move_to(
		PLOT_MARGIN_LEFT + 1 + i * PLOT_POINTS_MAX / (xTicks-1) -
		    (te.width/2) - 1,
		PLOT_MARGIN_TOP + PLOT_HEIGHT + 7 + te.height);
	    cr->show_text(label);
	    cr->move_to(PLOT_MARGIN_LEFT + 1 + i * PLOT_POINTS_MAX / (xTicks-1),
		PLOT_MARGIN_TOP + PLOT_HEIGHT);
	    cr->line_to(PLOT_MARGIN_LEFT + 1 + i * PLOT_POINTS_MAX / (xTicks-1),
		PLOT_MARGIN_TOP + PLOT_HEIGHT + 4);
	    cr->stroke();
	}
#endif

	    /* for each plot... */

	cr->set_font_size(12);
	for (p=0;p < m_numPlots;p++) {

		/* set plot color */

	    if (m_color[p] == COLOR_RED)
		cr->set_source_rgb(1.0, 0.0, 0.0);
	    else if (m_color[p] == COLOR_GREEN)
		cr->set_source_rgb(0.0, 1.0, 0.0);
	    else if (m_color[p] == COLOR_BLUE)
		cr->set_source_rgb(0.0, 0.0, 1.0);
	    else /* m_color[p] == COLOR_BLACK */
		cr->set_source_rgb(0.0, 0.0, 0.0);

		/* draw plot */

	    cr->move_to(m_x[p][0], m_y[p][0]);
	    for (i=1;i < m_pointsPlotted;i++)
		cr->line_to(m_x[p][i], m_y[p][i]);
	    cr->stroke();

		/* if multiple plots, draw legend */

	    if (m_numPlots != 1) {
		cr->get_text_extents(m_plotName[p], te);
		cr->move_to(PLOT_MARGIN_LEFT+1+PLOT_POINTS_MAX+10,
		    PLOT_MARGIN_TOP + 30 + (p+1) * (te.height + 10));
		cr->show_text(m_plotName[p]);
		cr->stroke();
	    }
	}
	cr->restore();
    }
    return true;
}


void PlotDisplay::setColor(int plot, color_t color)
{
    if (m_color[plot] != color) {
	m_color[plot] = color;

	Glib::RefPtr<Gdk::Window> win = get_window();
	if (win) {
	    Gdk::Rectangle r(0, 0, get_allocation().get_width(),
		get_allocation().get_height());
	    win->invalidate_rect(r, false);
	}
    }
}


void PlotDisplay::addPoint(double *v)
{
    int x, y, i, p;

    for (p=0;p < m_numPlots;p++) {
	y = PLOT_MARGIN_TOP + PLOT_HEIGHT -
	    (v[p]-m_min)/(m_max-m_min) * PLOT_HEIGHT;
	if (y < PLOT_MARGIN_TOP)
	    y = PLOT_MARGIN_TOP;
	else if (y > PLOT_MARGIN_TOP + PLOT_HEIGHT)
	    y = PLOT_MARGIN_TOP + PLOT_HEIGHT;

	if (m_pointsPlotted < PLOT_POINTS_MAX) {
	    x = PLOT_MARGIN_LEFT + 1 + m_pointsPlotted;
	    m_x[p][m_pointsPlotted] = x;
	    m_y[p][m_pointsPlotted] = y;
	    if (p == m_numPlots-1) m_pointsPlotted++;
	}
	else {
	    x = PLOT_MARGIN_LEFT + 1 + PLOT_POINTS_MAX - 1;
	    for (i=1;i < PLOT_POINTS_MAX;i++) {
		m_x[p][i-1] = m_x[p][i]-1;
		m_y[p][i-1] = m_y[p][i];
	    }
	    m_x[p][PLOT_POINTS_MAX-1] = x;
	    m_y[p][PLOT_POINTS_MAX-1] = y;
	}
    }
}


void PlotDisplay::draw(void)
{
    Glib::RefPtr<Gdk::Window> win = get_window();
    if (win) {
	Cairo::RefPtr<Cairo::Context> cr = win->create_cairo_context();
	Gdk::Rectangle r(0, 0, get_allocation().get_width(),
	    get_allocation().get_height());
	win->invalidate_rect(r, false);
    }
}


void PlotDisplay::reset(void)
{
    m_pointsPlotted = 0;

    Glib::RefPtr<Gdk::Window> win = get_window();
    if (win) {
	Cairo::RefPtr<Cairo::Context> cr = win->create_cairo_context();
	Gdk::Rectangle r(0, 0, get_allocation().get_width(),
	    get_allocation().get_height());
	win->invalidate_rect(r, false);
    }
}
