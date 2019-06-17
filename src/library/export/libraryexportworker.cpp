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

static void copyFilesInDir(const QString &srcDir, const QString &dstDir) {
    QDir src{srcDir};
    if (!src.exists()) {
        return;
    }

    QDir dst{dstDir};
    if (!dst.exists()) {
        QDir{}.mkpath(dstDir);
    }

    qInfo() << "Copying from" << srcDir << "to" << dstDir;
    for (auto &file : src.entryList(QDir::Files)) {
        QFile::copy(
                srcDir + QDir::separator() + file,
                dstDir + QDir::separator() + file);
    }
}

static el::track createOrLoadTrack(
        const el::database &db, const QString &relPath) {
    auto ids = el::find_track_ids_by_path(db, relPath.toStdString());
    if (ids.empty())
        return el::track{};
    else
        return el::track{db, ids[0]};
}

static el::crate createOrLoadCrate(
        const el::database &db, const QString &name) {
    int id;
    if (el::find_crate_by_name(db, name.toStdString(), id))
        return el::crate{db, id};
    else
        return el::crate{};
}

static el::musical_key convertKey(mixxx::track::io::key::ChromaticKey key) {
    return keyMap[key];
}

LibraryExportWorker::LibraryExportWorker(
        QWidget *parent,
        std::shared_ptr<LibraryExportModel> pModel,
        TrackCollection *pTrackCollection,
        AnalysisFeature *pAnalysisFeature) :
    QObject{parent},
    m_pModel{pModel},
    m_pTrackCollection{pTrackCollection},
    m_pAnalysisFeature{pAnalysisFeature},
    m_exportActive{false},
    m_numTracksDone{0},
    m_currCrateIndex{0}
{
    connect(this, &LibraryExportWorker::readyForSetupElDatabase,
            this, &LibraryExportWorker::setupElDatabase);
    connect(this, &LibraryExportWorker::readyForExportCurrentCrate,
            this, &LibraryExportWorker::exportCurrentCrate);
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

    if (m_trackIds.isEmpty()) {
        QMessageBox::information(
                dynamic_cast<QWidget *>(parent()),
                tr("No tracks to export"),
                tr("There are no tracks to export in the selection made.  "
                   "Nothing will be exported."),
                QMessageBox::Ok,
                QMessageBox::Ok);
        return;
    }

    // Check for presence of any existing EL database.  If there is already one,
    // prompt for whether to merge into it or not.
    m_pElDb.reset(new el::database{m_pModel->engineLibraryDir.toStdString()});
    if (m_pElDb->exists())
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
    int maxSteps = m_trackIds.count() + m_crateIds.count() + 2;
    m_pProgress.reset(new QProgressDialog{
        "Getting ready to export...",
            "Cancel",
            0,
            maxSteps,
            dynamic_cast<QWidget *>(parent())});
    connect(m_pProgress.get(), SIGNAL(canceled()), this, SLOT(cancel()));
    m_pProgress->setWindowTitle(tr("Export Library"));
    m_pProgress->setWindowModality(Qt::WindowModal);
    m_pProgress->setMinimumDuration(0);

    // Move onto setting up the EL database.
    emit readyForSetupElDatabase(QPrivateSignal{});
}

void LibraryExportWorker::setupElDatabase() {
    // Set up the EL database.
    m_pProgress->setLabelText(tr("Setting up Engine Library database..."));
    m_pProgress->setValue(m_pProgress->value() + 1);

    if (!m_pElDb->exists())
    {
        // Create new database.
        // Note that we create in temporary directory and then move over, as
        // SQLite commands appear to be slow when run directly on USB sticks.
        qInfo() << "Creating new empty database in" << m_pModel->engineLibraryDir;
        m_pProgress->setLabelText(tr("Creating database..."));
        m_pElDb.reset(new el::database{std::move(el::create_database(
                m_tempEngineLibraryDir.path().toStdString(), el::version_1_7_1))});
    }
    else {
        // Copy the DB to our temporary directory whilst exporting.
        copyFilesInDir(m_pModel->engineLibraryDir, m_tempEngineLibraryDir.path());
    }

    // Create the music export directory, if it doesn't already exist.
    QDir().mkpath(m_pModel->musicFilesDir);

    // Currently we don't schedule tracks for analysis because the AnalysisFeature code changed.

    for (auto trackId : m_trackIds) {
        exportTrack(m_pTrackCollection->getTrackDAO().getTrack(trackId));
    }
}

