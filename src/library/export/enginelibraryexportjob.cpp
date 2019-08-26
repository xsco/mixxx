#include "library/export/enginelibraryexportjob.h"

#include <iostream>

#include <djinterop/djinterop.hpp>

#include "library/crate/crate.h"
#include "library/trackcollection.h"
#include "track/track.h"

namespace el = djinterop::enginelibrary;
namespace stdex = std::experimental;

namespace mixxx {

namespace {

const std::string MixxxRootCrateName = "Mixxx";

stdex::optional<djinterop::musical_key> toDjinteropKey(
        mixxx::track::io::key::ChromaticKey key) {
    static const std::array<stdex::optional<djinterop::musical_key>, 25> keyMap{{
        stdex::nullopt,                        // INVALID = 0,
        djinterop::musical_key::c_major,       // C_MAJOR = 1,
        djinterop::musical_key::d_flat_major,  // D_FLAT_MAJOR = 2,
        djinterop::musical_key::d_major,       // D_MAJOR = 3,
        djinterop::musical_key::e_flat_major,  // E_FLAT_MAJOR = 4,
        djinterop::musical_key::e_major,       // E_MAJOR = 5,
        djinterop::musical_key::f_major,       // F_MAJOR = 6,
        djinterop::musical_key::f_sharp_major, // F_SHARP_MAJOR = 7,
        djinterop::musical_key::g_major,       // G_MAJOR = 8,
        djinterop::musical_key::a_flat_major,  // A_FLAT_MAJOR = 9,
        djinterop::musical_key::a_major,       // A_MAJOR = 10,
        djinterop::musical_key::b_flat_major,  // B_FLAT_MAJOR = 11,
        djinterop::musical_key::b_major,       // B_MAJOR = 12,
        djinterop::musical_key::c_minor,       // C_MINOR = 13,
        djinterop::musical_key::d_flat_minor,  // C_SHARP_MINOR = 14,
        djinterop::musical_key::d_minor,       // D_MINOR = 15,
        djinterop::musical_key::e_flat_minor,  // E_FLAT_MINOR = 16,
        djinterop::musical_key::e_minor,       // E_MINOR = 17,
        djinterop::musical_key::f_minor,       // F_MINOR = 18,
        djinterop::musical_key::f_sharp_minor, // F_SHARP_MINOR = 19,
        djinterop::musical_key::g_minor,       // G_MINOR = 20,
        djinterop::musical_key::a_flat_minor,  // G_SHARP_MINOR = 21,
        djinterop::musical_key::a_minor,       // A_MINOR = 22,
        djinterop::musical_key::b_flat_minor,  // B_FLAT_MINOR = 23,
        djinterop::musical_key::b_minor,       // B_MINOR = 24
    }};

    return keyMap[key];
}

std::string exportFile(const EngineLibraryExportRequest& request,
        TrackPointer pTrack) {
    if (!request.engineLibraryDbDir.exists() || !request.musicFilesDir.exists()) {
        // TODO (haslersn): Throw something
    }

    // Copy music files into the Mixxx export dir, if the source file has
    // been modified (or the destination doesn't exist).  To ensure no
    // chance of filename clashes, and to keep things simple, we will name
    // the destination files after the DB track identifier.
    TrackFile srcFileInfo = pTrack->getFileInfo();
    auto trackId = pTrack->getId().value();
    auto dstFilename = std::to_string(trackId) + "." +
        srcFileInfo.asFileInfo().suffix().toStdString();
    auto dstPath = request.musicFilesDir.filePath(QString::fromStdString(dstFilename));
    if (!QFile::exists(dstPath) || srcFileInfo.fileLastModified() > QFileInfo{dstPath}.lastModified()) {
        auto srcPath = srcFileInfo.location();
        QFile::copy(srcPath, dstPath);
    }

    return request.engineLibraryDbDir.relativeFilePath(dstPath).toStdString();
}

djinterop::track getTrackByRelativePath(
        djinterop::database database, const std::string& relativePath) {
    auto trackCandidates = database.tracks_by_relative_path(relativePath);
    switch (trackCandidates.size()) {
    case 0:
        return database.create_track(relativePath);
    case 1:
        return trackCandidates.front();
    default:
        std::cout << "Warning: More than one external track with the same relative path";
        return trackCandidates.front();
    }
}

void exportMetadata(djinterop::database &db,
        QHash<TrackId, int64_t>& mixxxToEngineLibraryTrackIdMap,
        TrackPointer pTrack, const std::string& relativePath) {
    // Create or load the track in the EL database, using the relative path to
    // the music file.  We will record the mapping from Mixxx track id to EL
    // track id as well.
    auto externalTrack = getTrackByRelativePath(db, relativePath);
    mixxxToEngineLibraryTrackIdMap.insert(pTrack->getId(), externalTrack.id());

    // Note that the Engine Library format has the scope for recording meta-data
    // about whteher track was imported from an external database.  However,
    // that meta-data only extends as far as other Engine Library databases,
    // which Mixxx is not.  So we do not set any import information on the EL
    // track.
    externalTrack.set_track_number(pTrack->getTrackNumber().toInt());
    externalTrack.set_bpm(pTrack->getBpm());
    externalTrack.set_year(pTrack->getYear().toInt());
    externalTrack.set_title(pTrack->getTitle().toStdString());
    externalTrack.set_artist(pTrack->getArtist().toStdString());
    externalTrack.set_album(pTrack->getAlbum().toStdString());
    externalTrack.set_genre(pTrack->getGenre().toStdString());
    externalTrack.set_comment(pTrack->getComment().toStdString());
    externalTrack.set_composer(pTrack->getComposer().toStdString());
    externalTrack.set_key(toDjinteropKey(pTrack->getKey()));
    int64_t lastModifiedMillisSinceEpoch =
        pTrack->getFileInfo().fileLastModified().toMSecsSinceEpoch();
    std::chrono::system_clock::time_point lastModifiedAt{
            std::chrono::milliseconds{lastModifiedMillisSinceEpoch}};
    externalTrack.set_last_modified_at(lastModifiedAt);
    externalTrack.set_bitrate(pTrack->getBitrate());

    // Frames used interchangeably with "samples" here.
    auto sampleCount = static_cast<int64_t>(pTrack->getDuration() * pTrack->getSampleRate());
    externalTrack.set_sampling({static_cast<double>(pTrack->getSampleRate()), sampleCount});

    // TODO (mrsmidge) - Set average loudness.
    externalTrack.set_average_loudness(0.5f);

    // Fill in beat grid.  For now, assume a constant average BPM across
    // the whole track.  Note that points in the track are specified as
    // "play positions", which are twice the sample offset.
    BeatsPointer beats = pTrack->getBeats();
    if (beats != nullptr) {
        double firstBeatPlayPos = beats->findNextBeat(0);
        double lastBeatPlayPos = beats->findPrevBeat(sampleCount * 2);
        int numBeats = beats->numBeatsInRange(firstBeatPlayPos, lastBeatPlayPos);
        std::vector<djinterop::beatgrid_marker> beatgrid{
                {0, firstBeatPlayPos / 2}, {numBeats, lastBeatPlayPos / 2}};
        beatgrid = el::normalize_beatgrid(std::move(beatgrid), sampleCount);
        externalTrack.set_default_beatgrid(beatgrid);
        externalTrack.set_adjusted_beatgrid(beatgrid);
    }

    auto cues = pTrack->getCuePoints();
    for (const CuePointer &pCue : cues) {
        // We are only interested in hot cues.
        if (pCue.get() == nullptr || pCue->getType() != Cue::CUE) {
            continue;
        }

        int hot_cue_index = pCue->getHotCue(); // Note: Mixxx uses 0-based.
        if (hot_cue_index < 0 || hot_cue_index >= 8) {
            // EL only supports a maximum of 8 hot cues.
            qInfo() << "Skipping hot cue" << hot_cue_index
                << "as the Engine Library format only supports at most 8"
                << "hot cues.";
            continue;
        }

        QString label = pCue->getLabel();
        if (label == "") {
            label = QString("Cue %1").arg(hot_cue_index + 1);
        }

        djinterop::hot_cue hc{};
        hc.label = label.toStdString();
        hc.sample_offset = pCue->getPosition() / 2; // Convert "play pos".
        hc.color = el::standard_pad_colors::pads[hot_cue_index];
        externalTrack.set_hot_cue_at(hot_cue_index, hc);
    }
    
    // Set main cue-point.
    double cuePlayPos = pTrack->getCuePoint().getPosition();
    externalTrack.set_default_main_cue(cuePlayPos / 2);
    externalTrack.set_adjusted_main_cue(cuePlayPos / 2);

    // Note that Mixxx does not support pre-calculated stored loops, but it will
    // remember the position of a single ad-hoc loop between track loads.
    // However, since this single ad-hoc loop is likely to be different in use
    // from a set of stored loops (and is easily overwritten), we do not export
    // it to the Engine Library database here.
    externalTrack.set_loops({});

    // Write high-resolution full waveform
    auto waveform = pTrack->getWaveform();
    if (waveform != nullptr) {
        int64_t externalWaveformSize = externalTrack.recommended_waveform_size();
        std::vector<djinterop::waveform_entry> externalWaveform;
        externalWaveform.reserve(externalWaveformSize);
        for (int64_t i = 0; i < externalWaveformSize; ++i) {
            auto j = waveform->getDataSize() * i / externalWaveformSize;
            externalWaveform.push_back({{waveform->getLow(j), 127},
                    {waveform->getMid(j), 127},
                    {waveform->getHigh(j), 127}});
        }
        externalTrack.set_waveform(std::move(externalWaveform));
    }

    qInfo() << "Wrote all meta-data for track";
}

void exportTrack(JobScheduler::Connection conn,
        TrackCollection &trackCollection,
        const EngineLibraryExportRequest& request, djinterop::database &db,
        QHash<TrackId, int64_t>& mixxxToEngineLibraryTrackIdMap,
        const TrackId& trackId) {
    // TODO (mrsmidge) - schedule for analysis, await completion.

    // Load track (synchronously, as TrackCollection should not be accessed in
    // an unchecked parallel manner).
    TrackPointer pTrack =
            conn.runSync([&]() {
                   return trackCollection.getTrackDAO().getTrack(trackId);
               }).get();

    // Copy the file, if required.
    auto musicFileRelativePath = exportFile(request, pTrack);

    // Export meta-data.
    exportMetadata(db, mixxxToEngineLibraryTrackIdMap, pTrack,
            musicFileRelativePath);
}

void exportCrate(JobScheduler::Connection conn,
        TrackCollection &trackCollection, djinterop::crate &extRootCrate,
        QHash<TrackId, int64_t>& mixxxToEngineLibraryTrackIdMap,
        const CrateId &crateId) {
    // Load the crate (synchronously, as TrackCollection should not be accessed
    // in an unchecked parallel manner).
    Crate crate;
    conn.runSync([&]() {
           trackCollection.crates().readCrateById(crateId, &crate);
       }).get();
    
    // Create a new crate as a sub-crate of the top-level Mixxx crate.
    auto extCrate = extRootCrate.create_sub_crate(crate.getName().toStdString());

    // Loop through all track ids in this crate and add.
    auto result = trackCollection.crates().selectCrateTracksSorted(crateId);
    while (result.next()) {
        auto extTrackId = mixxxToEngineLibraryTrackIdMap[result.trackId()];
        extCrate.add_track(extTrackId);
    }
}

} // namespace


// Obtain a set of all track ids across all directories in the whole Mixxx library.
QSet<TrackId> EngineLibraryExportJob::getAllTrackIds() const {
    QSet<TrackId> trackIds;
    auto dirs = m_trackCollection.getDirectoryDAO().getDirs();
    for (auto iter = dirs.cbegin(); iter != dirs.cend(); ++iter)
        trackIds.unite(m_trackCollection.getTrackDAO().getTrackIds(*iter).toSet());
    return trackIds;
}

// Obtain a set of track ids in a set of crates.
QSet<TrackId> EngineLibraryExportJob::getTracksIdsInCrates(
        const QSet<CrateId> &crateIds) const {
    QSet<TrackId> trackIds;
    for (auto iter = crateIds.cbegin(); iter != crateIds.cend(); ++iter) {
        auto result = m_trackCollection.crates().selectCrateTracksSorted(*iter);
        while (result.next()) {
            trackIds.insert(result.trackId());
        }
    }
    return trackIds;
}

EngineLibraryExportJob::EngineLibraryExportJob(
        TrackCollection &trackCollection, EngineLibraryExportRequest request)
        : m_trackCollection(trackCollection), m_request{std::move(request)} {
}

void EngineLibraryExportJob::operator()(JobScheduler::Connection conn) const {

    // Crate music directory if it doesn't already exist.
    QDir().mkpath(m_request.musicFilesDir.path());

    // Determine how many tracks and crates we have to export, and use to
    // calculate job progress.
    QSet<TrackId> trackIds;
    QSet<CrateId> crateIds;
    if (m_request.exportSelectedCrates) {
        trackIds = getTracksIdsInCrates(m_request.crateIdsToExport);
        crateIds = m_request.crateIdsToExport;
    }
    else {
        // Note that we do not currently export empty crates.
        trackIds = getAllTrackIds();
        crateIds = m_trackCollection.crates().collectCrateIdsOfTracks(
                trackIds.toList());
    }

    // Measure progress as one 'count' for each track, each crate, plus an
    // additional count for setting up the database at the start.
    double maxProgress = trackIds.size() + crateIds.size() + 1;
    double currProgress = 0;
    conn.setProgress(currProgress / maxProgress);

    // Ensure that the database exists, creating an empty one if not.
    bool created;
    auto db = el::create_or_load_database(
            m_request.engineLibraryDbDir.path().toStdString(),
            el::version_latest, created);
    ++currProgress;
    conn.setProgress(currProgress / maxProgress);
  
    // Export all relevant tracks first, building up a map from Mixxx track id
    // to EL track id as we go.
    QHash<TrackId, int64_t> mixxxToEngineLibraryTrackIdMap;
    for (const TrackId& trackId : trackIds) {
        qInfo() << "Exporting track" << trackId.value() << "...";
        exportTrack(conn, m_trackCollection, m_request, db,
                mixxxToEngineLibraryTrackIdMap, trackId);

        ++currProgress;
        conn.setProgress(currProgress / maxProgress);
    }

    // We will ensure that there is a special top-level crate representing the
    // root of all Mixxx-exported items.  Mixxx tracks and crates will exist
    // underneath this crate.
    auto optionalExtRootCrate = db.root_crate_by_name(MixxxRootCrateName);
    auto extRootCrate = optionalExtRootCrate
        ? optionalExtRootCrate.value()
        : db.create_root_crate(MixxxRootCrateName);
    for (const TrackId& trackId : trackIds) {
        // Add each track to the root crate, even if it also belongs to others.
        auto extTrackId = mixxxToEngineLibraryTrackIdMap[trackId];
        extRootCrate.add_track(extTrackId);
    }

    // Export all Mixxx crates
    for (const CrateId& crateId : crateIds) {
        qInfo() << "Exporting crate" << crateId.value() << "...";
        exportCrate(conn, m_trackCollection, extRootCrate,
                mixxxToEngineLibraryTrackIdMap, crateId);

        ++currProgress;
        conn.setProgress(currProgress / maxProgress);
    }

    std::cout << "Engine Library Export Job completed successfully" << std::endl;
}

} // namespace mixxx
