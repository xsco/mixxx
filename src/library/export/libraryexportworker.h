#ifndef LIBRARYEXPORTWORKER_H
#define LIBRARYEXPORTWORKER_H

#include <memory>
#include <QObject>
#include <QList>
#include <QProgressDialog>
#include <QTemporaryDir>
#include <djinterop/enginelibrary.hpp>
#include "library/analysisfeature.h"
#include "library/trackcollection.h"
#include "library/crate/crateid.h"
#include "library/export/libraryexportmodel.h"
#include "track/trackid.h"

class TrackCollection;

class LibraryExportWorker : public QObject {
    Q_OBJECT
public:
    LibraryExportWorker(
            QWidget *parent,
            std::shared_ptr<LibraryExportModel> pModel,
            TrackCollection *pTrackCollection,
            AnalysisFeature *pAnalysisFeature);

    virtual ~LibraryExportWorker();

signals:
    void exportFinished();
    void exportCancelled();
    void exportFailed();

    void readyForSetupElDatabase(QPrivateSignal);
    void readyForExportCurrentCrate(QPrivateSignal);

public slots:
    void startExport();

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

    std::shared_ptr<LibraryExportModel> m_pModel;
    TrackCollection *m_pTrackCollection;
    AnalysisFeature *m_pAnalysisFeature;

    std::unique_ptr<QProgressDialog> m_pProgress;
    bool m_exportActive;
    int m_numTracksDone;
    int m_currCrateIndex;
    QList<TrackId> m_trackIds;
    QList<CrateId> m_crateIds;

    QTemporaryDir m_tempEngineLibraryDir;
    std::unique_ptr<djinterop::enginelibrary::database> m_pElDb;
    QHash<int, int> m_trackIdToElId;
};

#endif  // LIBRARYEXPORTWORKER_H
