#include "changespage.h"

#include <QClipboard>
#include <QDesktopServices>
#include <QFileDialog>
#include <QMenu>
#include <QProgressBar>
#include <QShortcut>
#include <QtConcurrent>

#include "dialogs/cmddialog.h"
#include "dialogs/warningfilelistdialog.h"
#include "ui_changespage.h"

ChangesPage::ChangesPage(QWidget *parent, const Project &project)
    : QWidget(parent), ui(new Ui::ChangesPage), m_project(project)
{
    ui->setupUi(this);
    ui->bottomSplitter->setSizes(QList<int>({1000, 100}));
    ui->centerSplitter->setSizes(QList<int>({100, 300}));
    m_indicator = new QProgressIndicator(this);

    ui->unstagedTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    connect(ui->unstagedTable, &QTableWidget::cellDoubleClicked, this,
        &ChangesPage::onFileDoubleClicked);
    connect(
        ui->unstagedTable, &QTableWidget::currentItemChanged, this, &ChangesPage::onFileSelected);
    connect(ui->unstagedTable, &QTableWidget::customContextMenuRequested, this,
        &ChangesPage::onFileListMenuRequested);
    connect(ui->unstageBtn, &QPushButton::clicked, this, &ChangesPage::onTableButtonClicked);

    ui->stagedTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    connect(
        ui->stagedTable, &QTableWidget::cellDoubleClicked, this, &ChangesPage::onFileDoubleClicked);
    connect(ui->stagedTable, &QTableWidget::currentItemChanged, this, &ChangesPage::onFileSelected);
    connect(ui->stagedTable, &QTableWidget::customContextMenuRequested, this,
        &ChangesPage::onFileListMenuRequested);
    connect(ui->stageBtn, &QPushButton::clicked, this, &ChangesPage::onTableButtonClicked);

    connect(ui->diffView, &DiffView::diffParametersChanged, this, &ChangesPage::getDiffAsync);

    connect(ui->amendCheckBox, &QCheckBox::toggled, this, &ChangesPage::onAmendToggled);
    connect(ui->commitButton, &QPushButton::clicked, this, &ChangesPage::onCommit);

    getChangesAsync();
}

ChangesPage::~ChangesPage()
{
    reset(All);
    delete ui;
}

void ChangesPage::showEvent(QShowEvent *event)
{
    refresh();
}

void ChangesPage::refresh()
{
    reset(List | Diff);
    getChangesAsync();
}

void ChangesPage::updateUI(unsigned flags)
{
    if (flags & List) {
        const int stagedSize = m_stagedList.size();
        const int unstagedSize = m_unstagedList.size();

        for (int i = 0; i < unstagedSize; ++i) {
            GitFile f = m_unstagedList.at(i);
            ui->unstagedTable->insertRow(i);
            QTableWidgetItem *modeItem = new QTableWidgetItem(f.mode);
            modeItem->setToolTip(f.path);
            ui->unstagedTable->setItem(i, 0, modeItem);
            QTableWidgetItem *fileItem = new QTableWidgetItem(f.path);
            fileItem->setToolTip(f.path);
            ui->unstagedTable->setItem(i, 1, fileItem);
        }
        for (int i = 0; i < stagedSize; ++i) {
            GitFile f = m_stagedList.at(i);
            ui->stagedTable->insertRow(i);
            QTableWidgetItem *modeItem = new QTableWidgetItem(f.mode);
            modeItem->setToolTip(f.path);
            ui->stagedTable->setItem(i, 0, modeItem);
            QTableWidgetItem *fileItem = new QTableWidgetItem(f.path);
            fileItem->setToolTip(f.path);
            ui->stagedTable->setItem(i, 1, fileItem);
        }

        m_stagedSelection = qMin(stagedSize - 1, m_stagedSelection);
        m_unstagedSelection = qMin(unstagedSize - 1, m_unstagedSelection);
        if (m_stagedSelection >= 0) {
            ui->stagedTable->selectRow(m_stagedSelection);
        } else if (m_unstagedSelection >= 0) {
            ui->unstagedTable->selectRow(m_unstagedSelection);
        } else if (ui->stagedTable->rowCount() > 0) {
            ui->stagedTable->selectRow(0);
        } else if (ui->unstagedTable->rowCount() > 0) {
            ui->unstagedTable->selectRow(0);
        }
    }
    if (flags & Diff) {
        ui->diffView->setDiffHunks(m_file, m_diffHunks);
    }
}

void ChangesPage::reset(unsigned flags)
{
    if (flags & List) {
        m_changesWorker.cancel();
        m_unstagedList.clear();
        m_stagedList.clear();
        ui->unstagedTable->setRowCount(0);
        ui->stagedTable->setRowCount(0);
    }
    if (flags & Diff) {
        m_diffWorker.cancel();
        m_diffHunks.clear();
        ui->diffView->reset();
    }
    if (flags & Commit) {
        m_amendWorker.cancel();
        ui->amendCheckBox->setChecked(false);
        ui->commitTextEdit->clear();
    }
}

