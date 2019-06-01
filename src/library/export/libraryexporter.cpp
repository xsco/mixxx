#include "library/export/libraryexporter.h"
#include <QtCore>

LibraryExporter::LibraryExporter(
        QWidget *parent,
        UserSettingsPointer pConfig,
        TrackCollection *pTrackCollection,
        AnalysisFeature *pAnalysisFeature) :
    QObject{parent},
    m_pModel{new LibraryExportModel{}},
    m_pTrackCollection{pTrackCollection},
    m_pAnalysisFeature{pAnalysisFeature},
    m_pDialog{new DlgLibraryExport{
        parent, pConfig, pTrackCollection, m_pModel}}
{
    m_pDialog->setHidden(true);
    connect(m_pDialog, SIGNAL(accepted()), this, SLOT(startExport()));
}

void LibraryExporter::requestExport() {
    m_pDialog->reset();
    m_pDialog->show();
    m_pDialog->raise();
    m_pDialog->activateWindow();
}

void LibraryExporter::startExport() {
    auto *worker = new LibraryExportWorker{
        dynamic_cast<QWidget *>(parent()),
        m_pModel,
        m_pTrackCollection,
        m_pAnalysisFeature};
    connect(worker, SIGNAL(exportFinished()), worker, SLOT(deleteLater()));

    // Start export by calling slot directly.
    worker->startExport();
}

