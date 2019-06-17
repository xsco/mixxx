#include "library/export/dlglibraryexport.h"

#include <QStandardPaths>

#include "library/trackcollection.h"

DlgLibraryExport::DlgLibraryExport(
        QWidget *parent,
        UserSettingsPointer pConfig,
        TrackCollection *pTrackCollection,
        std::shared_ptr<LibraryExportModel> pModel) :
    QDialog(parent),
    m_pConfig{pConfig},
    m_pTrackCollection{pTrackCollection},
    m_pModel{pModel}
{
    m_pWholeLibraryRadio_ = new QRadioButton(tr("Entire music library"));
    m_pCratesRadio = new QRadioButton(tr("Selected crates"));
    m_pCratesList = new QListWidget();
    m_pCratesList->setSelectionMode(QListWidget::ExtendedSelection);

    connect(
            m_pWholeLibraryRadio_, &QRadioButton::clicked,
            this, &DlgLibraryExport::exportWholeLibrarySelected);
    connect(
            m_pCratesRadio, &QRadioButton::clicked,
            this, &DlgLibraryExport::exportSelectedCratedSelected);

    connect(
            m_pCratesList, &QListWidget::itemSelectionChanged,
            this, &DlgLibraryExport::crateSelectionChanged);

    auto *formLayout = new QFormLayout();
    m_pExportDirTextField = new QLineEdit();
    m_pExportDirTextField->setReadOnly(true);
    m_pEngineLibraryDirTextField = new QLineEdit();
    m_pEngineLibraryDirTextField->setReadOnly(true);
    m_pMusicFilesDirTextField = new QLineEdit();
    m_pMusicFilesDirTextField->setReadOnly(true);
    auto *trackAnalysisNoteField = new QLabel(tr(
                "Note: all affected music files will be scheduled for "
                "analysis before they can be exported.  This can take some "
                "time if there are many tracks requiring analysis in the music "
                "library or selected crates."));
    trackAnalysisNoteField->setWordWrap(true);
    auto *exportDirBrowseButton = new QPushButton(tr("Browse"));
    auto *exportDirLayout = new QHBoxLayout();
    exportDirLayout->addWidget(m_pExportDirTextField);
    exportDirLayout->addWidget(exportDirBrowseButton);
    connect(
            exportDirBrowseButton, &QPushButton::clicked,
            this, &DlgLibraryExport::browseExportDirectory);
    formLayout->addRow(tr("Base export directory"), exportDirLayout);
    formLayout->addRow(tr("Engine Library export directory"), m_pEngineLibraryDirTextField);
    formLayout->addRow(tr("Copy music files to"), m_pMusicFilesDirTextField);
    formLayout->addRow(trackAnalysisNoteField);

    auto *buttonBarLayout = new QHBoxLayout();
    auto *exportButton = new QPushButton(tr("Export"));
    exportButton->setDefault(true);
    auto *cancelButton = new QPushButton(tr("Cancel"));
    connect(exportButton, &QPushButton::clicked, this, &DlgLibraryExport::exportRequested);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonBarLayout->addStretch(1);
    buttonBarLayout->addWidget(exportButton);
    buttonBarLayout->addWidget(cancelButton);

    auto *layout = new QGridLayout();
    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 2);
    layout->addWidget(m_pWholeLibraryRadio_, 0, 0);
    layout->addWidget(m_pCratesRadio, 1, 0);
    layout->addWidget(m_pCratesList, 2, 0);
    layout->addLayout(formLayout, 0, 1, 3, 1);
    layout->addLayout(buttonBarLayout, 3, 0, 1, 2);

    setLayout(layout);
    setWindowTitle(tr("Export Library"));
    reset();
}

void DlgLibraryExport::reset() {
    // Reset the model
    m_pModel->clear();
    
    m_pWholeLibraryRadio_->setChecked(true);
    exportWholeLibrarySelected();

    // Populate list of crates.
    auto crates = m_pTrackCollection->crates().selectCrates();
    m_pCratesList->clear();
    Crate crate;
    while (crates.populateNext(&crate))
    {
        auto *item = new QListWidgetItem{crate.getName()};
        QVariant variant;
        variant.setValue(crate.getId().value());
        item->setData(Qt::UserRole, variant);
        m_pCratesList->addItem(item);
    }

    m_pExportDirTextField->clear();
    m_pEngineLibraryDirTextField->clear();
    m_pMusicFilesDirTextField->clear();
}

void DlgLibraryExport::exportWholeLibrarySelected()
{
    m_pCratesList->setEnabled(false);
    m_pModel->exportEntireMusicLibrary = true;
}

void DlgLibraryExport::exportSelectedCratedSelected()
{
    m_pCratesList->setEnabled(true);
    m_pModel->exportEntireMusicLibrary = false;
}

void DlgLibraryExport::crateSelectionChanged()
{
    m_pModel->selectedCrates.clear();
    for (auto *item : m_pCratesList->selectedItems())
    {
        QVariant variant = item->data(Qt::UserRole);
        CrateId id{variant.value<int>()};
        m_pModel->selectedCrates.append(id);
    }
}

void DlgLibraryExport::browseExportDirectory()
{
    QString lastExportDirectory = m_pConfig->getValue(
            ConfigKey("[Library]", "LastLibraryExportDirectory"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    auto baseExportDirectoryStr = QFileDialog::getExistingDirectory(
            NULL, tr("Export Library To"), lastExportDirectory);
    if (baseExportDirectoryStr.isEmpty()) {
        return;
    }
    m_pConfig->set(ConfigKey("[Library]", "LastLibraryExportDirectory"),
                   ConfigValue(baseExportDirectoryStr));

    QDir baseExportDirectory{baseExportDirectoryStr};
    m_pModel->engineLibraryDir = baseExportDirectory.filePath(EngineLibraryDirName);
    m_pModel->musicFilesDir = baseExportDirectory.filePath(MixxxExportDirName);

    m_pExportDirTextField->setText(baseExportDirectoryStr);
    m_pEngineLibraryDirTextField->setText(m_pModel->engineLibraryDir);
    m_pMusicFilesDirTextField->setText(m_pModel->musicFilesDir);
}

void DlgLibraryExport::exportRequested()
{
    // Check a base export directory has been chosen
    if (m_pExportDirTextField->text().isEmpty())
    {
        QMessageBox::information(
                this,
                tr("No Export Directory Chosen"),
                tr("No export directory was chosen.  Please choose a directory "
                   "in order to export the music library."),
                QMessageBox::Ok,
                QMessageBox::Ok);
        return;
    }

    accept();
}

