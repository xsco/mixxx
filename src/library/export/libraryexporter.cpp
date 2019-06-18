#include "library/export/libraryexporter.h"

#include "library/crate/crateid.h"
#include "library/export/dlglibraryexport.h"
#include "library/export/libraryexportworker.h"

namespace mixxx {

LibraryExporter::LibraryExporter(QWidget *parent,
        UserSettingsPointer pConfig,
        TrackCollection &trackCollection,
        AnalysisFeature &analysisFeature)
        : QWidget{parent},
          m_pConfig{pConfig},
          m_trackCollection{trackCollection},
          m_analysisFeature{analysisFeature} {
}

void LibraryExporter::requestExport() {
    if (!m_pDialog) {
        m_pDialog = make_parented<DlgLibraryExport>(this, m_pConfig, m_trackCollection);
        connect(m_pDialog.get(),
                SIGNAL(startExport(LibraryExportModel)),
                this,
                SLOT(workBegin(LibraryExportModel)));
    } else {
        m_pDialog->show();
        m_pDialog->raise();
        m_pDialog->setWindowState(
                (m_pDialog->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
    }
}

void LibraryExporter::workBegin(LibraryExportModel model) {
    if (!m_pWorker) {
        m_pWorker = make_parented<LibraryExportWorker>(
                this, model, m_trackCollection, m_analysisFeature);
        connect(m_pWorker.get(), SIGNAL(exportFinished()), m_pDialog.get(), SLOT(accept()));
        connect(m_pWorker.get(), SIGNAL(exportFinished()), this, SLOT(workEnd()));
        connect(m_pWorker.get(), SIGNAL(exportCancelled()), this, SLOT(workEnd()));
        connect(m_pWorker.get(), SIGNAL(exportFailed()), this, SLOT(workEnd()));
        m_pWorker->startExport();
    } else {
        // TODO: activate the worker
    }
}

void LibraryExporter::workEnd() {
    qWarning() << "workEnd";
    m_pWorker = nullptr;
}

} // namespace mixxx
