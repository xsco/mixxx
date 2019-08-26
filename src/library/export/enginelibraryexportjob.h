#pragma once

#include <memory>

#include <QSet>

#include "jobs/jobscheduler.h"
#include "library/trackcollection.h"
#include "library/export/enginelibraryexportrequest.h"

namespace mixxx {

// The Engine Library export job performs the work of exporting the Mixxx
// library to an external Engine Library database, using the libdjinterop
// library, in accordance with the export request with which it is constructed.
class EngineLibraryExportJob {
  public:
    EngineLibraryExportJob(
            TrackCollection &trackCollection,
            EngineLibraryExportRequest request);

    void operator()(JobScheduler::Connection conn) const;

  private:
    QSet<TrackId> getAllTrackIds() const;
    QSet<TrackId> getTracksIdsInCrates(
            const QSet<CrateId> &crateIds) const;

    TrackCollection &m_trackCollection;
    EngineLibraryExportRequest m_request;
};

} // namespace mixxx
