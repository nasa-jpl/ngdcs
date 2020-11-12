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


class PlotsTab: public Gtk::VBox
{
protected:

	/* pointer to settings block */

    SettingsBlock *m_block;

	/* GUI elements */

    Gtk::HBox m_row1;
    Gtk::HBox m_row2;
	
	/* plot objects */

    PlotDisplay *m_latAndLonPlots;
    PlotDisplay *m_altitudePlot;
    PlotDisplay *m_velocityPlot;
public:
    PlotsTab(SettingsBlock *sb);

    void addLatAndLon(double lat, double lon);
    void addAltitude(double altitude);
    void addVelocity(double velocity);
    void draw(void);
    void resetNavPlots(void);

    void disablePlotChanges(void);
    void enablePlotChanges(void);

    ~PlotsTab();
};
