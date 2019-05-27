#ifndef DLGLIBRARYEXPORT_H
#define DLGLIBRARYEXPORT_H

#include <QtWidgets>
#include "preferences/usersettings.h"
#include "library/export/libraryexportmodel.h"

class TrackCollection;

/**
 * The DlgLibraryExport class is a UI window that gathers information from
 * the user about how they would like to export the Mixxx library.
 */
class DlgLibraryExport : public QDialog
{
    Q_OBJECT

public:
    DlgLibraryExport(
            QWidget *parent,
            UserSettingsPointer pConfig,
            TrackCollection *pTrackCollection,
            LibraryExportModel &model);

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

    QRadioButton *wholeLibraryRadio_;
    QRadioButton *cratesRadio_;
    QListWidget *cratesList_;
    QLineEdit *exportDirTextField_;
    QLineEdit *engineLibraryDirTextField_;
    QLineEdit *musicFilesDirTextField_;

    LibraryExportModel &m_model;
};

#endif // DLGLIBRARYEXPORT_H

