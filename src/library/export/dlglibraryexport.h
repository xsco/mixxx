#ifndef DLGLIBRARYEXPORT_H
#define DLGLIBRARYEXPORT_H

#include <QtWidgets>

#include "library/export/libraryexportmodel.h"
#include "preferences/usersettings.h"
#include "util/memory.h"
#include "util/parented_ptr.h"

class TrackCollection;

/**
 * The DlgLibraryExport class is a UI window that gathers information from
 * the user about how they would like to export the Mixxx library.
 */
class DlgLibraryExport : public QDialog {
    Q_OBJECT

  public:
    DlgLibraryExport(
            QWidget* parent, UserSettingsPointer pConfig, TrackCollection& trackCollection);

  signals:
    void startExport(LibraryExportModel) const;

  private slots:
    void exportWholeLibrarySelected();
    void exportSelectedCratedSelected();
    void browseExportDirectory();
    void exportRequested();

  private:
    UserSettingsPointer m_pConfig;
    TrackCollection& m_trackCollection;
    LibraryExportModel m_model;

    parented_ptr<QListWidget> m_pCratesList;
    parented_ptr<QLineEdit> m_pExportDirTextField;
    parented_ptr<QLineEdit> m_pEngineLibraryDirTextField;
    parented_ptr<QLineEdit> m_pMusicFilesDirTextField;
};

#endif // DLGLIBRARYEXPORT_H
