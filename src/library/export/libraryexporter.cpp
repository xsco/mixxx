#include "library/export/libraryexporter.h"

#include <QtGlobal>
#include <QFileDialog>
#include <QFileInfo>
#include <QDesktopServices>
#include <QMessageBox>

#include <djinterop/enginelibrary.hpp>

#include "util/assert.h"

namespace el = djinterop::enginelibrary;

void LibraryExporter::exportLibrary() {

    // For now, we can only export in Engine Prime / Engine Library format
    // In this initial implementation, ask for an output directory and then
    // export the whole library.
    QString lastExportDirectory = m_pConfig->getValue(
            ConfigKey("[Library]", "LastLibraryExportDirectory"),
            QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation));

    QString destDirStr = QFileDialog::getExistingDirectory(
            NULL, tr("Export Library To"), lastExportDirectory);
    if (destDirStr.isEmpty()) {
        return;
    }
    m_pConfig->set(ConfigKey("[Library]", "LastLibraryExportDirectory"),
                   ConfigValue(destDirStr));

    // Check to see if destDir ends in "Engine Library" or not, and then ensure
    // we have the right folder to work with.
    QDir destDir{destDirStr};
    if (destDir.dirName() != "Engine Library")
    {
    	// Ensure we are working with the Engine Library directory.
    	destDirStr = destDir.filePath("Engine Library");
    }

    // TODO - check for presence of any existing database.  If there is already
    // one, prompt for whether to merge into it or not.
    el::database db{destDirStr.toStdString()};
    if (db.exists())
    {
    	int ret = QMessageBox::question(
    			NULL,
				tr("Merge Into Existing Library?"),
				tr("There is already an existing library in directory ") +
				destDirStr +
				tr("\nIf you proceed, the Mixxx library will be merged into "
				   "this existing library.  Do you want to merge into the "
				   "the existing library?"),
				QMessageBox::Yes | QMessageBox::Cancel,
				QMessageBox::Cancel);
    	if (ret != QMessageBox::Yes)
    	{
    		return;
    	}
    }
    else
    {
    	// Create new database.
    	qInfo() << "Creating new empty database in " << destDirStr;
    	db = el::create_database(destDirStr.toStdString(), el::version_1_7_1);
    }

    // TODO - copy music files into a new "Mixxx Music" sub-directory
    // TODO - write DB metadata, incl. track, beat grid, waveforms, crates, etc.
    // TODO - set up a QProgressDialog in modal mode, loop over files, etc.

    // All done.
    qInfo() << "Library exported to path " << destDirStr;
}
