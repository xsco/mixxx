#ifndef LIBRARYEXPORTWORKER_H
#define LIBRARYEXPORTWORKER_H

#include <QList>
#include <QObject>
#include <QProgressDialog>
#include <QTemporaryDir>

#include "library/analysisfeature.h"
#include "library/crate/crateid.h"
#include "library/export/libraryexportmodel.h"
#include "library/trackcollection.h"
#include "track/trackid.h"
#include "util/memory.h"
#include "util/parented_ptr.h"

namespace djinterop::enginelibrary {
class database;
}

class TrackCollection;

class LibraryExportWorker : public QWidget {
    Q_OBJECT
  public:
    LibraryExportWorker(QWidget *parent,
            LibraryExportModel model,
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

    LibraryExportModel m_model;
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

#endif // LIBRARYEXPORTWORKER_H
