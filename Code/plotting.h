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

#define PLOT_POINTS_MAX		300
#define PLOT_TICKS		10

#define PLOT_MARGIN_TOP		20
#define PLOT_HEIGHT		200
#define PLOT_MARGIN_BOTTOM	10
#define PLOT_MARGIN_LEFT	50
#define PLOT_MARGIN_RIGHT	45

#define PLOT_AREA_WIDTH		(PLOT_MARGIN_LEFT + 1 + PLOT_POINTS_MAX + \
				    PLOT_MARGIN_RIGHT)
#define PLOT_AREA_HEIGHT	(PLOT_MARGIN_TOP + PLOT_HEIGHT + \
				    PLOT_MARGIN_BOTTOM)


class PlotDisplay: public Gtk::DrawingArea
{
public:

	/* types */

    typedef enum {
	COLOR_BLACK, COLOR_RED, COLOR_GREEN, COLOR_BLUE
    } color_t;
protected:

	/* variables */

    char *m_title;
    double m_min, m_max;
    double m_xMax, m_xIncr;
    int m_numPlots;
    color_t *m_color;
    char **m_plotName;
    int **m_x, **m_y;
    int m_pointsPlotted;

	/* implementation routines */

    virtual bool on_expose_event(GdkEventExpose *event);
public:

	/* routines */

    PlotDisplay(const char *title, int numPlots, const char **plotNames,
	double min, double max, double xMax, double xIncr);
    void setColor(int plot, color_t color);
    void addPoint(double *v);
    void draw(void);
    void reset(void);
    virtual ~PlotDisplay();
};
