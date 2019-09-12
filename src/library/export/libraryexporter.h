#pragma once

#include <memory>

#include <QWidget>

#include "library/export/dlglibraryexport.h"
#include "library/export/enginelibraryexportrequest.h"
#include "preferences/usersettings.h"
#include "util/parented_ptr.h"

class AnalysisFeature;
class TrackCollection;

namespace mixxx {

// The LibraryExporter class allows an export of the Mixxx library to be
// initiated.  It can present a dialog that gathers information from the user
// about the nature of the export, and schedules a job to perform the export.
// The class uses libdjinterop to perform the export.
class LibraryExporter : public QWidget {
    Q_OBJECT
  public:
    LibraryExporter(QWidget* parent,
            UserSettingsPointer pConfig,
            TrackCollection& trackCollection,
            AnalysisFeature& analysisFeature);

  public slots:
    // Begin the process of a library export.
    void requestExport();

  private slots:
    void beginEngineLibraryExport(EngineLibraryExportRequest request);

  private:
    UserSettingsPointer m_pConfig;
    TrackCollection& m_trackCollection;
    AnalysisFeature& m_analysisFeature;
    parented_ptr<DlgLibraryExport> m_pDialog;
};

} // namespace mixxx
