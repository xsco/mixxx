#ifndef LIBRARYEXPORTMODEL_H
#define LIBRARYEXPORTMODEL_H

#include <QSet>
#include <QString>

struct LibraryExportModel {
    QString engineLibraryDir;
    QString musicFilesDir;
    bool exportEntireMusicLibrary;
    QSet<QString> selectedCrates;

    void clear() {
        engineLibraryDir.clear();
        musicFilesDir.clear();
        exportEntireMusicLibrary = false;
        selectedCrates.clear();
    }
};

#endif  // LIBRARYEXPORTMODEL_H
