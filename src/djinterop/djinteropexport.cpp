#include "djinterop/djinteropexport.h"

#include <iostream>

namespace el = djinterop::enginelibrary;

namespace mixxx {

namespace {

template<class Inserter>
Inserter insertMappingsForCrate(
        const DjinteropExportModel::CrateMapping& crateMapping, Inserter inserter) {
    auto iter = crateMapping.trackCollection.crates().selectCrateTracksSorted(crateMapping.crateId);
    while (iter.next()) {
        *inserter = DjinteropExportModel::TrackMapping{
                crateMapping.trackCollection, iter.trackId(), crateMapping.externalCrate};
        ++inserter;
    }
    return inserter;
}

std::string exportFile(const DjinteropExportModel::TrackMapping& mapping,
        const boost::optional<std::string>& musicDirectory) {
    QDir dbDirectory{QString::fromStdString(mapping.externalCrate.db().directory())};
    if (!dbDirectory.exists()) {
        // TODO (haslersn): Throw something
    }
    auto pTrack = mapping.trackCollection.getTrackDAO().getTrack(mapping.trackId);
    QFileInfo srcFileInfo = pTrack->getFileInfo();
    if (!musicDirectory) {
        return dbDirectory.relativeFilePath(srcFileInfo.filePath()).toStdString();
    }
    // Copy music files into the Mixxx export dir, if the source file has
    // been modified (or the destination doesn't exist).  To ensure no
    // chance of filename clashes, and to keep things simple, we will name
    // the destination files after the DB track identifier.
    auto trackId = pTrack->getId().value();
    auto dstFilename = std::to_string(trackId) + "." + srcFileInfo.suffix().toStdString();
    QDir dstDirectory{QString::fromStdString(*musicDirectory)};
    dstDirectory.mkpath("."); // creates it if it doesn't exist
    auto dstPath = dstDirectory.filePath(QString::fromStdString(dstFilename));
    if (!QFile::exists(dstPath) || srcFileInfo.lastModified() > QFileInfo{dstPath}.lastModified()) {
        auto srcPath = srcFileInfo.filePath();
        std::cout << "Copying " << srcPath.toStdString() << " to " << dstPath.toStdString() << '\n';
        QFile::copy(srcPath, dstPath);
    }
    return dbDirectory.relativeFilePath(dstPath).toStdString();
}

el::track getTrackByRelativePath(el::database database, const std::string& relativePath) {
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

boost::optional<el::musical_key> toDjinteropKey(mixxx::track::io::key::ChromaticKey key) {
    static const std::array<boost::optional<el::musical_key>, 25> keyMap{
            boost::none,                    // INVALID = 0,
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

    return keyMap[key];
}

void exportMetadata(
        const DjinteropExportModel::TrackMapping& mapping, const std::string& relativePath) {
    auto pTrack = mapping.trackCollection.getTrackDAO().getTrack(mapping.trackId);
    auto db = mapping.externalCrate.db();
    auto externalTrack = getTrackByRelativePath(db, relativePath);
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
    externalTrack.set_relative_path(relativePath);
    std::chrono::system_clock::time_point lastModifiedAt{
            std::chrono::milliseconds{pTrack->getFileModifiedTime().toMSecsSinceEpoch()}};
    externalTrack.set_last_modified_at(lastModifiedAt);
    externalTrack.set_bitrate(pTrack->getBitrate());
    // TODO (haslersn): Should we set the import info?

    mapping.externalCrate.add_track(externalTrack);

    // Frames used interchangeably with "samples" here.
    double sampleCount = pTrack->getDuration() * pTrack->getSampleRate();
    externalTrack.set_sampling({static_cast<double>(pTrack->getSampleRate()), sampleCount});

    // externalTrack.set_average_loudness(...); TODO - set average loudness

    // Fill in beat grid.  For now, assume a constant average BPM across
    // the whole track.  Note that points in the track are specified as
    // "play positions", which are twice the sample offset.
    BeatsPointer beats = pTrack->getBeats();
    if (beats != nullptr) {
        double firstBeatPlayPos = beats->findNextBeat(0);
        double lastBeatPlayPos = beats->findPrevBeat(sampleCount * 2);
        int numBeats = beats->numBeatsInRange(firstBeatPlayPos, lastBeatPlayPos);
        std::vector<el::beatgrid_marker> beatgrid{
                {0, firstBeatPlayPos / 2}, {numBeats, lastBeatPlayPos / 2}};
        beatgrid = el::normalize_beatgrid(std::move(beatgrid), sampleCount);
        externalTrack.set_default_beatgrid(beatgrid);
        externalTrack.set_adjusted_beatgrid(beatgrid);
    }

    auto cues = pTrack->getCuePoints();
    // externalTrack.set_hot_cues(...); TODO - fill in cues
    // externalTrack.set_loops(...); TODO - fill in loops

    // Set main cue-point.
    double cuePlayPos = pTrack->getCuePoint().getPosition();
    externalTrack.set_default_main_cue(cuePlayPos / 2);
    externalTrack.set_adjusted_main_cue(cuePlayPos / 2);

    // Write high-resolution full waveform
    auto waveform = pTrack->getWaveform();
    if (waveform != nullptr) {
        int64_t externalWaveformSize = externalTrack.recommended_waveform_size();
        std::vector<el::waveform_entry> externalWaveform;
        externalWaveform.reserve(externalWaveformSize);
        for (int64_t i = 0; i < externalWaveformSize; ++i) {
            auto j = waveform->getDataSize() * i / externalWaveformSize;
            externalWaveform.push_back({{waveform->getLow(j), 127},
                    {waveform->getMid(j), 127},
                    {waveform->getHigh(j), 127}});
        }
        externalTrack.set_waveform(std::move(externalWaveform));
    }
}

void exportTrack(const DjinteropExportModel::TrackMapping& mapping,
        const boost::optional<std::string>& musicDirectory) {
    auto relativePath = exportFile(mapping, musicDirectory);
    exportMetadata(mapping, relativePath);
}

} // namespace

const std::string DjinteropExportModel::EngineLibraryFolder = "Engine Library";
const std::string DjinteropExportModel::CanonicalMusicFolder = "mixxx-export";

DjinteropExportJob::DjinteropExportJob(DjinteropExportModel model) : m_model{std::move(model)} {
}

void DjinteropExportJob::operator()(JobScheduler::Connection con) const {
    // Local copy because we want this to be a const function
    auto trackMappings = m_model.trackMappings;

    for (const auto& crateMapping : m_model.crateMappings) {
        insertMappingsForCrate(crateMapping, std::back_inserter(trackMappings));
    }

    int32_t tracksDone = 0;
    double tracksAsDouble = static_cast<double>(trackMappings.size()) / 100;
    for (const auto& trackMapping : trackMappings) {
        if (con.terminationRequested()) {
            std::cout << "DjinteropExportJob cancelled\n";
            return;
        }
        exportTrack(trackMapping, m_model.musicDirectory);
        ++tracksDone;
        con.setProgress(static_cast<int32_t>(tracksDone / tracksAsDouble));
    }
    std::cout << "DjinteropExportJob finished\n";
}

} // namespace mixxx
