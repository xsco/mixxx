#include "library/export/libraryexporter.h"

#include "djinterop/djinteropexport.h"
#include "library/crate/crateid.h"

namespace mixxx {

LibraryExporter::LibraryExporter(QWidget *parent,
        UserSettingsPointer pConfig,
        TrackCollection &trackCollection,
        AnalysisFeature &analysisFeature,
        std::shared_ptr<JobScheduler> pScheduler)
        : QWidget{parent},
          m_pConfig{std::move(pConfig)},
          m_trackCollection{trackCollection},
          m_analysisFeature{analysisFeature},
          m_pScheduler{std::move(pScheduler)} {
}

void LibraryExporter::requestExport() {
    if (!m_pDialog) {
        m_pDialog = make_parented<DlgLibraryExport>(this, m_pConfig, m_trackCollection);
        connect(m_pDialog.get(),
                SIGNAL(startExport(DjinteropExportModel)),
                this,
                SLOT(workBegin(DjinteropExportModel)));
    } else {
        m_pDialog->show();
        m_pDialog->raise();
        m_pDialog->setWindowState(
                (m_pDialog->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
    }
}

void LibraryExporter::workBegin(DjinteropExportModel model) {
    m_pScheduler->run(DjinteropExportJob{std::move(model)});
}

} // namespace mixxx
