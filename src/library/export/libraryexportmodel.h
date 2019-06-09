#ifndef LIBRARYEXPORTMODEL_H
#define LIBRARYEXPORTMODEL_H

#include <QList>
#include <QString>
#include <djinterop/enginelibrary.hpp>

#include "library/crate/crateid.h"

namespace djinterop::enginelibrary {
class database;
}

struct LibraryExportModel {
    static const QString EngineLibraryDirName;
    static const QString MixxxExportDirName;

    std::shared_ptr<djinterop::enginelibrary::database> pDatabase;
    QString engineLibraryDir;
    QString musicFilesDir;
    bool exportEntireMusicLibrary;
    QList<CrateId> selectedCrates;
};

#endif // LIBRARYEXPORTMODEL_H
