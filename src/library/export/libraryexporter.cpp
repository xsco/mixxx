#include "library/export/libraryexporter.h"

#include <QtGlobal>
#include <QFileDialog>
#include <QFileInfo>
#include <QDesktopServices>
#include <QMessageBox>
#include <QProgressDialog>

#include <djinterop/enginelibrary.hpp>

#include "library/trackcollection.h"
#include "util/assert.h"

namespace el = djinterop::enginelibrary;

const auto engineLibraryDirName = "Engine Library";
const auto mixxxExportDirName = "MixxxExport";

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
    // we have the right folder to work with.  We will write the Engine Library
    // database to a directory called "Engine Library", and copy the music
    // files to a directory called "MixxxExport".  Both directories must share
    // the same parent directory.
    QDir parentDir{destDirStr};
    QString elDirStr;
    QString exDirStr;
    if (parentDir.dirName() == engineLibraryDirName)
    {
    	// The user chose the Engine Library dir.
    	parentDir.cdUp();
    	elDirStr = destDirStr;
    	exDirStr = parentDir.filePath(mixxxExportDirName);
    }
    else if (parentDir.dirName() == mixxxExportDirName)
    {
    	// The user chose the music export dir.
    	parentDir.cdUp();
    	elDirStr = parentDir.filePath(engineLibraryDirName);
    	exDirStr = destDirStr;
    }
    else
    {
    	// The user chose some other dir.
    	elDirStr = parentDir.filePath(engineLibraryDirName);
    	exDirStr = parentDir.filePath(mixxxExportDirName);
    }

    qInfo() << "EL directory: " << elDirStr;
    qInfo() << "Music export directory: " << exDirStr;

    // Check for presence of any existing database.  If there is already one,
    // prompt for whether to merge into it or not.
    el::database db{elDirStr.toStdString()};
    if (db.exists())
    {
    	int ret = QMessageBox::question(
    			m_parent,
				tr("Merge Into Existing Library?"),
				tr("There is already an existing library in directory ") +
				elDirStr +
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
    	qInfo() << "Creating new empty database in " << elDirStr;
    	db = el::create_database(elDirStr.toStdString(), el::version_1_7_1);
    }

    // Create the music export directory, if it doesn't already exist.
    parentDir.mkpath(mixxxExportDirName);
    QDir exDir{exDirStr};

    // Obtain a list of all track pointers across all directories.
    QList<TrackId> trackIds;
    auto dirs = m_pTrackCollection->getDirectoryDAO().getDirs();
    for (auto iter = dirs.cbegin(); iter != dirs.cend(); ++iter)
    	trackIds.append(m_pTrackCollection->getTrackDAO().getTrackIds(*iter));
    auto numTracks = trackIds.size();

    // Run a modal progress dialog as we export each track.
    qInfo() << "Copying tracks to " << exDirStr;
    QProgressDialog progressFiles{
    	"Copying tracks...",
		"Cancel",
		0,
		numTracks,
    	m_parent};
    progressFiles.setWindowModality(Qt::WindowModal);
    for (auto i = 0; i < numTracks; ++i)
    {
    	progressFiles.setValue(i);
    	auto trackId = trackIds[i];
    	auto trackPtr = m_pTrackCollection->getTrackDAO().getTrack(trackId);

    	// Check if the file format is supported by Engine Library.
    	// Supported file formats are:
        // * MP3 32 kbit/s - 320 kbit/s + VBR
        // * M4A / AAC
        // * MP4
        // * OGG Vorbis
        // * WAV 16-32 bit, 44.1k - 192k
        // * AIFF 16-32 bit, 44.1k - 192k
        // * FLAC
        // * ALAC
    	// TODO - check file format using trackPtr->getType()

    	// Copy music files into the MixxxExport dir, if the source file has
    	// been modified (or the destination doesn't exist).  To ensure no
    	// chance of filename clashes, and to keep things simple, we will name
    	// the destination files after the DB track identifier.
    	auto srcFileInfo = trackPtr->getFileInfo();
    	QString dstFilename =
			QString::number(trackId.value()) +
    		"." +
			srcFileInfo.suffix();
    	auto srcFilePath = trackPtr->getFileInfo().filePath();
    	auto dstFilePath = exDir.filePath(dstFilename);
    	auto copyFile = true;
    	if (QFile::exists(dstFilePath))
    	{
    		// The destination file already exists.  Only copy if the source
    		// is newer than the file currently there.
    		QFileInfo srcFileInfo{srcFilePath};
    		QFileInfo dstFileInfo{dstFilePath};
    		copyFile = srcFileInfo.lastModified() > dstFileInfo.lastModified();
    	}
    	if (copyFile)
    	{
    		QFile::copy(srcFilePath, dstFilePath);
    	}

    	// TODO - write DB metadata, incl. track, beat grid, waveforms, etc.

    	// Abort the export operation if cancelled.
    	if (progressFiles.wasCanceled())
    	{
    		QMessageBox::information(
    				m_parent,
					tr("Export Aborted"),
					tr("Library export was aborted.  The Mixxx library has "
					   "only been partially exported."),
					QMessageBox::Ok,
					QMessageBox::Ok);
    		return;
    	}
    }
    progressFiles.setValue(numTracks);

    // TODO - export crates

    // All done.
    QMessageBox::information(
    		m_parent,
			tr("Export Completed"),
			tr("The Mixxx library has been successfully exported."),
			QMessageBox::Ok,
			QMessageBox::Ok);
}
