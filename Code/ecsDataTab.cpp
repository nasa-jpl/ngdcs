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


#include <assert.h>
#include <dirent.h>
#include <math.h>
#include <sys/mount.h>
#include <sys/param.h>
#include <gtkmm.h>
#include "main.h"
#include "framebuf.h"
#include "plotting.h"
#include "appFrame.h"
#include "displayTab.h"
#include "recordingsTab.h"
#include "ecsDataTab.h"
#include "settingsBlock.h"
#include "settingsTab.h"

extern const char *const ecsDataTabDate = "$Date: 2014/05/09 16:35:34 $";


ECSDataTab::ECSDataTab(SettingsBlock *sb, DisplayTab *dd) :
    m_ftpLabel("", Gtk::ALIGN_LEFT)
{
	/* init member variables */

    m_block = sb;
    m_displayTab = dd;
    m_ftp = NULL;

	/* construct the GUI */

    set_border_width(10);
    set_spacing(25);
    m_listButtonBox.set_spacing(25);

	/* add columns to the record -- some columns are for internal
	   use and won't be displayed */

    m_record.add(m_nameActualColumn);
    m_record.add(m_nameShortColumn);
    m_record.add(m_timeColumn);
    m_record.add(m_sizeColumn);
    m_record.add(m_sizeInBytesColumn);

	/* create model around the record */

    m_model = Gtk::ListStore::create(m_record);
    m_list.set_model(m_model);

	/* add columns to the list */

    (void)m_list.append_column("Name", m_nameShortColumn);
    (void)m_list.append_column("Last written", m_timeColumn);
    (void)m_list.append_column("Size", m_sizeColumn);

    Gtk::TreeViewColumn *col = m_list.get_column(0);
    col->set_min_width(230);
    col = m_list.get_column(1);
    col->set_min_width(180);
    col = m_list.get_column(2);
    col->set_min_width(50);

	/* catch selections */

    m_selection = m_list.get_selection();
    m_selection->set_mode(Gtk::SELECTION_MULTIPLE);
    (void)m_selection->signal_changed().connect(sigc::mem_fun(*this,
	&ECSDataTab::onSelection));

	/* put the list in a scrolling window and add the window */

    m_listWindow.add(m_list);
    m_listWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    m_listButtonBox.pack_start(m_listWindow);

	/* list is disabled until refresh */

    m_list.set_sensitive(false);

	/* add the buttons */

    m_buttonBox.set_spacing(10);

    m_refreshButton.set_label("Refresh");
    m_refreshButton.set_sensitive(true);
    (void)m_refreshButton.signal_clicked().connect(sigc::mem_fun(*this,
	&ECSDataTab::onRefreshButton));
    m_buttonBox.set_layout(Gtk::BUTTONBOX_START);
    m_buttonBox.add(m_refreshButton);
    if (m_block->currentECSConnected() != ECSCONNECTED_YES)
	m_refreshButton.set_sensitive(false);

    m_exportButton.set_label("Export");
    m_exportButton.set_sensitive(false);
    (void)m_exportButton.signal_clicked().connect(sigc::mem_fun(*this,
	&ECSDataTab::onExportButton));
    m_buttonBox.set_layout(Gtk::BUTTONBOX_START);
    m_buttonBox.add(m_exportButton);

    m_deleteButton.set_label("Delete");
    m_deleteButton.set_sensitive(false);
    (void)m_deleteButton.signal_clicked().connect(sigc::mem_fun(*this,
	&ECSDataTab::onDeleteButton));
    m_buttonBox.set_layout(Gtk::BUTTONBOX_START);
    m_buttonBox.add(m_deleteButton);
    m_listButtonBox.pack_start(m_buttonBox, Gtk::PACK_SHRINK);
    pack_start(m_listButtonBox);

	/* add ftp window */

    m_ftpLabel.set_label("");
    m_ftpWindow.set_size_request(100, 100);
    m_ftpWindow.add(m_ftpLabel);
    m_ftpWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_ALWAYS);
    pack_start(m_ftpWindow, Gtk::PACK_SHRINK);
}


int ECSDataTab::showFTPError(void *clientData, const char *msg)
{
    ECSDataTab *me = (ECSDataTab *)clientData;

    me->m_displayTab->error(msg);
    return -1;
}


void ECSDataTab::logFTPMessage(void *clientData, const char *msg)
{
    ECSDataTab *me = (ECSDataTab *)clientData;

    Gtk::Adjustment *adj;

    (void)me->m_ftpOutput.append(msg);
    (void)me->m_ftpOutput.append("\n");
    me->m_ftpLabel.set_text(me->m_ftpOutput);

    adj = me->m_ftpWindow.get_vadjustment();
    adj->set_value(adj->get_upper());
    while (Gtk::Main::events_pending())
	(void)Gtk::Main::iteration();
}