void LibraryExportWorker::exportTrack(TrackPointer pTrack) {
    if (!m_exportActive) {
        return;
    }
    else if (pTrack == nullptr) {
        qWarning() << "Received null track pointer for export!";
        fail();
        return;
    }

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
    // TODO - check file format using pTrack->getType()

    qInfo() << "Exporting" << pTrack->getTitle();
    m_pProgress->setLabelText(tr("Exporting track...") + " " + pTrack->getTitle());

    // Copy the underlying file.
    auto dstFilename = copyFile(pTrack);

    // Write metadata for the file.
    writeMetadata(pTrack, dstFilename);

    // Update progress.
    m_numTracksDone++;
    m_pProgress->setValue(m_pProgress->value() + 1);
    if (m_numTracksDone == m_trackIds.count()) {
        // Move onto crates.
        emit readyForExportCurrentCrate(QPrivateSignal{});
    }
}

QString LibraryExportWorker::copyFile(TrackPointer pTrack) {
    // Copy music files into the Mixxx export dir, if the source file has
    // been modified (or the destination doesn't exist).  To ensure no
    // chance of filename clashes, and to keep things simple, we will name
    // the destination files after the DB track identifier.
    auto srcFileInfo = pTrack->getFileInfo();
    QString dstFilename =
        QString::number(pTrack->getId().value()) +
        "." +
        srcFileInfo.suffix();
    QDir dstDir{m_pModel->musicFilesDir};
    auto dstFilePath = dstDir.filePath(dstFilename);
    auto shouldCopyFile = true;
    if (QFile::exists(dstFilePath))
    {
        // The destination file already exists.  Only copy if the source
        // is newer than the file currently there.
        QFileInfo dstFileInfo{dstFilePath};
        shouldCopyFile = srcFileInfo.lastModified() > dstFileInfo.lastModified();
    }

    if (shouldCopyFile)
    {
        QString label = tr("Copying") + " " + pTrack->getTitle();
        m_pProgress->setLabelText(label);
        auto srcFilePath = srcFileInfo.filePath();
        qInfo() << "Copying" << srcFilePath << "to" << dstFilePath;
        QFile::copy(srcFilePath, dstFilePath);
    }

    return dstFilename;
}

