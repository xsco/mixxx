#include "library/export/dlglibraryexport.h"

#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QStandardPaths>

#include "library/crate/crateid.h"
#include "library/trackcollection.h"

namespace el = djinterop::enginelibrary;

namespace mixxx {

DlgLibraryExport::DlgLibraryExport(
        QWidget* parent, UserSettingsPointer pConfig, TrackCollection& trackCollection)
        : QDialog(parent), m_pConfig{pConfig}, m_trackCollection{trackCollection} {
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

    m_pExternalCratesTree = make_parented<QTreeWidget>();
    m_pBaseDirectoryTextField = make_parented<QLineEdit>();
    m_pBaseDirectoryTextField->setReadOnly(true);
    m_pDatabaseDirectoryTextField = make_parented<QLineEdit>();
    m_pDatabaseDirectoryTextField->setReadOnly(true);
    m_pMusicDirectoryTextField = make_parented<QLineEdit>();
    m_pMusicDirectoryTextField->setReadOnly(true);

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
    pExportDirLayout->addWidget(m_pBaseDirectoryTextField);
    pExportDirLayout->addWidget(pExportDirBrowseButton);

    auto pFormLayout = make_parented<QFormLayout>();
    pFormLayout->addRow(tr("Base export directory"), pExportDirLayout);
    pFormLayout->addRow(tr("Engine Library export directory"), m_pDatabaseDirectoryTextField);
    pFormLayout->addRow(tr("Copy music files to"), m_pMusicDirectoryTextField);
    pFormLayout->addRow(pTrackAnalysisNoteField);

    auto pExportButton = make_parented<QPushButton>(tr("Export"));
    pExportButton->setDefault(true);
    connect(pExportButton, &QPushButton::clicked, this, &DlgLibraryExport::exportRequested);

    auto pCancelButton = make_parented<QPushButton>(tr("Cancel"));
    connect(pCancelButton, &QPushButton::clicked, this, &QDialog::reject);

    auto pButtonBarLayout = make_parented<QHBoxLayout>();
    pButtonBarLayout->addStretch(1);
    pButtonBarLayout->addWidget(pWholeLibraryRadio);
    pButtonBarLayout->addWidget(pCratesRadio);
    pButtonBarLayout->addWidget(pExportButton);
    pButtonBarLayout->addWidget(pCancelButton);

    auto pLayout = make_parented<QGridLayout>();
    pLayout->addLayout(pFormLayout, 0, 0);
    pLayout->addLayout(pButtonBarLayout, 1, 0);
    pLayout->addWidget(m_pCratesList, 0, 1, 2, 1);
    pLayout->addWidget(m_pExternalCratesTree, 0, 2, 2, 1);

    setLayout(pLayout);
    setWindowTitle(tr("Export Library"));

    show();
    raise();
    activateWindow();
}

void DlgLibraryExport::exportWholeLibrarySelected() {
    m_pCratesList->selectAll();
    m_pCratesList->setEnabled(false);
    // TODO (haslersn): Does it work?
    // TODO (haslersn): Simple "Select all" button instead
}

void DlgLibraryExport::exportSelectedCratedSelected() {
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

    QDir baseExportDirectory{baseDirectory};
    auto databaseDirectory = baseExportDirectory.filePath(
            QString::fromStdString(DjinteropExportModel::EngineLibraryFolder));
    auto musicDirectory = baseExportDirectory.filePath(
            QString::fromStdString(DjinteropExportModel::CanonicalMusicFolder));

    m_database = el::make_database(databaseDirectory.toStdString());

    m_pBaseDirectoryTextField->setText(baseDirectory);
    m_pDatabaseDirectoryTextField->setText(databaseDirectory);
    m_pMusicDirectoryTextField->setText(musicDirectory);

    updateExternalCratesList();
}

void DlgLibraryExport::updateExternalCratesList() {
    // TODO (haslersn): Call this whenever appropriate

    m_pExternalCratesTree->clear();

    if (!m_database) {
        return;
    }

    std::unordered_map<int, QTreeWidgetItem*> items;
    std::vector<std::pair<int, std::unique_ptr<QTreeWidgetItem>>> parentIdsAndChildren;

    for (auto crate : m_database->crates()) {
        auto pItem = std::make_unique<QTreeWidgetItem>();
        items[crate.id()] = pItem.get();
        pItem->setText(0, QString::fromStdString(crate.name()));
        if (crate.parent()) {
            parentIdsAndChildren.emplace_back(crate.parent()->id(), std::move(pItem));
        } else {
            m_pExternalCratesTree->addTopLevelItem(pItem.release());
        }
    }

    for (auto&& edge : parentIdsAndChildren) {
        auto it = items.find(edge.first);
        if (it == items.end()) {
            qWarning() << "Crate had an invalid parent ID.";
            continue;
        }
        it->second->addChild(to_parented(edge.second));
    }
}

namespace {

djinterop::crate createOrLoadCrate(const djinterop::database& database, const std::string& name) {
    auto crateCandidates = database.crates_by_name(name);
    crateCandidates.erase(
            std::remove_if(crateCandidates.begin(),
                    crateCandidates.end(),
                    [](const auto& crate) {
                        return static_cast<bool>(
                                crate.parent()); // crates that have a parent are no candidate
                    }),
            crateCandidates.end());
    switch (crateCandidates.size()) {
    case 0:
        return database.create_crate(name);
    case 1:
        return crateCandidates.front();
    default:
        // TODO (haslersn): Throw something
        return crateCandidates.front();
    }
}

} // namespace

void DlgLibraryExport::exportRequested() {
    // Check a base export directory has been chosen
    if (!m_database) {
        QMessageBox::information(this,
                tr("No Export Directory Chosen"),
                tr("No export directory was chosen. Please choose a directory in order to export "
                   "the music library."),
                QMessageBox::Ok,
                QMessageBox::Ok);
        return;
    }

    DjinteropExportModel model;
    model.musicDirectory = m_pMusicDirectoryTextField->text().toStdString();

    for (auto* pItem : m_pCratesList->selectedItems()) {
        CrateId id{pItem->data(Qt::UserRole).value<int>()};
        Crate crate;
        m_trackCollection.crates().readCrateById(id, &crate);
        auto name = crate.getName().toStdString();
        auto externalCrate = createOrLoadCrate(*m_database, name);
        model.crateMappings.emplace_back(m_trackCollection, id, externalCrate);
    }

    emit startExport(std::move(model));
    accept();
}

} // namespace mixxx
