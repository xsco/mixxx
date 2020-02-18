#include "library/export/libraryexporter.h"

#include <QThreadPool>

#include "library/trackloader.h"
#include "library/export/enginelibraryexportjob.h"

namespace mixxx {

LibraryExporter::LibraryExporter(QWidget *parent,
        UserSettingsPointer pConfig,
        TrackCollectionManager &trackCollectionManager,
        AnalysisFeature &analysisFeature)
        : QWidget{parent},
          m_pConfig{std::move(pConfig)},
          m_trackCollectionManager{trackCollectionManager},
          m_pTrackLoader{nullptr},
          m_analysisFeature{analysisFeature} {
    m_pTrackLoader = new TrackLoader(&m_trackCollectionManager, this);
}

void LibraryExporter::requestExport() {
    if (!m_pDialog) {
        m_pDialog = make_parented<DlgLibraryExport>(this, m_pConfig, m_trackCollectionManager);
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
        this, m_trackCollectionManager, *m_pTrackLoader, m_analysisFeature,
            std::move(request)};
    connect(pJobThread, &EngineLibraryExportJob::finished,
            pJobThread, &QObject::deleteLater);
    pJobThread->start();

    // TODO (mr-smidge) - construct a modal dialog to monitor job progress.
}

} // namespace mixxx
