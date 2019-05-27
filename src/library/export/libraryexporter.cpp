#include "library/export/libraryexporter.h"
#include <QtCore>

LibraryExporter::LibraryExporter(
        QWidget *parent,
        UserSettingsPointer pConfig,
        TrackCollection *pTrackCollection) :
    QObject{parent},
    m_pWorker{new LibraryExportWorker{parent, pTrackCollection}},
    m_pDialog{new DlgLibraryExport{parent, pConfig, pTrackCollection, m_pWorker->model()}}
{
    m_pDialog->setHidden(true);

    // Connect up dialog and worker.
    connect(
            m_pDialog, SIGNAL(accepted()),
            m_pWorker, SLOT(startExport()));
}

void LibraryExporter::requestExport() {
    m_pDialog->reset();
    m_pDialog->show();
    m_pDialog->raise();
    m_pDialog->activateWindow();
}

