#pragma once

#include <memory>

#include <boost/optional.hpp>
#include <djinterop/djinterop.hpp>

#include "jobs/jobscheduler.h"
#include "library/crate/crate.h"
#include "library/trackcollection.h"
#include "track/track.h"

namespace mixxx {

struct DjinteropExportModel {
    static const std::string EngineLibraryFolder;
    static const std::string CanonicalMusicFolder;

    struct TrackMapping {
        TrackMapping(
                TrackCollection& trackCollection, TrackId trackId, djinterop::crate externalCrate)
                : trackCollection{trackCollection},
                  trackId{std::move(trackId)},
                  externalCrate{std::move(externalCrate)} {
        }

        TrackCollection& trackCollection;
        TrackId trackId;
        djinterop::crate externalCrate;
    };

    struct CrateMapping {
        CrateMapping(
                TrackCollection& trackCollection, CrateId crateId, djinterop::crate externalCrate)
                : trackCollection{trackCollection},
                  crateId{std::move(crateId)},
                  externalCrate{std::move(externalCrate)} {
        }

        TrackCollection& trackCollection;
        CrateId crateId;
        djinterop::crate externalCrate;
    };

    // If given, then the music should be copied to `*musicDirectory`, which must be set relative to
    // `externalCrate.db().directory()`.
    // TODO (haslersn): Should this be mandatory (instead of optional)?
    boost::optional<std::string> musicDirectory;

    std::vector<TrackMapping> trackMappings;
    std::vector<CrateMapping> crateMappings;
};

class DjinteropExportJob {
  public:
    DjinteropExportJob(DjinteropExportModel model);

    void operator()(JobScheduler::Connection con) const;

  private:
    DjinteropExportModel m_model;
};

} // namespace mixxx