static bool isStagedFile(const QString &mode)
{
    // Untracked
    if (mode == "??") {
        return false;
    }
    // Unmerged
    if (mode == "AA" || mode == "DD" || mode.contains("U")) {
        return false;
    }
    if (mode[0] != ' ') {
        return true;
    }
    if (mode[1] != ' ') {
        return false;
    }
    Q_UNREACHABLE();
}

void ChangesPage::getChangesAsync()
{
    QString projectPath = m_project.absPath;
    if (projectPath.isEmpty()) {
        return;
    }
    m_indicator->startHint();
    m_changesWorker = QtConcurrent::run([projectPath](QPromise<ChangesResult> &promise) {
        QString cmdResult =
            global::getCmdResult("git status --porcelain --untracked-files=all", projectPath);
        if (promise.isCanceled()) {
            return;
        }
        QStringList lines = cmdResult.split('\n');
        ChangesResult result;
        for (int i = 0; i < lines.size(); ++i) {
            QString line = lines[i];
            if (line.isEmpty()) {
                continue;
            }
            result.fileCount++;

            QString mode = line.sliced(0, 2);
            QString path = line.sliced(3);

            if (path.contains(" -> ")) {
                path = path.split(" -> ").last();
            }
            if (isStagedFile(mode)) {
                result.stagedList.emplace_back(path, mode);
            } else {
                result.unstagedList.emplace_back(path, mode);
            }
        }
        promise.addResult(result);
    });
    QPointer thisPtr(this);
    m_changesWorker
        .then(qApp,
            [this](const ChangesResult &result) {
                this->m_indicator->stopHint();
                this->m_unstagedList = result.unstagedList;
                this->m_stagedList = result.stagedList;
                updateUI(List);
                emit newChangesEvent(result.fileCount);
            })
        .onCanceled(qApp, [thisPtr] {
            if (thisPtr.isNull()) return;
            thisPtr->m_indicator->stopHint();
        });
}

void ChangesPage::getDiffAsync()
{
    QString cmd;
    if (m_file.mode == "??") {
        cmd = QString(R"(git diff -U%1 -- /dev/null "%2")")
                  .arg(QString::number(ui->diffView->getContextLines()), m_file.path);
    } else if (isStagedFile(m_file.mode)) {
        cmd = QString(R"(git diff -U%1 -M --cached -- "%2")")
                  .arg(QString::number(ui->diffView->getContextLines()), m_file.path);
    } else {
        cmd = QString(R"(git diff -U%1 -M -- "%2")")
                  .arg(QString::number(ui->diffView->getContextLines()), m_file.path);
    }
    QString projectPath = m_project.absPath;
    m_indicator->startHint();
    m_diffWorker = QtConcurrent::run([projectPath, cmd](QPromise<DiffResult> &promise) {
        QString cmdResult = global::getCmdResult(cmd, projectPath);
        if (promise.isCanceled()) {
            return;
        }

        DiffResult result;
        result.hunks = parseDiffHunks(cmdResult);
        promise.addResult(result);
    });
    QPointer thisPtr(this);
    m_diffWorker
        .then(qApp,
            [this](const DiffResult &result) {
                this->m_indicator->stopHint();
                this->m_diffHunks = result.hunks;
                updateUI(Diff);
            })
        .onCanceled(qApp, [thisPtr] {
            if (thisPtr.isNull()) return;
            thisPtr->m_indicator->stopHint();
        });
}

