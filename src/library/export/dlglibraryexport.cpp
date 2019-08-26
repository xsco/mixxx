#include "library/export/dlglibraryexport.h"

#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QStandardPaths>

#include <djinterop/enginelibrary.hpp>

#include "library/crate/crateid.h"
#include "library/export/enginelibraryexportrequest.h"
#include "library/trackcollection.h"

static const QString DefaultMixxxExportDirName = "mixxx-export";

namespace el = djinterop::enginelibrary;

namespace mixxx {

DlgLibraryExport::DlgLibraryExport(
        QWidget* parent, UserSettingsPointer pConfig, TrackCollection& trackCollection)
        : QDialog(parent), m_pConfig{pConfig}, m_trackCollection{trackCollection} {
    // Selectable list of crates from the Mixxx library.
    m_pCratesList = make_parented<QListWidget>();
    m_pCratesList->setSelectionMode(QListWidget::ExtendedSelection);
    {
        // Populate list of crates.
        // TODO (haslersn): Also update this if crates are created/removed
        auto crates = m_trackCollection.crates().selectCrates();
        Crate crate;
        while (crates.populateNext(&crate)) {
            auto pItem = std::make_unique<QListWidgetItem>(crate.getName());
            pItem->setData(Qt::UserRole, crate.getId().value());
            m_pCratesList->addItem(pItem.release());
        }
    }

    // Read-only text fields showing key directories for export.
    m_pBaseDirectoryTextField = make_parented<QLineEdit>();
    m_pBaseDirectoryTextField->setReadOnly(true);
    m_pDatabaseDirectoryTextField = make_parented<QLineEdit>();
    m_pDatabaseDirectoryTextField->setReadOnly(true);
    m_pMusicDirectoryTextField = make_parented<QLineEdit>();
    m_pMusicDirectoryTextField->setReadOnly(true);

    // Radio buttons to allow choice between exporting the whole music library
    // or just tracks in a selection of crates.
    auto pWholeLibraryRadio = make_parented<QRadioButton>(tr("Entire music library"));
    pWholeLibraryRadio->setChecked(true);
    this->exportWholeLibrarySelected();
    connect(pWholeLibraryRadio,
            &QRadioButton::clicked,
            this,
            &DlgLibraryExport::exportWholeLibrarySelected);
    auto pCratesRadio = make_parented<QRadioButton>(tr("Selected crates"));
    connect(pCratesRadio,
            &QRadioButton::clicked,
            this,
            &DlgLibraryExport::exportSelectedCratedSelected);

    // Note about need to analyse tracks before export.
    auto pTrackAnalysisNoteField = make_parented<QLabel>(
            tr("Note: all affected music files will be scheduled for "
               "analysis before they can be exported.  This can take some "
               "time if there are many tracks requiring analysis in the music "
               "library or selected crates."));
    pTrackAnalysisNoteField->setWordWrap(true);

    // Button to allow ability to browse for the export directory.
    auto pExportDirBrowseButton = make_parented<QPushButton>(tr("Browse"));
    connect(pExportDirBrowseButton,
            &QPushButton::clicked,
            this,
            &DlgLibraryExport::browseExportDirectory);
    auto pExportDirLayout = make_parented<QHBoxLayout>();
    pExportDirLayout->addWidget(m_pBaseDirectoryTextField);
    pExportDirLayout->addWidget(pExportDirBrowseButton);

    auto pFormLayout = make_parented<QFormLayout>();
    pFormLayout->addRow(tr("Base export directory"), pExportDirLayout);
    pFormLayout->addRow(tr("Engine Library export directory"), m_pDatabaseDirectoryTextField);
    pFormLayout->addRow(tr("Copy music files to"), m_pMusicDirectoryTextField);
    pFormLayout->addRow(pTrackAnalysisNoteField);

    // Buttons to begin the export or cancel.
    auto pExportButton = make_parented<QPushButton>(tr("Export"));
    pExportButton->setDefault(true);
    connect(pExportButton, &QPushButton::clicked, this, &DlgLibraryExport::exportRequested);
    auto pCancelButton = make_parented<QPushButton>(tr("Cancel"));
    connect(pCancelButton, &QPushButton::clicked, this, &QDialog::reject);

    // Arrange action buttons at bottom of dialog.
    auto pButtonBarLayout = make_parented<QHBoxLayout>();
    pButtonBarLayout->addStretch(1);
    pButtonBarLayout->addWidget(pWholeLibraryRadio); // TODO (mrsmidge) - move these to top
    pButtonBarLayout->addWidget(pCratesRadio);
    pButtonBarLayout->addWidget(pExportButton);
    pButtonBarLayout->addWidget(pCancelButton);

    auto pLayout = make_parented<QGridLayout>();
    pLayout->addLayout(pFormLayout, 0, 0);
    pLayout->addLayout(pButtonBarLayout, 1, 0);
    pLayout->addWidget(m_pCratesList, 0, 1, 2, 1);

    setLayout(pLayout);
    setWindowTitle(tr("Export Library"));

    show();
    raise();
    activateWindow();
}

void DlgLibraryExport::exportWholeLibrarySelected() {
    // Disallow selection of individual crates.
    m_pCratesList->setEnabled(false);
}

void DlgLibraryExport::exportSelectedCratedSelected() {
    // Allow selection of individual crates
    m_pCratesList->setEnabled(true);
}

void DlgLibraryExport::browseExportDirectory() {
    QString lastExportDirectory =
            m_pConfig->getValue(ConfigKey("[Library]", "LastLibraryExportDirectory"),
                    QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    auto baseDirectory =
            QFileDialog::getExistingDirectory(NULL, tr("Export Library To"), lastExportDirectory);
    if (baseDirectory.isEmpty()) {
        return;
    }
    m_pConfig->set(
            ConfigKey("[Library]", "LastLibraryExportDirectory"), ConfigValue(baseDirectory));

    // TODO (mrsmidge) - ask libdjinterop for default EL directory
    QDir baseExportDirectory{baseDirectory};
    auto databaseDirectory = baseExportDirectory.filePath("Engine Library");
    auto musicDirectory = baseExportDirectory.filePath(DefaultMixxxExportDirName);

    m_pBaseDirectoryTextField->setText(baseDirectory);
    m_pDatabaseDirectoryTextField->setText(databaseDirectory);
    m_pMusicDirectoryTextField->setText(musicDirectory);
}

void DlgLibraryExport::exportRequested() {
    // Check a base export directory has been chosen
    if (m_pBaseDirectoryTextField->text() == "") {
        QMessageBox::information(this,
                tr("No Export Directory Chosen"),
                tr("No export directory was chosen. Please choose a directory "
                    "in order to export the music library."),
                QMessageBox::Ok,
                QMessageBox::Ok);
        return;
    }

    // TODO (mrsmidge) - see if an EL DB exists in the chosen dir already, and
    // ask the user for confirmation before proceeding if so.

    // Construct a request to export the library/crates.
    // Assumed to always be an Engine Library export in this iteration of the
    // dialog.
    EngineLibraryExportRequest request;
    request.engineLibraryDbDir = QDir{m_pDatabaseDirectoryTextField->text()};
    request.musicFilesDir = QDir{m_pMusicDirectoryTextField->text()};
    request.exportSelectedCrates = m_pCratesList->isEnabled();
    if (request.exportSelectedCrates) {
        for (auto* pItem : m_pCratesList->selectedItems()) {
            CrateId id{pItem->data(Qt::UserRole).value<int>()};
            request.crateIdsToExport.insert(id);
        }
    }

    emit startEngineLibraryExport(std::move(request));
    accept();
}

} // namespace mixxx
