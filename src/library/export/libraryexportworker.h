#pragma once

#include <memory>

#include <QList>
#include <QTemporaryDir>
#include <QWidget>

#include "interop/djinteropexport.h"
#include "library/crate/crateid.h"
#include "track/track.h"
#include "track/trackid.h"
#include "util/parented_ptr.h"

class QProgressDialog;

namespace djinterop::enginelibrary {
class database;
}

class AnalysisFeature;
class TrackCollection;

namespace mixxx {

class LibraryExportWorker : public QWidget {
    Q_OBJECT
  public:
    LibraryExportWorker(QWidget *parent,
            DjinteropExportModel model,
            TrackCollection &trackCollection,
            AnalysisFeature &analysisFeature);

    virtual ~LibraryExportWorker();

    void startExport();

  signals:
    void exportFinished();
    void exportCancelled();
    void exportFailed();

    void readyForSetupElDatabase(QPrivateSignal);
    void readyForExportCurrentCrate(QPrivateSignal);

  private slots:
    void setupElDatabase();
    void exportTrack(TrackPointer pTrack);
    void exportCurrentCrate();
    void finishExport();
    void cancel();
    void fail();

  private:
    QString copyFile(TrackPointer pTrack);
    void writeMetadata(TrackPointer pTrack, const QString &dstFilename);
    QList<TrackId> GetAllTrackIds();
    QList<TrackId> GetTracksIdsInCrate(CrateId crateId);
    QList<TrackId> GetTracksIdsInCrates(const QList<CrateId> &crateIds);

    DjinteropExportModel m_model;
    TrackCollection &m_trackCollection;
    AnalysisFeature &m_analysisFeature;

    parented_ptr<QProgressDialog> m_pProgress;
    int m_numTracksDone;
    int m_currCrateIndex;
    QList<TrackId> m_trackIds;
    QList<CrateId> m_crateIds;

    QTemporaryDir m_tempEngineLibraryDir;
    QHash<int, int> m_trackIdToElId;
};

} // namespace mixxx
