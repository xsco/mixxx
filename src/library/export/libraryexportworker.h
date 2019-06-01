#ifndef LIBRARYEXPORTWORKER_H
#define LIBRARYEXPORTWORKER_H

#include <QObject>
#include <QList>
#include <QProgressDialog>
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
            QSharedPointer<LibraryExportModel> pModel,
			TrackCollection *pTrackCollection,
            AnalysisFeature *pAnalysisFeature) :
        QObject{parent},
        m_pModel{pModel},
		m_pTrackCollection{pTrackCollection},
        m_pAnalysisFeature{pAnalysisFeature},
        m_exportActive{false},
        m_currTrackIndex{0},
        m_currCrateIndex{0}
    {
    }

    virtual ~LibraryExportWorker();

signals:
    void exportFinished();

public slots:
    void startExport();

private slots:
    void exportCurrentTrack(TrackPointer track);
    void exportCurrentCrate();
    void finishExport();
    void cancel();

private:
    QList<TrackId> GetAllTrackIds();
    QList<TrackId> GetTracksIdsInCrates(const QList<CrateId> &crateIds);

    QSharedPointer<LibraryExportModel> m_pModel;
    TrackCollection *m_pTrackCollection;
    AnalysisFeature *m_pAnalysisFeature;

    QScopedPointer<QProgressDialog> m_pProgress;
    bool m_exportActive;
    int m_currTrackIndex;
    int m_currCrateIndex;
    QList<TrackId> m_trackIds;
    QList<CrateId> m_crateIds;
};

#endif  // LIBRARYEXPORTWORKER_H
