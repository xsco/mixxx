#pragma once

#include <memory>

#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QTreeWidget>
#include <QWidget>

#include <boost/optional.hpp>
#include <djinterop/djinterop.hpp>

#include "interop/djinteropexport.h"
#include "preferences/usersettings.h"
#include "util/parented_ptr.h"

namespace djinterop::enginelibrary {
class database;
}

class TrackCollection;

namespace mixxx {

// The DlgLibraryExport class is a UI window that gathers information from
// the user about how they would like to export the Mixxx library.
class DlgLibraryExport : public QDialog {
    Q_OBJECT

  public:
    DlgLibraryExport(
            QWidget* parent, UserSettingsPointer pConfig, TrackCollection& trackCollection);

  signals:
    void startExport(DjinteropExportModel) const;

  private slots:
    void exportWholeLibrarySelected();
    void exportSelectedCratedSelected();
    void browseExportDirectory();
    void updateExternalCratesList();
    void exportRequested();

  private:
    UserSettingsPointer m_pConfig;
    TrackCollection& m_trackCollection;
    boost::optional<djinterop::database> m_database;

    parented_ptr<QListWidget> m_pCratesList;
    parented_ptr<QTreeWidget> m_pExternalCratesTree;
    parented_ptr<QLineEdit> m_pBaseDirectoryTextField;
    parented_ptr<QLineEdit> m_pDatabaseDirectoryTextField;
    parented_ptr<QLineEdit> m_pMusicDirectoryTextField;
};

} // namespace mixxx
