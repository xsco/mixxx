#ifndef LIBRARYEXPORTER_H
#define LIBRARYEXPORTER_H

#include <QString>

#include "library/analysisfeature.h"
#include "library/export/dlglibraryexport.h"
#include "library/export/libraryexportworker.h"
#include "library/trackcollection.h"
#include "preferences/usersettings.h"
#include "util/memory.h"
#include "util/parented_ptr.h"

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

#endif // LIBRARYEXPORTER_H
