#include "library/export/libraryexportworker.h"

#include <QtGlobal>
#include <QFileDialog>
#include <QFileInfo>
#include <QDesktopServices>
#include <QMessageBox>

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

LibraryExportWorker::~LibraryExportWorker() {
    qInfo() << "Library export worker now being cleaned up...";
}

void LibraryExportWorker::startExport() {
    // Only permit one export to be active at any given time.
    if (m_exportActive) {
    	QMessageBox::information(
    			dynamic_cast<QWidget *>(parent()),
    			tr("Export Already Active"),
    			tr("There is already a active export taking place.  Please "
                   "wait for this to finish, or cancel the existing export."),
    			QMessageBox::Ok,
    			QMessageBox::Ok);
        return;
    }

    m_exportActive = true;

    qInfo() << "EL directory: " << m_pModel->engineLibraryDir;
    qInfo() << "Music export directory: " << m_pModel->musicFilesDir;
    if (m_pModel->exportEntireMusicLibrary) {
        qInfo() << "Exporting ENTIRE music library...";
        m_trackIds = GetAllTrackIds();
        m_crateIds = QList<CrateId>::fromSet(
                m_pTrackCollection->crates().collectCrateIdsOfTracks(m_trackIds));
    }
    else {
        qInfo() << "Exporting selected crates...";
        m_trackIds = GetTracksIdsInCrates(m_pModel->selectedCrates);
        m_crateIds = m_pModel->selectedCrates;
    }

    // Check for presence of any existing database.  If there is already one,
    // prompt for whether to merge into it or not.
    el::database db{m_pModel->engineLibraryDir.toStdString()};
    if (db.exists())
    {
    	int ret = QMessageBox::question(
    			dynamic_cast<QWidget *>(parent()),
				tr("Merge Into Existing Library?"),
				tr("There is already an existing library in directory ") +
				m_pModel->engineLibraryDir +
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

    // Max progress count = no. crates + no. tracks + 2 (start & finish actions)
    qInfo() << "Num tracks =" << m_trackIds.count();
    qInfo() << "Num crates =" << m_crateIds.count();
    int maxSteps = m_trackIds.count() + m_crateIds.count() + 2;
    m_pProgress.reset(new QProgressDialog{
        "Getting ready to export...",
            "Cancel",
            0,
            maxSteps,
            dynamic_cast<QWidget *>(parent())});
    connect(m_pProgress.data(), SIGNAL(canceled()), this, SLOT(cancel()));
    m_pProgress->setWindowTitle(tr("Export Library"));
    m_pProgress->setWindowModality(Qt::WindowModal);

    if (!db.exists())
    {
    	// Create new database.
    	qInfo() << "Creating new empty database in" << m_pModel->engineLibraryDir;
        m_pProgress->setLabelText(tr("Creating database..."));
    	db = el::create_database(
                m_pModel->engineLibraryDir.toStdString(), el::version_1_7_1);
    }

    // The LibraryExporter should connect signals/slots:
    // * Track analysis complete -> LEW.exportTrack()
    // * Crate export request -> LEW.exportCurrentCrate()
    // * Export finish request -> LEW.finishExport()
    //
    // Approx operation:
    // * LEW.startExport() begins export
    // * LEW.startExport() emits first track analysis request
    // * LEW.exportTrack() exports one track at a time
    // * LEW.exportTrack() emits further track analysis requests
    // * LEW.exportTrack() emits crate export request after all tracks done
    // * LEW.exportCurrentCrate() exports one crate at a time
    // * LEW.exportCurrentCrate() emits further crate export requests
    // * LEW.exportCurrentCrate() emits export finish request after all crates done
    // * LEW.finishExport() finishes up and emits exportFinished() signal!
    //
    // Suggestions for track analysis:
    // * Slot to analyse tracks: AnalysisFeature.analyzeTracks(const QList<int> &)
    // * Signal to listen for:   (none yet)
    //           perhaps create: AnalysisFeature.trackAnalyzed(TrackPointer) ??
    // (AnalysisFeature available on Library class)
    //
    // * Slot to call to start:  AnalyzerQueue.slotAnalyzeTrack(TrackPointer)
    // * Signal to listen for:   AnalyzerQueue.trackDone(TrackPointer)
    //
    // How does exportTrack() / exportCurrentCrate() know which one to do next?
    // The trackDone() signal includes a pointer to the track that has just
    // been analysed.  Safe to assume that analysis is processed sequentially?
    // 
    // The hash from Mixxx track id to EL id will need to be locked using a
    // QMutexLocker before any new track id is added, as we assume that track
    // callbacks could happen in parallel.  Same for updating progress dialog?


    /*
    qInfo() << "Scheduling" << trackIds.count() << "tracks for analysis...";
    if (trackIds.isEmpty()) {
        emit(m_pAnalysisFeature->analyzeTracks(trackIds));
    }
    */

    // TODO - temporary
    emit exportCurrentTrack(nullptr);


    /*
    // Create the music export directory, if it doesn't already exist.
    parentDir.mkpath(mixxxExportDirName);
    QDir exDir{exDirStr};

    auto numTracks = trackIds.size();

    // Keep a mapping from Mixxx track id to EL track id
    QHash<int, int> trackIdToElId;

    // Run a modal progress dialog as we export each track.
    QProgressDialog progressFiles{
    	"Copying tracks...",
		"Cancel",
		0,
		numTracks,
    	m_parent};
    progressFiles.setWindowModality(Qt::WindowModal);
    int numTracksExported = 0;
    for (int i = 0; i < numTracks; ++i)
    {
    	progressFiles.setValue(i);
    	auto trackId = trackIds[i];
    	auto trackPtr = m_pTrackCollection->getTrackDAO().getTrack(trackId);

        // FIXME - for testing, only bother to export tracks that we have beats and waveform analysis for.
        // TODO/FIXME - waveform summary is not loaded by default!!
        if (trackPtr->getBeats() == nullptr || trackPtr->getWaveformSummary() == nullptr)
            continue;

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
        // TODO - put some stopwatches in to test copy/EL speed on slow USB sticks.
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
        el::track t = createOrLoadTrack(db, trackRelPath);
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
        el::musical_key elKey = convertKey(key);
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
    	bool perfDataExists = el::performance_data::exists(db, t.id());
    	el::performance_data p = perfDataExists
    			? el::performance_data{db, t.id()}
    			: el::performance_data{t.id()};

        // Frames used interchangeably with "samples" here.
        double totalFrames = trackPtr->getDuration() * trackPtr->getSampleRate();
    	p.set_sample_rate(trackPtr->getSampleRate());
        p.set_total_samples(totalFrames);
        p.set_key(elKey);
        //p.set_average_loudness(...) TODO - set average loudness

        // Fill in beat grid.  For now, assume a constant average BPM across
        // the whole track.  Note that points in the track are specified as
        // "play positions", which are twice the sample offset.
    	BeatsPointer beats = trackPtr->getBeats();
        double firstBeatPlayPos = beats->findNextBeat(0);
        double lastBeatPlayPos = beats->findPrevBeat(totalFrames * 2);
        double numBeats = beats->numBeatsInRange(firstBeatPlayPos, lastBeatPlayPos);
        el::track_beat_grid elGrid{0, firstBeatPlayPos / 2, numBeats, lastBeatPlayPos / 2};
        el::normalise_beat_grid(elGrid, totalFrames);
        p.set_default_beat_grid(elGrid);
        p.set_adjusted_beat_grid(elGrid);

        // Write cues
        std::vector<el::track_hot_cue_point> hotCues; // TODO - empty for now
        p.set_hot_cues(std::begin(hotCues), std::end(hotCues));
        double cuePlayPos = trackPtr->getCuePoint();
        p.set_default_main_cue_sample_offset(cuePlayPos / 2);
        p.set_adjusted_main_cue_sample_offset(cuePlayPos / 2);
    	auto cues = trackPtr->getCuePoints();

        // TODO - fill in loops
       
        // Write overview/summary waveform
        auto overviewWaveform = trackPtr->getWaveformSummary();
        if (overviewWaveform != nullptr)
        {
            uint_least64_t overviewAdjustedTotalSamples, overviewNumEntries;
            double overviewSamplesPerEntry;
            el::calculate_overview_waveform_details(
                    totalFrames, trackPtr->getSampleRate(),
                    overviewAdjustedTotalSamples, overviewNumEntries,
                    overviewSamplesPerEntry);
            std::vector<el::overview_waveform_entry> elOverviewWaveform;
            for (uint_least64_t i = 0; i < overviewNumEntries; ++i)
            {
                auto j = overviewWaveform->getDataSize() * i / overviewNumEntries;
                elOverviewWaveform.push_back(el::overview_waveform_entry{
                    overviewWaveform->getLow(j),
                    overviewWaveform->getMid(j),
                    overviewWaveform->getHigh(j)});
            }
            p.set_overview_waveform_entries(
                    overviewNumEntries,
                    overviewSamplesPerEntry,
                    std::begin(elOverviewWaveform),
                    std::end(elOverviewWaveform));
        }

        // Write high-resolution full waveform
        auto highResWaveform = trackPtr->getWaveform();
        if (highResWaveform != nullptr)
        {
            uint_least64_t highResAdjustedTotalSamples, highResNumEntries;
            double highResSamplesPerEntry;
            el::calculate_high_res_waveform_details(
                    totalFrames, trackPtr->getSampleRate(),
                    highResAdjustedTotalSamples, highResNumEntries,
                    highResSamplesPerEntry);
            std::vector<el::high_res_waveform_entry> elHighResWaveform;
            for (uint_least64_t i = 0; i < highResNumEntries; ++i)
            {
                auto j = highResWaveform->getDataSize() * i / highResNumEntries;
                elHighResWaveform.push_back(el::high_res_waveform_entry{
                    highResWaveform->getLow(j),
                    highResWaveform->getMid(j),
                    highResWaveform->getHigh(j),
                    127,
                    127,
                    127});
            }
            p.set_high_res_waveform_entries(
                    highResNumEntries,
                    highResSamplesPerEntry,
                    std::begin(elHighResWaveform),
                    std::end(elHighResWaveform));
        }

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

        ++numTracksExported;
    }
    progressFiles.setValue(numTracks);

    // TODO - export crates

    // All done.
    qInfo() << "Exported" << numTracksExported << "out of" << numTracks << "tracks";
    QMessageBox::information(
    		m_parent,
			tr("Export Completed"),
			tr("The Mixxx library has been successfully exported."),
			QMessageBox::Ok,
			QMessageBox::Ok);
    */
}

void LibraryExportWorker::exportCurrentTrack(TrackPointer track) {
    if (!m_exportActive) {
        return;
    }

    // Check that the track pointer matches the current track.  If it doesn't,
    // perhaps another track that we're not interested in has been scheduled
    // for analysis by another operation.
    auto trackId = m_trackIds[m_currTrackIndex];
    // TODO - reinstate below check once track pointers are valid
    /*
    if (track->getId() != trackId) {
        qInfo() << "Track" << track->getId ()
            << "completed analysis, but this wasn't the track the export "
            "process was expecting (perhaps it was scheduled by another "
            "operation) - will ignore";
        return;
    }
    */

    m_pProgress->setLabelText(tr("Exporting track..."));
    qInfo() << "Would export track" << (m_currTrackIndex + 1) << "/" << m_trackIds.count() << "...";

    // TODO - export track
    qInfo() << "Track id =" << trackId;

    // Update progress.
    m_currTrackIndex++;
    m_pProgress->setValue(m_pProgress->value() + 1);
    if (m_currTrackIndex == m_trackIds.count()) {
        // Move onto crates.
        emit exportCurrentCrate();
    }

    // TODO - schedule next track for analysis.
    emit exportCurrentTrack(nullptr);
}

void LibraryExportWorker::exportCurrentCrate() {
    if (!m_exportActive) {
        return;
    }

    m_pProgress->setLabelText(tr("Exporting crate..."));
    qInfo() << "Would export crate" << (m_currCrateIndex + 1) << "/" << m_crateIds.count() << "...";

    // TODO - export crate
    auto crateId = m_crateIds[m_currCrateIndex];
    qInfo() << "Crate id =" << crateId;

    // Update progress.
    m_currCrateIndex++;
    m_pProgress->setValue(m_pProgress->value() + 1);
    if (m_currCrateIndex == m_crateIds.count()) {
        // All finished
        emit finishExport();
    }

    // Schedule next crate.
    emit exportCurrentCrate();
}

void LibraryExportWorker::finishExport() {
    m_pProgress->setValue(m_pProgress->maximum());
    m_exportActive = false;
    QMessageBox::information(
			dynamic_cast<QWidget *>(parent()),
			tr("Export Completed"),
			tr("The Mixxx library has been successfully exported."),
			QMessageBox::Ok,
			QMessageBox::Ok);
    emit exportFinished();
}

void LibraryExportWorker::cancel() {
    m_exportActive = false;
	QMessageBox::information(
			dynamic_cast<QWidget *>(parent()),
			tr("Export Aborted"),
			tr("Library export was aborted.  The Mixxx library has "
			   "only been partially exported."),
			QMessageBox::Ok,
			QMessageBox::Ok);
}

// Get all track ids across the entire music library.
QList<TrackId> LibraryExportWorker::GetAllTrackIds() {
    // Obtain a list of all track ids across all directories.
    QList<TrackId> trackIds;
    auto dirs = m_pTrackCollection->getDirectoryDAO().getDirs();
    for (auto iter = dirs.cbegin(); iter != dirs.cend(); ++iter)
    	trackIds.append(m_pTrackCollection->getTrackDAO().getTrackIds(*iter));
    return trackIds;
}

// Get track ids in a provided list of crates.
QList<TrackId> LibraryExportWorker::GetTracksIdsInCrates(const QList<CrateId> &crateIds) {
    QList<TrackId> trackIds;
    for (auto iter = crateIds.cbegin(); iter != crateIds.end(); ++iter) {
        auto result = m_pTrackCollection->crates().selectCrateTracksSorted(*iter);
        while (result.next()) {
            trackIds.append(result.trackId());
        }
    }
    return trackIds;
}

