#ifndef DLGLIBRARYEXPORT_H
#define DLGLIBRARYEXPORT_H

#include <QtWidgets>
#include <memory>
#include "library/export/libraryexportmodel.h"
#include "preferences/usersettings.h"

class TrackCollection;

/**
 * The DlgLibraryExport class is a UI window that gathers information from
 * the user about how they would like to export the Mixxx library.
 */
class DlgLibraryExport : public QDialog {
    Q_OBJECT

  public:
    DlgLibraryExport(QWidget *parent,
            UserSettingsPointer pConfig,
            TrackCollection *pTrackCollection,
            std::shared_ptr<LibraryExportModel> pModel);

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
    TrackCollection *m_pTrackCollection;

    QRadioButton *m_pWholeLibraryRadio_;
    QRadioButton *m_pCratesRadio;
    QListWidget *m_pCratesList;
    QLineEdit *m_pExportDirTextField;
    QLineEdit *m_pEngineLibraryDirTextField;
    QLineEdit *m_pMusicFilesDirTextField;

    std::shared_ptr<LibraryExportModel> m_pModel;
};

#endif // DLGLIBRARYEXPORT_H
