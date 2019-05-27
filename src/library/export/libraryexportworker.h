#ifndef LIBRARYEXPORTWORKER_H
#define LIBRARYEXPORTWORKER_H

#include <QObject>
#include "library/export/libraryexportmodel.h"

class TrackCollection;

class LibraryExportWorker : public QObject {
    Q_OBJECT
public:
    LibraryExportWorker(
    		QObject *parent,
			TrackCollection *pTrackCollection) :
        QObject{parent},
		m_pTrackCollection{pTrackCollection}
    {
        m_model.clear();
    }

    LibraryExportModel &model() { return m_model; }

public slots:
    void startExport();

private:
    LibraryExportModel m_model;
    TrackCollection *m_pTrackCollection;
};

#endif  // LIBRARYEXPORTWORKER_H
