#ifndef LIBRARYEXPORTER_H
#define LIBRARYEXPORTER_H

#include <QString>
#include <QScopedPointer>

#include "preferences/usersettings.h"
#include "library/export/dlglibraryexport.h"
#include "library/export/libraryexportworker.h"

// The LibraryExporter class holds both a library export dialog and a library
// export worker, managing the communication between them and allowing the user
// to easily kick off a library export.
class LibraryExporter : public QObject {
  Q_OBJECT
public:
    LibraryExporter(
            QWidget *parent,
            UserSettingsPointer pConfig,
            TrackCollection *pTrackCollection);

    virtual ~LibraryExporter() { }

public slots:
    void requestExport();

private:
    LibraryExportWorker *m_pWorker;
    DlgLibraryExport *m_pDialog;
};

#endif // LIBRARYEXPORTER_H

