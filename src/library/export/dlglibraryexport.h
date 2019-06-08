#ifndef DLGLIBRARYEXPORT_H
#define DLGLIBRARYEXPORT_H

#include <QtWidgets>
#include <memory>

#include "library/export/libraryexportmodel.h"
#include "preferences/usersettings.h"
#include "util/parented_ptr.h"

class TrackCollection;

/**
 * The DlgLibraryExport class is a UI window that gathers information from
 * the user about how they would like to export the Mixxx library.
 */
class DlgLibraryExport : public QDialog {
    Q_OBJECT

  public:
    DlgLibraryExport(QWidget* parent,
            UserSettingsPointer pConfig,
            TrackCollection& trackCollection,
            LibraryExportModel& model);

    // Reset dialog (and model) back to its initial state.
    void reset();

  private slots:
    void exportWholeLibrarySelected();
    void exportSelectedCratedSelected();
    void crateSelectionChanged();
    void browseExportDirectory();
    void exportRequested();

  private:
    UserSettingsPointer m_pConfig;
    TrackCollection& m_trackCollection;
    LibraryExportModel& m_model;

    parented_ptr<QRadioButton> m_pWholeLibraryRadio_;
    parented_ptr<QRadioButton> m_pCratesRadio;
    parented_ptr<QListWidget> m_pCratesList;
    parented_ptr<QLineEdit> m_pExportDirTextField;
    parented_ptr<QLineEdit> m_pEngineLibraryDirTextField;
    parented_ptr<QLineEdit> m_pMusicFilesDirTextField;
};

#endif // DLGLIBRARYEXPORT_H