ECSDataTab::~ECSDataTab()
{
    m_block = NULL;
    m_displayTab = NULL;
    if (m_ftp != NULL)
	delete m_ftp; /*lint !e1551 may throw exception */
}


void ECSDataTab::disableECSDataChanges(void)
{
    m_list.set_sensitive(false);
    m_selection->unselect_all(); /* selected lines are invisible otherwise */
    m_refreshButton.set_sensitive(false);
    m_exportButton.set_sensitive(false);
    m_deleteButton.set_sensitive(false);
}


void ECSDataTab::enableECSDataChanges(void)
{
    if (m_block->currentECSConnected() == ECSCONNECTED_YES) {
	m_list.set_sensitive(true);
	m_refreshButton.set_sensitive(true);
	m_exportButton.set_sensitive(false);
	m_deleteButton.set_sensitive(false);
    }
}


void ECSDataTab::onSelection(void)
{
    m_exportButton.set_sensitive(true);
    m_deleteButton.set_sensitive(true);
}


void ECSDataTab::onRefreshButton(void)
{
    int i, nDatasets;
    FTPConnection::ftpDirEntry_t *datasets;
    char logmsg[200];

	/* log */

    sprintf(logmsg, "User pressed ECS Refresh button.");
    m_displayTab->log(logmsg);

	/* clear out our current listing */

    m_model->clear();

	/* drop any previous connection in case parameters have changed */

    if (m_ftp != NULL) {
	delete m_ftp;
	m_ftp = NULL;
    }

	/* connect to the FTP server */

    m_ftp = new FTPConnection(m_block->currentECSHost(),
	m_block->currentECSPort(), m_block->currentECSUsername(),
	m_block->currentECSPassword(), m_block->currentECSTimeoutInMs(),
	ECSDataTab::showFTPError, ECSDataTab::logFTPMessage, this);
    if (m_ftp->failed) {
	delete m_ftp;
	m_ftp = NULL;
        return;
    }

	/* switch to the ECS directory */

    if (m_ftp->FTPChdir("ECS") == -1) {
	delete m_ftp;
	m_ftp = NULL;
        return;
    }

	/* refresh our listing */

    if (m_ftp->FTPList(&datasets, &nDatasets) != -1) {
	for (i=0;i < nDatasets;i++)
	    addDataset(&datasets[i]);
	delete[] datasets;
	m_list.set_sensitive(true);
    }
}


void ECSDataTab::addDataset(const FTPConnection::ftpDirEntry_t *ds) const
{
    char *temp;

    Gtk::TreeModel::Row row = *(m_model->append());
    
    temp = new char[strlen(ds->name)+1];
    strcpy(temp, ds->name);
    if (strncmp(temp, "NGIS", 4) == 0)
	(void)strcpy(temp, temp+4);
    row[m_nameShortColumn] = temp;

    temp = new char[strlen(ds->name)+1]; /*lint !e423 memory leak */
    strcpy(temp, ds->name);
    row[m_nameActualColumn] = temp;

    temp = new char[MAX_TIME_LEN]; /*lint !e423 memory leak */
    (void)strftime(temp, MAX_TIME_LEN, "%Y/%m/%d %H:%M:%S", &ds->time);
    row[m_timeColumn] = temp;

    temp = new char[11]; /*lint !e423 memory leak */
    sprintf(temp, "%d", ds->size);
    row[m_sizeColumn] = temp;

    row[m_sizeInBytesColumn] = ds->size;
}