void ChangesPage::onFileListMenuRequested(const QPoint &pos)
{
    auto sourceTable = qobject_cast<QTableWidget *>(sender());
    const QModelIndex &index = sourceTable->indexAt(pos);
    if (!index.isValid()) return;
    QList<GitFile> &fileList = sourceTable == ui->stagedTable ? m_stagedList : m_unstagedList;
    const GitFile &file = fileList.at(index.row());
    const QString &absPath = QDir::cleanPath(m_project.absPath + "/" + file.path);
    const QModelIndexList &selectedRows = sourceTable->selectionModel()->selectedRows();
    QStringList selectedFiles;
    for (const QModelIndex &index : selectedRows) {
        selectedFiles << fileList.at(index.row()).path;
    }

    QMenu menu;
    menu.addAction("Open", this,
            [&]() {
                QDesktopServices::openUrl(QUrl::fromLocalFile(absPath));
            })
        ->setEnabled(selectedRows.size() == 1);
    menu.addAction("Open Containing Folder", this, [&]() {
        QDir dir = QFileInfo(absPath).absoluteDir();
        QDesktopServices::openUrl(QUrl::fromLocalFile(dir.path()));
    });
    menu.addAction("Copy Absolute Path", this,
            [&]() {
                QGuiApplication::clipboard()->setText(absPath);
            })
        ->setEnabled(selectedRows.size() == 1);
    menu.addSeparator();
    menu.addAction("Discard", this, [&]() {
        if (WarningFileListDialog::confirm(this, "Confirm Discard",
                "Changes in the following files will be discarded, THIS CANNOT BE UNDONE",
                selectedFiles)) {
            if (sourceTable == ui->unstagedTable) {
                batchFilesAction(ui->unstagedTable, {"git checkout --"});
            } else {
                batchFilesAction(ui->stagedTable, {"git reset --", "git checkout --"});
            }
        }
    });

    bool enableRemove = true;
    for (auto &index : selectedRows) {
        if (fileList.at(index.row()).mode.contains("D")) {
            enableRemove = false;
            break;
        }
    }
    menu.addAction("Remove", this,
            [&]() {
                if (WarningFileListDialog::confirm(this, "Confirm Remove",
                        "The following files will be removed, THIS CANNOT BE UNDONE if file is not "
                        "tracked by git",
                        selectedFiles)) {
                    if (sourceTable == ui->unstagedTable) {
                        batchFilesAction(ui->unstagedTable, {"rm"});
                    } else {
                        batchFilesAction(ui->stagedTable, {"git reset --", "rm"});
                    }
                }
            })
        ->setEnabled(enableRemove);
    menu.addSeparator();
    menu.addAction(sourceTable == ui->stagedTable ? "Unstage" : "Stage", this, [&]() {
        if (sourceTable == ui->unstagedTable) {
            batchFilesAction(ui->unstagedTable, {"git add --"});
        } else {
            batchFilesAction(ui->stagedTable, {"git reset --"});
        }
    });
    menu.addAction(sourceTable == ui->stagedTable ? "Unstage All" : "Stage All", this, [&]() {
        QString cmd = sourceTable == ui->stagedTable ? "git reset" : "git add .";
        CmdDialog::execute(this, cmd, m_project.absPath, true);
        reset(List | Diff);
        getChangesAsync();
    });
    menu.exec(sourceTable->mapToGlobal(pos));
}

void ChangesPage::onFileDoubleClicked(int row, int column)
{
    if (sender() == ui->unstagedTable) {
        batchFilesAction(ui->unstagedTable, {"git add --"});
    } else {
        batchFilesAction(ui->stagedTable, {"git reset --"});
    }
}

void ChangesPage::onTableButtonClicked()
{
    if (sender() == ui->stageBtn) {
        batchFilesAction(ui->unstagedTable, {"git add --"});
    } else {
        batchFilesAction(ui->stagedTable, {"git reset --"});
    }
}

void ChangesPage::batchFilesAction(QTableWidget *table, const QStringList &cmds)
{
    const QModelIndexList &indexes = table->selectionModel()->selectedRows();
    if (indexes.isEmpty()) return;
    const QList<GitFile> &fileList = table == ui->stagedTable ? m_stagedList : m_unstagedList;

    QStringList fullCmds;
    for (auto &c : cmds) {
        QString fullCmd = c;
        for (const QModelIndex &index : indexes) {
            fullCmd.append(" ").append(fileList.at(index.row()).path);
        }
        fullCmds << fullCmd;
    }

    CmdDialog::execute(this, fullCmds, m_project.absPath, true);
    reset(List | Diff);
    getChangesAsync();
}

void ChangesPage::onFileSelected(QTableWidgetItem *current, QTableWidgetItem *previous)
{
    if (!current) return;

    GitFile file;
    if (sender() == ui->stagedTable) {
        file = m_stagedList.at(current->row());
        ui->unstagedTable->setCurrentItem(nullptr);
        m_stagedSelection = current->row();
        m_unstagedSelection = -1;
    } else {
        file = m_unstagedList.at(current->row());
        ui->stagedTable->setCurrentItem(nullptr);
        m_stagedSelection = -1;
        m_unstagedSelection = current->row();
    }
    m_file = file;
    getDiffAsync();
}

void ChangesPage::onAmendToggled(bool checked)
{
    if (checked) {
        const QString &projectPath = m_project.absPath;
        m_indicator->startHint();
        m_amendWorker.cancel();
        m_amendWorker = QtConcurrent::run([projectPath]() {
            return global::getCmdResult("git log -n1 --format=%B", projectPath);
        });
        QPointer thisPtr(this);
        m_amendWorker
            .then(qApp,
                [&](QString body) {
                    m_indicator->stopHint();
                    ui->commitTextEdit->clear();
                    ui->commitTextEdit->insertPlainText(body.trimmed());
                })
            .onCanceled(qApp, [thisPtr] {
                if (thisPtr.isNull()) return;
                thisPtr->m_indicator->stopHint();
            });
    }
}

void ChangesPage::onCommit()
{
    QString msg = ui->commitTextEdit->toPlainText();
    if (msg.isEmpty()) {
        return;
    }
    QString cmd = QString("git commit%1 -m \"%2\"")
                      .arg(ui->amendCheckBox->isChecked() ? " --amend --no-edit" : "", msg);
    if (CmdDialog::execute(this, cmd, m_project.absPath, true) == 0) {
        reset(All);
        emit commitEvent(HistorySelectionArg(HistorySelectionArg::First));
    }
}
