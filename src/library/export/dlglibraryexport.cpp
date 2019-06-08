#include "library/export/dlglibraryexport.h"

#include <QStandardPaths>

#include "library/trackcollection.h"

DlgLibraryExport::DlgLibraryExport(QWidget *parent,
        UserSettingsPointer pConfig,
        TrackCollection &trackCollection,
        LibraryExportModel &model)
        : QDialog(parent), m_pConfig{pConfig}, m_trackCollection{trackCollection}, m_model{model} {
    m_pWholeLibraryRadio_ = make_parented<QRadioButton>(tr("Entire music library"));
    connect(m_pWholeLibraryRadio_,
            &QRadioButton::clicked,
            this,
            &DlgLibraryExport::exportWholeLibrarySelected);

    m_pCratesRadio = make_parented<QRadioButton>(tr("Selected crates"));
    connect(m_pCratesRadio,
            &QRadioButton::clicked,
            this,
            &DlgLibraryExport::exportSelectedCratedSelected);

    m_pCratesList = make_parented<QListWidget>();
    m_pCratesList->setSelectionMode(QListWidget::ExtendedSelection);
    connect(m_pCratesList,
            &QListWidget::itemSelectionChanged,
            this,
            &DlgLibraryExport::crateSelectionChanged);

    m_pExportDirTextField = make_parented<QLineEdit>();
    m_pExportDirTextField->setReadOnly(true);
    m_pEngineLibraryDirTextField = make_parented<QLineEdit>();
    m_pEngineLibraryDirTextField->setReadOnly(true);
    m_pMusicFilesDirTextField = make_parented<QLineEdit>();
    m_pMusicFilesDirTextField->setReadOnly(true);

    auto pTrackAnalysisNoteField = make_parented<QLabel>(
            tr("Note: all affected music files will be scheduled for "
               "analysis before they can be exported.  This can take some "
               "time if there are many tracks requiring analysis in the music "
               "library or selected crates."));
    pTrackAnalysisNoteField->setWordWrap(true);

    auto pExportDirBrowseButton = make_parented<QPushButton>(tr("Browse"));
    connect(pExportDirBrowseButton,
            &QPushButton::clicked,
            this,
            &DlgLibraryExport::browseExportDirectory);

    auto pExportDirLayout = make_parented<QHBoxLayout>();
    pExportDirLayout->addWidget(m_pExportDirTextField);
    pExportDirLayout->addWidget(pExportDirBrowseButton);

    auto pFormLayout = make_parented<QFormLayout>();
    pFormLayout->addRow(tr("Base export directory"), pExportDirLayout);
    pFormLayout->addRow(tr("Engine Library export directory"), m_pEngineLibraryDirTextField);
    pFormLayout->addRow(tr("Copy music files to"), m_pMusicFilesDirTextField);
    pFormLayout->addRow(pTrackAnalysisNoteField);

    auto pExportButton = make_parented<QPushButton>(tr("Export"));
    pExportButton->setDefault(true);
    connect(pExportButton, &QPushButton::clicked, this, &DlgLibraryExport::exportRequested);

    auto pCancelButton = make_parented<QPushButton>(tr("Cancel"));
    connect(pCancelButton, &QPushButton::clicked, this, &QDialog::reject);

    auto pButtonBarLayout = make_parented<QHBoxLayout>();
    pButtonBarLayout->addStretch(1);
    pButtonBarLayout->addWidget(pExportButton);
    pButtonBarLayout->addWidget(pCancelButton);

    auto pLayout = make_parented<QGridLayout>();
    pLayout->setColumnStretch(0, 1);
    pLayout->setColumnStretch(1, 2);
    pLayout->addWidget(m_pWholeLibraryRadio_, 0, 0);
    pLayout->addWidget(m_pCratesRadio, 1, 0);
    pLayout->addWidget(m_pCratesList, 2, 0);
    pLayout->addLayout(pFormLayout, 0, 1, 3, 1);
    pLayout->addLayout(pButtonBarLayout, 3, 0, 1, 2);

    setLayout(pLayout);
    setWindowTitle(tr("Export Library"));
    reset();
}

void DlgLibraryExport::reset() {
    // Reset the model
    m_model.clear();

    m_pWholeLibraryRadio_->setChecked(true);
    exportWholeLibrarySelected();

    // Populate list of crates.
    auto crates = m_trackCollection.crates().selectCrates();
    m_pCratesList->clear();
    Crate crate;
    while (crates.populateNext(&crate)) {
        auto pItem = std::make_unique<QListWidgetItem>(crate.getName());
        QVariant variant;
        variant.setValue(crate.getId().value());
        pItem->setData(Qt::UserRole, variant);
        m_pCratesList->addItem(pItem.release());
    }

    m_pExportDirTextField->clear();
    m_pEngineLibraryDirTextField->clear();
    m_pMusicFilesDirTextField->clear();
}

void DlgLibraryExport::exportWholeLibrarySelected() {
    m_pCratesList->setEnabled(false);
    m_model.exportEntireMusicLibrary = true;
}

void DlgLibraryExport::exportSelectedCratedSelected() {
    m_pCratesList->setEnabled(true);
    m_model.exportEntireMusicLibrary = false;
}

void DlgLibraryExport::crateSelectionChanged() {
    m_model.selectedCrates.clear();
    for (auto *item : m_pCratesList->selectedItems()) {
        QVariant variant = item->data(Qt::UserRole);
        CrateId id{variant.value<int>()};
        m_model.selectedCrates.append(id);
    }
}

void DlgLibraryExport::browseExportDirectory() {
    QString lastExportDirectory =
            m_pConfig->getValue(ConfigKey("[Library]", "LastLibraryExportDirectory"),
                    QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    auto baseExportDirectoryStr =
            QFileDialog::getExistingDirectory(NULL, tr("Export Library To"), lastExportDirectory);
    if (baseExportDirectoryStr.isEmpty()) {
        return;
    }
    m_pConfig->set(ConfigKey("[Library]", "LastLibraryExportDirectory"),
            ConfigValue(baseExportDirectoryStr));

    QDir baseExportDirectory{baseExportDirectoryStr};
    m_model.engineLibraryDir =
            baseExportDirectory.filePath(LibraryExportModel::EngineLibraryDirName);
    m_model.musicFilesDir = baseExportDirectory.filePath(LibraryExportModel::MixxxExportDirName);

    m_pExportDirTextField->setText(baseExportDirectoryStr);
    m_pEngineLibraryDirTextField->setText(m_model.engineLibraryDir);
    m_pMusicFilesDirTextField->setText(m_model.musicFilesDir);
}

void DlgLibraryExport::exportRequested() {
    // Check a base export directory has been chosen
    if (m_pExportDirTextField->text().isEmpty()) {
        QMessageBox::information(this,
                tr("No Export Directory Chosen"),
                tr("No export directory was chosen.  Please choose a directory "
                   "in order to export the music library."),
                QMessageBox::Ok,
                QMessageBox::Ok);
        return;
    }

    accept();
}
