// TrackExportWizard handles exporting a list of tracks to an external directory
//
// TODO:
//   * Offer customizable file renaming
//   * Offer the option to transcode files to the codec of choice (e.g.,
//     FLAC -> AIFF for CDJ
//   * Export sidecar metadata files for import into Mixxx

#ifndef LIBRARYEXPORTER_H
#define LIBRARYEXPORTER_H

#include <QString>
#include <QScopedPointer>

#include "preferences/usersettings.h"

class LibraryExporter : public QObject {
  Q_OBJECT
  public:
    LibraryExporter(QWidget *parent, UserSettingsPointer pConfig)
        : m_parent(parent), m_pConfig(pConfig)
    {}

    virtual ~LibraryExporter() { }

    // Displays a dialog requesting destination directory, then performs
    // a full library export if a directory is chosen.
    void exportLibrary();

  private:
    QWidget* m_parent;
    UserSettingsPointer m_pConfig;
};

#endif  // LIBRARYEXPORTER_H
