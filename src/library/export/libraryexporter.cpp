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

const el::musical_key keyMap[] =
{
  el::musical_key::c_major,       // INVALID = 0,
  el::musical_key::c_major,       // C_MAJOR = 1,
  el::musical_key::d_flat_major,  // D_FLAT_MAJOR = 2,
  el::musical_key::d_major,       // D_MAJOR = 3,
  el::musical_key::e_flat_major,  // E_FLAT_MAJOR = 4,
  el::musical_key::e_major,       // E_MAJOR = 5,
  el::musical_key::f_major,       // F_MAJOR = 6,
  el::musical_key::f_sharp_major, // F_SHARP_MAJOR = 7,
  el::musical_key::g_major,       // G_MAJOR = 8,
  el::musical_key::a_flat_major,  // A_FLAT_MAJOR = 9,
  el::musical_key::a_major,       // A_MAJOR = 10,
  el::musical_key::b_flat_major,  // B_FLAT_MAJOR = 11,
  el::musical_key::b_major,       // B_MAJOR = 12,
  el::musical_key::c_minor,       // C_MINOR = 13,
  el::musical_key::d_flat_minor,  // C_SHARP_MINOR = 14,
  el::musical_key::d_minor,       // D_MINOR = 15,
  el::musical_key::e_flat_minor,  // E_FLAT_MINOR = 16,
  el::musical_key::e_minor,       // E_MINOR = 17,
  el::musical_key::f_minor,       // F_MINOR = 18,
  el::musical_key::f_sharp_minor, // F_SHARP_MINOR = 19,
  el::musical_key::g_minor,       // G_MINOR = 20,
  el::musical_key::a_flat_minor,  // G_SHARP_MINOR = 21,
  el::musical_key::a_minor,       // A_MINOR = 22,
  el::musical_key::b_flat_minor,  // B_FLAT_MINOR = 23,
  el::musical_key::b_minor,       // B_MINOR = 24
};

static el::track createOrLoadTrack(
		const el::database &db, const QString &relPath)
{
	auto ids = el::find_track_ids_by_path(db, relPath.toStdString());
	if (ids.empty())
		return el::track{};
	else
		return el::track{db, ids[0]};
}

static el::musical_key convertKey(mixxx::track::io::key::ChromaticKey key)
{
	return keyMap[key];
}

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

    // Keep a mapping from Mixxx track id to EL track id
    QHash<int, int> trackIdToElId;

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

    	// Create or update a track record.
    	QString trackRelPath = "../";
    	trackRelPath = trackRelPath + mixxxExportDirName + "/" + dstFilename;
    	auto t = createOrLoadTrack(db, trackRelPath);
    	t.set_track_number(trackPtr->getTrackNumber().toInt());
    	t.set_duration(std::chrono::seconds{trackPtr->getDurationInt()});
    	t.set_bpm((int)trackPtr->getBpm());
    	t.set_year(trackPtr->getYear().toInt());
    	t.set_title(trackPtr->getTitle().toStdString());
    	t.set_artist(trackPtr->getArtist().toStdString());
    	t.set_album(trackPtr->getAlbum().toStdString());
    	t.set_genre(trackPtr->getGenre().toStdString());
    	t.set_comment(trackPtr->getComment().toStdString());
    	t.set_composer(trackPtr->getComposer().toStdString());
    	auto key = trackPtr->getKey();
    	auto elKey = convertKey(key);
    	if (key != mixxx::track::io::key::INVALID)
    		t.set_key(elKey);
    	t.set_path(trackRelPath.toStdString());
    	t.set_filename(srcFileInfo.fileName().toStdString());
    	t.set_file_extension(srcFileInfo.suffix().toStdString());
    	std::chrono::system_clock::time_point lastModifiedAt{
    		std::chrono::milliseconds{
    			trackPtr->getFileModifiedTime().toMSecsSinceEpoch()}};
    	t.set_last_modified_at(lastModifiedAt);
    	t.set_bitrate(trackPtr->getBitrate());
    	t.set_ever_played(trackPtr->getTimesPlayed() > 0);
    	t.set_imported(false); // Not imported from another EL database
    	t.set_no_album_art(); // Album art is not currently supported
    	t.save(db);

    	// Add to the mapping of track ids
    	trackIdToElId[trackId.value()] = t.id();

    	// Write the performance data record
    	auto perfDataExists = el::performance_data::exists(db, t.id());
    	el::performance_data p = perfDataExists
    			? el::performance_data{db, t.id()}
    			: el::performance_data{t.id()};

    	auto beats = trackPtr->getBeats();
    	auto cues = trackPtr->getCuePoints();
    	p.set_sample_rate(trackPtr->getSampleRate());

    	// TODO - write DB metadata, incl. track, beat grid, waveforms, etc.
    	p.save(db);

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
