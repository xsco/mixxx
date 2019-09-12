#include "library/export/libraryexporter.h"

#include <QThreadPool>

#include "library/export/enginelibraryexportjob.h"

namespace mixxx {

LibraryExporter::LibraryExporter(QWidget *parent,
        UserSettingsPointer pConfig,
        TrackCollection &trackCollection,
        AnalysisFeature &analysisFeature)
        : QWidget{parent},
          m_pConfig{std::move(pConfig)},
          m_trackCollection{trackCollection},
          m_analysisFeature{analysisFeature} {
}

void LibraryExporter::requestExport() {
    if (!m_pDialog) {
        m_pDialog = make_parented<DlgLibraryExport>(this, m_pConfig, m_trackCollection);
        connect(m_pDialog.get(),
                SIGNAL(startEngineLibraryExport(EngineLibraryExportRequest)),
                this,
                SLOT(beginEngineLibraryExport(EngineLibraryExportRequest)));
    } else {
        m_pDialog->show();
        m_pDialog->raise();
        m_pDialog->setWindowState(
                (m_pDialog->windowState() & ~Qt::WindowMinimized) |
                Qt::WindowActive);
    }
}

void LibraryExporter::beginEngineLibraryExport(
        EngineLibraryExportRequest request) {
    // Run the job in a background thread.
    auto *pJobThread = new EngineLibraryExportJob{
        this, m_trackCollection, m_analysisFeature, std::move(request)};
    connect(pJobThread, &EngineLibraryExportJob::finished,
            pJobThread, &QObject::deleteLater);
    pJobThread->start();

    // TODO (mr-smidge) - construct a modal dialog to monitor job progress.
}

} // namespace mixxx
