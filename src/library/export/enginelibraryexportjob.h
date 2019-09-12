#pragma once

#include <memory>

#include <QMutex>
#include <QSet>
#include <QThread>
#include <QVector>
#include <QWaitCondition>

#include "library/analysisfeature.h"
#include "library/trackcollection.h"
#include "library/export/enginelibraryexportrequest.h"

namespace mixxx {

// The Engine Library export job performs the work of exporting the Mixxx
// library to an external Engine Library database, using the libdjinterop
// library, in accordance with the export request with which it is constructed.
class EngineLibraryExportJob : public QThread {
    Q_OBJECT
  public:
    EngineLibraryExportJob(
            QObject* parent, TrackCollection& trackCollection,
            AnalysisFeature& analysisFeature,
            EngineLibraryExportRequest request);

    void run() override;

  signals:
    void analyzeTracks(QList<TrackId> trackIds);
    void jobProgress(double progress);

  private slots:
    void analysisActive(bool bActive);
    void trackAnalysisDone(TrackId trackId);

  private:
    QSet<TrackId> getAllTrackIds() const;
    QSet<TrackId> getTracksIdsInCrates(
            const QSet<CrateId> &crateIds) const;

    bool m_allTrackAnalysisDone;
    QVector<TrackId> m_analysedTrackQueue;
    QMutex m_trackMutex;
    QWaitCondition m_waitAnyTrack;

    TrackCollection& m_trackCollection;
    AnalysisFeature& m_analysisFeature;
    EngineLibraryExportRequest m_request;
};

} // namespace mixxx
