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


class RecordingsTab : public Gtk::HBox
{
protected:
	/* pointer to the settings block */

    SettingsBlock *m_block;

	/* pointer to display tab */

    DisplayTab *m_displayTab;

	/* variables */

    int m_numFlightLines;

	/* GUI elements */

    Gtk::TreeModel::ColumnRecord m_record;

    Gtk::TreeModelColumn<Glib::ustring> m_typeColumn;
    Gtk::TreeModelColumn<Glib::ustring> m_imagepathColumn;
    Gtk::TreeModelColumn<Glib::ustring> m_imagefileColumn;
    Gtk::TreeModelColumn<Glib::ustring> m_imagehdrpathColumn;
    Gtk::TreeModelColumn<Glib::ustring> m_imagehdrfileColumn;
    Gtk::TreeModelColumn<Glib::ustring> m_gpspathColumn;
    Gtk::TreeModelColumn<Glib::ustring> m_gpsfileColumn;
    Gtk::TreeModelColumn<Glib::ustring> m_ppspathColumn;
    Gtk::TreeModelColumn<Glib::ustring> m_ppsfileColumn;
    Gtk::TreeModelColumn<Glib::ustring> m_diagspathColumn;
    Gtk::TreeModelColumn<Glib::ustring> m_diagsfileColumn;
    Gtk::TreeModelColumn<Glib::ustring> m_logpathColumn;
    Gtk::TreeModelColumn<Glib::ustring> m_logfileColumn;
    Gtk::TreeModelColumn<Glib::ustring> m_startTimeColumn;
    Gtk::TreeModelColumn<Glib::ustring> m_endTimeColumn;
    Gtk::TreeModelColumn<Glib::ustring> m_durationColumn;
    Gtk::TreeModelColumn<time_t> m_durationsecsColumn;
    Gtk::TreeModelColumn<int> m_recordingTypeColumn;

    Glib::RefPtr<Gtk::ListStore> m_model;
    Gtk::TreeView m_list;
    Glib::RefPtr<Gtk::TreeSelection> m_selection;
    Gtk::ScrolledWindow m_window;

    Gtk::VButtonBox m_buttonBox;
    Gtk::Button m_deleteButton;

	/* handlers */

    void onSelection(void);
    void onDeleteButton(void);

	/* misc support routines */

    void scanDirs(char *productRootDir, recording_t **recordings_p,
	int *nRecordings_p) const;
    int checkForEmptyDirs(char *productRootDir, int removeThem) const;
public:
    RecordingsTab(SettingsBlock *sb, DisplayTab *dt);
    void addRecording(const recording_t *r);
    void enableRecordingsChanges(void);
    void disableRecordingsChanges(void);
    ~RecordingsTab();
};
