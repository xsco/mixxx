#pragma once

#include <memory>

#include <QWidget>

#include "library/export/dlglibraryexport.h"
#include "library/export/libraryexportmodel.h"
#include "library/export/libraryexportworker.h"
#include "preferences/usersettings.h"
#include "util/parented_ptr.h"

namespace djinterop::enginelibrary {
class database;
}

class AnalysisFeature;
class TrackCollection;

namespace mixxx {

// The LibraryExporter class holds both a library export dialog and a library
// export worker, managing the communication between them and allowing the user
// to easily kick off a library export.
class LibraryExporter : public QWidget {
    Q_OBJECT
  public:
    LibraryExporter(QWidget* parent,
            UserSettingsPointer pConfig,
            TrackCollection& trackCollection,
            AnalysisFeature& analysisFeature);

  public slots:
    void requestExport();

  private slots:
    void workBegin(LibraryExportModel model);
    void workEnd();

  private:
    UserSettingsPointer m_pConfig;
    TrackCollection& m_trackCollection;
    AnalysisFeature& m_analysisFeature;
    parented_ptr<DlgLibraryExport> m_pDialog;
    parented_ptr<LibraryExportWorker> m_pWorker;
};

} // namespace mixxx
