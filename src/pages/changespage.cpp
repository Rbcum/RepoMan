#include "changespage.h"

#include <QProgressBar>
#include <QShortcut>
#include <QtConcurrent>

#include "dialogs/cmddialog.h"
#include "ui_changespage.h"

ChangesPage::ChangesPage(QWidget *parent)
    : QWidget(parent), ui(new Ui::ChangesPage), m_threadPool(new QThreadPool(this))
{
    m_threadPool->setMaxThreadCount(1);
    ui->setupUi(this);
    ui->bottomSplitter->setSizes(QList<int>({1000, 100}));
    ui->centerSplitter->setSizes(QList<int>({100, 300}));
    m_indicator = new QProgressIndicator(this);

    ui->unstagedTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    connect(ui->unstagedTable, &QTableWidget::cellDoubleClicked, this,
        &ChangesPage::onFileDoubleClicked);
    connect(
        ui->unstagedTable, &QTableWidget::itemSelectionChanged, this, &ChangesPage::onFileSelected);
    connect(ui->unstageAllBtn, &QPushButton::clicked, this, &ChangesPage::onTableButtonClicked);

    ui->stagedTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    connect(
        ui->stagedTable, &QTableWidget::cellDoubleClicked, this, &ChangesPage::onFileDoubleClicked);
    connect(
        ui->stagedTable, &QTableWidget::itemSelectionChanged, this, &ChangesPage::onFileSelected);
    connect(ui->stageAllBtn, &QPushButton::clicked, this, &ChangesPage::onTableButtonClicked);

    connect(ui->amendCheckBox, &QCheckBox::toggled, this, &ChangesPage::onAmendToggled);
    connect(ui->commitButton, &QPushButton::clicked, this, &ChangesPage::onCommit);
}

ChangesPage::~ChangesPage()
{
    delete ui;
}

void ChangesPage::setCurrentProjectPath(const QString &path)
{
    if (m_projectPath == path) {
        return;
    }
    reset(All);
    this->m_projectPath = path;
    getChangesAsync(path, m_indicator);
}

void ChangesPage::refresh()
{
    reset(List | Diff);
    getChangesAsync(m_projectPath, m_indicator);
}

void ChangesPage::updateUI(unsigned flags)
{
    if (flags & List) {
        for (int i = 0; i < m_unstagedList.size(); ++i) {
            GitFile f = m_unstagedList.at(i);
            ui->unstagedTable->insertRow(i);
            QTableWidgetItem *modeItem = new QTableWidgetItem(f.mode);
            modeItem->setToolTip(f.path);
            ui->unstagedTable->setItem(i, 0, modeItem);
            QTableWidgetItem *fileItem = new QTableWidgetItem(f.path);
            fileItem->setToolTip(f.path);
            ui->unstagedTable->setItem(i, 1, fileItem);
        }

        for (int i = 0; i < m_stagedList.size(); ++i) {
            GitFile f = m_stagedList.at(i);
            ui->stagedTable->insertRow(i);
            QTableWidgetItem *modeItem = new QTableWidgetItem(f.mode);
            modeItem->setToolTip(f.path);
            ui->stagedTable->setItem(i, 0, modeItem);
            QTableWidgetItem *fileItem = new QTableWidgetItem(f.path);
            fileItem->setToolTip(f.path);
            ui->stagedTable->setItem(i, 1, fileItem);
        }

        //    if (ui->unstagedTable->rowCount() > 0) {
        //        ui->unstagedTable->selectRow(0);
        //    } else if (ui->stagedTable->rowCount() > 0) {
        //        ui->stagedTable->selectRow(0);
        //    }
    }
    if (flags & Diff) {
        ui->diffScrollArea->setDiffHunks(m_diffHunks);
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
        ui->curFileLabel->clear();
    }
    if (flags & Diff) {
        m_diffWorker.cancel();
        m_diffHunks.clear();
        ui->diffScrollArea->reset();
    }
    if (flags & Commit) {
        m_amendWorker.cancel();
        ui->amendCheckBox->setChecked(false);
        ui->commitTextEdit->clear();
    }
}

