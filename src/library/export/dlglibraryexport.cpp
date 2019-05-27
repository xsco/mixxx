#include "library/export/dlglibraryexport.h"
#include "library/trackcollection.h"

const auto engineLibraryDirName = "Engine Library";
const auto mixxxExportDirName = "MixxxExport";

DlgLibraryExport::DlgLibraryExport(
        QWidget *parent,
        UserSettingsPointer pConfig,
        TrackCollection *pTrackCollection,
        LibraryExportModel &model) :
    QDialog(parent),
    m_pConfig{pConfig},
    m_pTrackCollection{pTrackCollection},
    m_model{model}
{
    this->wholeLibraryRadio_ = new QRadioButton(tr("Entire music library"));
    this->cratesRadio_ = new QRadioButton(tr("Selected crates"));
    this->cratesList_ = new QListWidget();
    this->cratesList_->setSelectionMode(QListWidget::ExtendedSelection);

    this->connect(
            this->wholeLibraryRadio_, &QRadioButton::clicked,
            this, &DlgLibraryExport::exportWholeLibrarySelected);
    this->connect(
            this->cratesRadio_, &QRadioButton::clicked,
            this, &DlgLibraryExport::exportSelectedCratedSelected);

    this->connect(
            this->cratesList_, &QListWidget::itemSelectionChanged,
            this, &DlgLibraryExport::crateSelectionChanged);

    auto *formLayout = new QFormLayout();

    this->exportDirTextField_ = new QLineEdit();
    this->exportDirTextField_->setReadOnly(true);
    this->engineLibraryDirTextField_ = new QLineEdit();
    this->engineLibraryDirTextField_->setReadOnly(true);
    this->musicFilesDirTextField_ = new QLineEdit();
    this->musicFilesDirTextField_->setReadOnly(true);
    auto *exportDirBrowseButton = new QPushButton(tr("Browse"));
    auto *exportDirLayout = new QHBoxLayout();
    exportDirLayout->addWidget(this->exportDirTextField_);
    exportDirLayout->addWidget(exportDirBrowseButton);
    this->connect(
            exportDirBrowseButton, &QPushButton::clicked,
            this, &DlgLibraryExport::browseExportDirectory);
    formLayout->addRow(tr("Base export directory"), exportDirLayout);
    formLayout->addRow(tr("Engine Library export directory"), this->engineLibraryDirTextField_);
    formLayout->addRow(tr("Copy music files to"), this->musicFilesDirTextField_);

    auto *buttonBarLayout = new QHBoxLayout();
    auto *exportButton = new QPushButton(tr("Export"));
    exportButton->setDefault(true);
    auto *cancelButton = new QPushButton(tr("Cancel"));
    this->connect(exportButton, &QPushButton::clicked, this, &DlgLibraryExport::exportRequested);
    this->connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonBarLayout->addStretch(1);
    buttonBarLayout->addWidget(exportButton);
    buttonBarLayout->addWidget(cancelButton);

    auto *layout = new QGridLayout();
    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 2);
    layout->addWidget(this->wholeLibraryRadio_, 0, 0);
    layout->addWidget(this->cratesRadio_, 1, 0);
    layout->addWidget(this->cratesList_, 2, 0);
    layout->addLayout(formLayout, 0, 1, 3, 1);
    layout->addLayout(buttonBarLayout, 3, 0, 1, 2);

    this->setLayout(layout);
    this->setWindowTitle(tr("Export Library"));
    reset();
}

void DlgLibraryExport::reset() {
    // Reset the model
    m_model.clear();
    
    this->wholeLibraryRadio_->setChecked(true);
    this->exportWholeLibrarySelected();

    // TODO - get from track collection
    QStringList crates = { "Crate 1", "Crate 2", "Crate 3", "Crate 4", "Crate 5" };
    this->cratesList_->clear();
    this->cratesList_->insertItems(0, crates);

    this->exportDirTextField_->clear();
    this->engineLibraryDirTextField_->clear();
    this->musicFilesDirTextField_->clear();
}

void DlgLibraryExport::exportWholeLibrarySelected()
{
    this->cratesList_->setEnabled(false);
    m_model.exportEntireMusicLibrary = true;
}

void DlgLibraryExport::exportSelectedCratedSelected()
{
    this->cratesList_->setEnabled(true);
    m_model.exportEntireMusicLibrary = false;
}

void DlgLibraryExport::crateSelectionChanged()
{
    m_model.selectedCrates.clear();
    for (auto *item : this->cratesList_->selectedItems())
    {
        m_model.selectedCrates.insert(item->text());
    }
}

void DlgLibraryExport::browseExportDirectory()
{
    QString lastExportDirectory = m_pConfig->getValue(
            ConfigKey("[Library]", "LastLibraryExportDirectory"),
            QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation));
    auto baseExportDirectoryStr = QFileDialog::getExistingDirectory(
            NULL, tr("Export Library To"), lastExportDirectory);
    if (baseExportDirectoryStr.isEmpty()) {
        return;
    }
    m_pConfig->set(ConfigKey("[Library]", "LastLibraryExportDirectory"),
                   ConfigValue(baseExportDirectoryStr));

    QDir baseExportDirectory{baseExportDirectoryStr};
    m_model.engineLibraryDir = baseExportDirectory.filePath(engineLibraryDirName);
    m_model.musicFilesDir = baseExportDirectory.filePath(mixxxExportDirName);

    this->exportDirTextField_->setText(baseExportDirectoryStr);
    this->engineLibraryDirTextField_->setText(m_model.engineLibraryDir);
    this->musicFilesDirTextField_->setText(m_model.musicFilesDir);
}

void DlgLibraryExport::exportRequested()
{
    // Check a base export directory has been chosen
    if (this->exportDirTextField_->text().isEmpty())
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

