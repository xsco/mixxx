#include "library/export/libraryexporter.h"

#include <QtCore>

LibraryExporter::LibraryExporter(QWidget *parent,
        UserSettingsPointer pConfig,
        TrackCollection &trackCollection,
        AnalysisFeature &analysisFeature)
        : QWidget{parent},
          m_model{},
          m_trackCollection{trackCollection},
          m_analysisFeature{analysisFeature},
          m_pDialog{make_parented<DlgLibraryExport>(parent, pConfig, trackCollection, m_model)} {
    m_pDialog->setHidden(true);
    connect(m_pDialog.get(), SIGNAL(accepted()), this, SLOT(startExport()));
}

void LibraryExporter::requestExport() {
    m_pDialog->reset();
    m_pDialog->show();
    m_pDialog->raise();
    m_pDialog->activateWindow();
}

void LibraryExporter::startExport() {
    m_pWorker =
            make_parented<LibraryExportWorker>(this, m_model, m_trackCollection, m_analysisFeature);

    // Start export by calling slot directly.
    m_pWorker->startExport();
}