void ChangesPage::getChangesAsync(const QString &projectPath, QProgressIndicator *const indicator)
{
    if (projectPath.isEmpty()) {
        return;
    }
    indicator->startHint();
    m_changesWorker =
        QtConcurrent::run(m_threadPool, [projectPath](QPromise<ChangesResult> &promise) {
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
                QString mode = line.sliced(0, 2);
                QString path = line.sliced(3);

                if (path.contains(" -> ")) {
                    path = path.split(" -> ").last();
                }
                if (mode.startsWith(" ") || mode == "??") {
                    result.unstagedList.emplace_back(path, mode);
                } else {
                    result.stagedList.emplace_back(path, mode);
                }
            }
            promise.addResult(result);
        });
    m_changesWorker
        .then(this,
            [this](const ChangesResult &result) {
                this->m_indicator->stopHint();
                this->m_unstagedList = result.unstagedList;
                this->m_stagedList = result.stagedList;
                updateUI(List);
                emit newChangesEvent(m_stagedList.size() + m_unstagedList.size());
            })
        .onCanceled(this, [this] {
            this->m_indicator->stopHint();
        });
}

void ChangesPage::getDiffAsync(
    const QString &projectPath, const GitFile &file, QProgressIndicator *const indicator)
{
    QString cmd;
    if (file.mode == "??") {
        cmd = QString(R"(git diff -- /dev/null "%1")").arg(file.path);
    } else if (file.mode.startsWith(" ")) {
        cmd = QString(R"(git diff -M -- "%1")").arg(file.path);
    } else {
        cmd = QString(R"(git diff -M --cached -- "%1")").arg(file.path);
    }
    indicator->startHint();
    m_diffWorker =
        QtConcurrent::run(m_threadPool, [projectPath, cmd](QPromise<DiffResult> &promise) {
            QString cmdResult = global::getCmdResult(cmd, projectPath);
            if (promise.isCanceled()) {
                return;
            }

            DiffResult result;
            result.hunks = global::parseDiffHunks(cmdResult);

            promise.addResult(result);
        });
    m_diffWorker
        .then(this,
            [this](const DiffResult &result) {
                this->m_indicator->stopHint();
                this->m_diffHunks = result.hunks;
                updateUI(Diff);
            })
        .onCanceled(this, [this] {
            this->m_indicator->stopHint();
        });
}

void ChangesPage::onFileDoubleClicked(int row, int column)
{
    QString cmd;
    if (sender() == ui->stagedTable) {
        GitFile file = m_stagedList.at(row);
        cmd = "git reset -- " + file.path;
    } else {
        GitFile file = m_unstagedList.at(row);
        cmd = "git add " + file.path;
    }

    CmdDialog dialog(this, cmd, m_projectPath, true);
    dialog.exec();
    reset(List | Diff);
    getChangesAsync(m_projectPath, m_indicator);
}

void ChangesPage::onFileSelected()
{
    QTableWidget *table = qobject_cast<QTableWidget *>(sender());
    QModelIndexList indexes = table->selectionModel()->selectedIndexes();
    if (indexes.empty()) {
        return;
    }
    GitFile file;
    if (sender() == ui->stagedTable) {
        file = m_stagedList.at(indexes.first().row());
        ui->unstagedTable->clearSelection();
    } else {
        file = m_unstagedList.at(indexes.first().row());
        ui->stagedTable->clearSelection();
    }
    ui->curFileLabel->setText(file.path);
    getDiffAsync(m_projectPath, file, m_indicator);
}

void ChangesPage::onTableButtonClicked()
{
    QString cmd;
    if (sender() == ui->stageAllBtn) {
        cmd = "git add .";
    } else {
        cmd = "git reset";
    }

    CmdDialog dialog(this, cmd, m_projectPath, true);
    dialog.exec();
    reset(List | Diff);
    getChangesAsync(m_projectPath, m_indicator);
}

void ChangesPage::onAmendToggled(bool checked)
{
    if (checked) {
        const QString &projectPath = m_projectPath;
        m_indicator->startHint();
        m_amendWorker.cancel();
        m_amendWorker = QtConcurrent::run(m_threadPool, [projectPath]() {
            return global::getCmdResult("git log -n1 --format=%B", projectPath);
        });
        m_amendWorker
            .then(this,
                [&](QString body) {
                    m_indicator->stopHint();
                    ui->commitTextEdit->clear();
                    ui->commitTextEdit->insertPlainText(body.trimmed());
                })
            .onCanceled(this, [&] {
                this->m_indicator->stopHint();
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
    CmdDialog dialog(this, cmd, m_projectPath, true);
    int code = dialog.exec();
    if (code == 0) {
        reset(All);
        emit commitEvent(HistorySelectionArg(HistorySelectionArg::First));
    }
}

void ChangesPage::showEvent(QShowEvent *event)
{
    refresh();
}
