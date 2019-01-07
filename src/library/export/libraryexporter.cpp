#include "library/export/libraryexporter.h"

#include <QtGlobal>
#include <QFileDialog>
#include <QFileInfo>
#include <QDesktopServices>
#include <QMessageBox>

#include <djinterop/enginelibrary.hpp>

#include "library/trackcollection.h"
#include "util/assert.h"

namespace el = djinterop::enginelibrary;

void LibraryExporter::exportLibrary() {

    // For now, we can only export in Engine Prime / Engine Library format
    // In this initial implementation, ask for an output directory and then
    // export the whole library.
    QString lastExportDirectory = m_pConfig->getValue(
            ConfigKey("[Library]", "LastLibraryExportDirectory"),
            QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation));

    QString destDir = QFileDialog::getExistingDirectory(
            NULL, tr("Export Library To"), lastExportDirectory);
    if (destDir.isEmpty()) {
        return;
    }
    m_pConfig->set(ConfigKey("[Library]", "LastLibraryExportDirectory"),
                   ConfigValue(destDir));

    // TODO - check to see if destDir ends in "Engine Library" or not, and then
    // ensure we have a root folder to work with.

    // TODO - check for presence of any existing database.  If there is already
    // one, prompt for whether to merge into it or not.
    el::database db{destDir.toStdString()};
    qInfo() << "Does DB exist? " << db.exists();

    // TODO - not yet implemented - create new database if required
    // TODO - copy music files into a new "Mixxx Music" sub-directory
    // TODO - write DB metadata
}