void LibraryExportWorker::writeMetadata(
        TrackPointer pTrack, const QString &dstFilename) {
    auto trackId = pTrack->getId();
    auto &db = *m_pElDb;

    // Create or update a track record.
    QString trackRelPath = "../";
    trackRelPath = trackRelPath + MixxxExportDirName + "/" + dstFilename;
    el::track t = createOrLoadTrack(db, trackRelPath);
    t.set_track_number(pTrack->getTrackNumber().toInt());
    t.set_duration(std::chrono::seconds{pTrack->getDurationInt()});
    t.set_bpm((int)pTrack->getBpm());
    t.set_year(pTrack->getYear().toInt());
    t.set_title(pTrack->getTitle().toStdString());
    t.set_artist(pTrack->getArtist().toStdString());
    t.set_album(pTrack->getAlbum().toStdString());
    t.set_genre(pTrack->getGenre().toStdString());
    t.set_comment(pTrack->getComment().toStdString());
    t.set_composer(pTrack->getComposer().toStdString());
    auto key = pTrack->getKey();
    el::musical_key elKey = convertKey(key);
    if (key != mixxx::track::io::key::INVALID)
        t.set_key(elKey);
    t.set_path(trackRelPath.toStdString());
    t.set_filename(dstFilename.toStdString());
    t.set_file_extension(pTrack->getFileInfo().suffix().toStdString());
    std::chrono::system_clock::time_point lastModifiedAt{
        std::chrono::milliseconds{
            pTrack->getFileModifiedTime().toMSecsSinceEpoch()}};
    t.set_last_modified_at(lastModifiedAt);
    t.set_bitrate(pTrack->getBitrate());
    t.set_ever_played(pTrack->getTimesPlayed() > 0);
    t.set_imported(false); // Not imported from another EL database
    t.set_no_album_art(); // Album art is not currently supported
    t.save(db);

    // Add to the mapping of track ids
    m_trackIdToElId[trackId.value()] = t.id();

    // Write the performance data record
    bool perfDataExists = el::performance_data::exists(db, t.id());
    el::performance_data p = perfDataExists
            ? el::performance_data{db, t.id()}
            : el::performance_data{t.id()};

    // Frames used interchangeably with "samples" here.
    double totalFrames = pTrack->getDuration() * pTrack->getSampleRate();
    p.set_sample_rate(pTrack->getSampleRate());
    p.set_total_samples(totalFrames);
    p.set_key(elKey);
    //p.set_average_loudness(...); TODO - set average loudness

    // Fill in beat grid.  For now, assume a constant average BPM across
    // the whole track.  Note that points in the track are specified as
    // "play positions", which are twice the sample offset.
    BeatsPointer beats = pTrack->getBeats();
    if (beats != nullptr) {
        double firstBeatPlayPos = beats->findNextBeat(0);
        double lastBeatPlayPos = beats->findPrevBeat(totalFrames * 2);
        int numBeats = beats->numBeatsInRange(firstBeatPlayPos, lastBeatPlayPos);
        el::track_beat_grid elGrid{
            0, firstBeatPlayPos / 2, numBeats, lastBeatPlayPos / 2};
        el::normalise_beat_grid(elGrid, totalFrames);
        p.set_default_beat_grid(elGrid);
        p.set_adjusted_beat_grid(elGrid);
    }

    // Write hot cues.
    std::vector<el::track_hot_cue_point> elHotCues;
    // TODO - fill in cues
    p.set_hot_cues(std::begin(elHotCues), std::end(elHotCues));

    // Set main cue-point.
    double cuePlayPos = pTrack->getCuePoint().getPosition();
    p.set_default_main_cue_sample_offset(cuePlayPos / 2);
    p.set_adjusted_main_cue_sample_offset(cuePlayPos / 2);
    auto cues = pTrack->getCuePoints();

    std::vector<el::track_loop> elLoops;
    // TODO - fill in loops
    p.set_loops(std::begin(elLoops), std::end(elLoops));
   
    // Write overview/summary waveform
    auto overviewWaveform = pTrack->getWaveformSummary();
    if (overviewWaveform != nullptr)
    {
        uint_least64_t overviewAdjustedTotalSamples, overviewNumEntries;
        double overviewSamplesPerEntry;
        el::calculate_overview_waveform_details(
                totalFrames, pTrack->getSampleRate(),
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
    auto highResWaveform = pTrack->getWaveform();
    if (highResWaveform != nullptr)
    {
        uint_least64_t highResAdjustedTotalSamples, highResNumEntries;
        double highResSamplesPerEntry;
        el::calculate_high_res_waveform_details(
                totalFrames, pTrack->getSampleRate(),
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
}

void LibraryExportWorker::exportCurrentCrate() {
    if (!m_exportActive) {
        return;
    }

    auto &crateId = m_crateIds[m_currCrateIndex];
    auto &db = *m_pElDb;
    Crate crate;
    m_pTrackCollection->crates().readCrateById(crateId, &crate);
    m_pProgress->setLabelText(tr("Exporting crate") + " " + crate.getName());
    qInfo() << "Exporting crate" << crate.getName();
    auto elCrate = createOrLoadCrate(db, crate.getName());
    elCrate.set_no_parent();
    elCrate.set_name(crate.getName().toStdString());

    // Set track membership
    auto trackIds = GetTracksIdsInCrate(crateId);
    elCrate.clear_tracks();
    for (auto &trackId : trackIds) {
        elCrate.add_track(m_trackIdToElId[trackId.value()]);
    }
    elCrate.save(db);

    // Update progress.
    m_currCrateIndex++;
    m_pProgress->setValue(m_pProgress->value() + 1);
    if (m_currCrateIndex == m_crateIds.count()) {
        // All finished
        emit finishExport();
    }

    // Schedule next crate.
    emit readyForExportCurrentCrate(QPrivateSignal{});
}

void LibraryExportWorker::finishExport() {
    // Copy the DB files over to the final destination.
    copyFilesInDir(m_tempEngineLibraryDir.path(), m_pModel->engineLibraryDir);

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
    emit exportCancelled();
}

void LibraryExportWorker::fail() {
    m_exportActive = false;
    QMessageBox::information(
            dynamic_cast<QWidget *>(parent()),
            tr("Export Failed"),
            tr("Library export failed.  The Mixxx library has only been "
               "partially exported."),
            QMessageBox::Ok,
            QMessageBox::Ok);
    emit exportFailed();
}

QList<TrackId> LibraryExportWorker::GetAllTrackIds() {
    // Obtain a list of all track ids across all directories.
    QList<TrackId> trackIds;
    auto dirs = m_pTrackCollection->getDirectoryDAO().getDirs();
    for (auto iter = dirs.cbegin(); iter != dirs.cend(); ++iter)
        trackIds.append(m_pTrackCollection->getTrackDAO().getTrackIds(*iter));
    return trackIds;
}

QList<TrackId> LibraryExportWorker::GetTracksIdsInCrate(CrateId crateId) {
    QList<TrackId> trackIds;
    auto result = m_pTrackCollection->crates().selectCrateTracksSorted(crateId);
    while (result.next()) {
        trackIds.append(result.trackId());
    }
    return trackIds;
}

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

