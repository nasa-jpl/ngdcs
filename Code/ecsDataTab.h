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


#include <ipc.h>
#include "libftp.h"


class ECSDataTab : public Gtk::VBox
{
protected:
	/* pointer to the settings block */

    SettingsBlock *m_block;

	/* pointers to other tabs */

    DisplayTab *m_displayTab;

	/* variables */

    FTPConnection *m_ftp;
    std::string m_ftpOutput;

	/* GUI elements */

    Gtk::HBox m_listButtonBox;

    Gtk::TreeModel::ColumnRecord m_record;

    Gtk::TreeModelColumn<Glib::ustring> m_nameActualColumn;
    Gtk::TreeModelColumn<Glib::ustring> m_nameShortColumn;
    Gtk::TreeModelColumn<Glib::ustring> m_timeColumn;
    Gtk::TreeModelColumn<Glib::ustring> m_sizeColumn;
    Gtk::TreeModelColumn<int> m_sizeInBytesColumn;

    Glib::RefPtr<Gtk::ListStore> m_model;
    Gtk::TreeView m_list;
    Glib::RefPtr<Gtk::TreeSelection> m_selection;
    Gtk::ScrolledWindow m_listWindow;

    Gtk::VButtonBox m_buttonBox;
    Gtk::Button m_refreshButton;
    Gtk::Button m_exportButton;
    Gtk::Button m_deleteButton;

    Gtk::Label m_ftpLabel;
    Gtk::ScrolledWindow m_ftpWindow;

	/* handlers */

    void onSelection(void);
    void onRefreshButton(void);
    void onExportButton(void);
    void onDeleteButton(void);

	/* misc support routines */

    void addDataset(const FTPConnection::ftpDirEntry_t *ds) const;
    static int showFTPError(void *clientData, const char *msg);
    static void logFTPMessage(void *clientData, const char *msg);
public:
    ECSDataTab(SettingsBlock *sb, DisplayTab *dd);
    void enableECSDataChanges(void);
    void disableECSDataChanges(void);
    ~ECSDataTab();
};