void ECSDataTab::onExportButton(void)
{
    int numDatasets, size;
    char warningMsg[MAXPATHLEN+MAX_ERROR_LEN];
    struct stat statbuf;
    const char *name;
    char remoteName[MAXPATHLEN];
    char localName[MAXPATHLEN];
    char logmsg[MAXPATHLEN+100];

	/* log */

    sprintf(logmsg, "User pressed ECS Export button.");
    m_displayTab->log(logmsg);

	/* make sure we have an active FTP connection */

    if (!m_ftp) {
	m_displayTab->error("FTP connection has been lost.  Refresh, please!");
        return;
    }

        /* make sure export directory exists */

    if (stat(appFrame::dailyDir, &statbuf) == -1) {
        sprintf(warningMsg,
            "Can't access data directory \"%s\".  "
	    "You'll need to FTP the data manually.", appFrame::dailyDir);
	m_displayTab->error(warningMsg);
        return;
    }

	/* init */

    typedef Gtk::TreeModel::Children children_t;
    children_t c = m_model->children();

    numDatasets = 0;
    size = 0;

	/* count number of selected items */

    for (children_t::iterator iter = c.begin(); iter != c.end(); ++iter)
	if (m_selection->is_selected(iter)) {
	    Gtk::TreeModel::Row row = *iter;
	    size += row[m_sizeInBytesColumn];
	    numDatasets++;
	}
    if (numDatasets == 0) {
	m_displayTab->error(
	    "Internal error: Can't determine selected ECS data files.");
        return;
    }

        /* give user chance to bail */

    sprintf(warningMsg,
        "You're about to export %d ECS data file(s), "
	    "representing %.1f MB.\n\n",
	numDatasets, size / (double)BYTES_PER_MB);
    Gtk::MessageDialog d(warningMsg, false /* no markup */,
	Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK_CANCEL);
    if ((Gtk::ResponseType)d.run() != Gtk::RESPONSE_OK) {
	sprintf(logmsg, "User cancelled export.");
	m_displayTab->log(logmsg);
	return;
    }
    sprintf(logmsg, "User confirmed export.");
    m_displayTab->log(logmsg);

        /* go ahead and export each file */

    for (children_t::iterator iter = c.begin(); iter != c.end();++iter) {

	    /* if the row is selected... */

	if (m_selection->is_selected(iter)) {

		/* get file name */

	    Gtk::TreeModel::Row row = *iter;

	    Glib::ustring us = row[m_nameActualColumn];
	    name = us.c_str();
	    (void)strcpy(remoteName, name);

	    us = row[m_nameShortColumn];
	    name = us.c_str();
	    (void)sprintf(localName, "%s/%s%s", appFrame::dailyDir,
		m_block->currentPrefix(), name);

		/* make and log the transfer */

	    if (m_ftp->FTPGetFile(remoteName, localName) == -1) return;
	    sprintf(logmsg, "Fetched %s over FTP; saved as %s.",
		remoteName, localName);
	    m_displayTab->log(logmsg);
	}
    }

        /* update the time left since the export may have gone to the 
	   same filesystem as the flightline and end-to-end data */

    m_displayTab->updateResourcesDisplay();
}


void ECSDataTab::onDeleteButton(void)
{
    int numDatasets, size;
    char warningMsg[MAXPATHLEN+MAX_ERROR_LEN];
    const char *name;
    char remoteName[MAXPATHLEN];
    char logmsg[MAXPATHLEN+100];

	/* log */

    sprintf(logmsg, "User pressed ECS Delete button.");
    m_displayTab->log(logmsg);

	/* make sure we have an active FTP connection */

    if (!m_ftp) {
	m_displayTab->error("FTP connection has been lost.  Refresh, please!");
        return;
    }

	/* init */

    typedef Gtk::TreeModel::Children children_t;
    children_t c = m_model->children();

    numDatasets = 0;
    size = 0;

	/* count number of selected items */

    for (children_t::iterator iter = c.begin(); iter != c.end(); ++iter)
        if (m_selection->is_selected(iter)) {
            Gtk::TreeModel::Row row = *iter;
            size += row[m_sizeInBytesColumn];
            numDatasets++;
        }
    if (numDatasets == 0) {
        m_displayTab->error(
	    "Internal error: Can't determine selected ECS data files.");
        return;
    }

        /* give user chance to bail */

    sprintf(warningMsg,
        "You're about to delete %d ECS data file(s), "
	    "representing %.1f MB.\n\n",
	numDatasets, size / (double)BYTES_PER_MB);
    Gtk::MessageDialog d(warningMsg, false /* no markup */,
	Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK_CANCEL);
    if ((Gtk::ResponseType)d.run() != Gtk::RESPONSE_OK) {
	m_displayTab->log("User cancelled delete.");
	return;
    }
    else m_displayTab->log("User confirmed delete.");

        /* go ahead and delete them */

    for (children_t::iterator iter = c.begin(); iter != c.end();) {

	    /* if the row is selected... */

	if (m_selection->is_selected(iter)) {

		/* get file name */

	    Gtk::TreeModel::Row row = *iter;

	    Glib::ustring us = row[m_nameActualColumn];
	    name = us.c_str();
	    (void)strcpy(remoteName, name);

		/* delete file and log deletion */

            if (m_ftp->FTPDeleteFile(remoteName) == -1)
		return;
	    sprintf(logmsg, "Deleted %s from remote server using FTP.",
		remoteName);
	    m_displayTab->log(logmsg);

		/* erase it from the listing */

	    iter = m_model->erase(iter);
        }

	    /* otherwise move on to the next */

	else ++iter;
    }

        /* disable the buttons since nothing is now selected */

    m_deleteButton.set_sensitive(false);

        /* update time left on display dialog */

    m_displayTab->updateResourcesDisplay();
}
