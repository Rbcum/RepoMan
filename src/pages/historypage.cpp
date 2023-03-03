#include "historypage.h"

#include <QMenu>
#include <QScrollBar>
#include <QShortcut>
#include <QtConcurrent>

#include "dialogs/branchdialog.h"
#include "dialogs/checkoutdialog.h"
#include "dialogs/resetdialog.h"
#include "ui_historypage.h"

HistoryPage::HistoryPage(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::HistoryPage),
      m_logThreadPool(new QThreadPool(this)),
      m_threadPool(new QThreadPool(this))
{
    m_threadPool->setMaxThreadCount(1);
    m_logThreadPool->setMaxThreadCount(1);

    ui->setupUi(this);
    ui->splitter->setSizes(QList<int>({300, 300}));
    ui->splitter_2->setSizes(QList<int>({200, 300}));
    ui->splitter_3->setSizes(QList<int>({200, 200}));

    m_indicator = new QProgressIndicator(this);

    connect(ui->orderComboBox, &QComboBox::currentIndexChanged, this,
        &HistoryPage::onDisplayParamsChanged);
    connect(ui->branchComboBox, &QComboBox::currentIndexChanged, this,
        &HistoryPage::onDisplayParamsChanged);
    connect(
        ui->remotesCheckBox, &QCheckBox::stateChanged, this, &HistoryPage::onDisplayParamsChanged);

    connect(ui->detailScrollArea, &CommitDetailScrollArea::linkClicked, this,
        &HistoryPage::onParentLinkClicked);
    m_graphDelegate = new HistoryGraphDelegate(this);
    m_historyModel = new HistoryTableModel(this);
    ui->tableView->setModel(m_historyModel);
    ui->tableView->setItemDelegate(m_graphDelegate);
    connect(
        m_historyModel, &HistoryTableModel::fetchMoreEvt, this, &HistoryPage::onFetchMoreCommits);
    connect(ui->tableView->selectionModel(), &QItemSelectionModel::currentChanged, this,
        &HistoryPage::onCommitSelected);
    connect(ui->tableView, &QHistoryTableView::cancelLoading, this, &HistoryPage::onCancelLoading);
    connect(ui->tableView, &QTableView::doubleClicked, this, &HistoryPage::onTableDoubleClick);
    connect(ui->tableView, &QTableView::customContextMenuRequested, this,
        &HistoryPage::onTableMenuRequested);

    connect(this, &HistoryPage::logResult, this, &HistoryPage::onLogResult);

    ui->fileTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    connect(ui->fileTable, &QTableWidget::itemSelectionChanged, this, &HistoryPage::onFileSelected);
}

HistoryPage::~HistoryPage()
{
    reset(All);
    delete ui;
}

void HistoryPage::setCurrentProjectPath(const QString &path)
{
    if (m_projectPath == path) {
        return;
    }
    reset(All);
    this->m_projectPath = path;
    m_historyModel->fetchMore(QModelIndex());
}

void HistoryPage::refresh(const HistorySelectionArg &arg)
{
    auto newArg = arg;
    if (newArg.type == HistorySelectionArg::Null) {
        newArg.type = HistorySelectionArg::Hash;
        newArg.data = m_currentCommit.hash;
    }
    reset(All);
    m_historyModel->fetchMore(QModelIndex(), newArg);
}

void HistoryPage::updateUI(unsigned flags)
{
    if (flags & Table) {
        this->m_graphDelegate->addGraphTable(m_logResult.graphTable);
        this->m_historyModel->addCommits(m_logResult.commits);
    }
    if (flags & Detail) {
        ui->detailScrollArea->setCommit(m_currentCommit, m_detailResult.rawBody);

        const QList<GitFile> &fileList = m_detailResult.fileList;
        for (int i = 0; i < fileList.size(); ++i) {
            GitFile f = fileList.at(i);
            ui->fileTable->insertRow(i);
            QTableWidgetItem *modeItem = new QTableWidgetItem(f.mode);
            modeItem->setToolTip(f.path);
            ui->fileTable->setItem(i, 0, modeItem);
            QTableWidgetItem *fileItem = new QTableWidgetItem(f.path);
            fileItem->setToolTip(f.path);
            ui->fileTable->setItem(i, 1, fileItem);
        }
        if (ui->fileTable->rowCount() > 0) {
            ui->fileTable->selectRow(0);
        }
    }
    if (flags & Diff) {
        ui->diffScrollArea->setDiffHunks(m_diffResult.hunks);
    }
}

void HistoryPage::reset(unsigned flags)
{
    if (flags & Table) {
        m_logResult = {};
        m_logWorker.cancel();
        m_historyModel->reset();
        m_graphDelegate->reset();
    }
    if (flags & Detail) {
        //        m_currentCommit = {};
        m_detailResult = {};
        m_detailWorker.cancel();
        ui->detailScrollArea->reset();
        ui->fileTable->setRowCount(0);
    }
    if (flags & Diff) {
        m_diffResult = {};
        m_diffWorker.cancel();
        ui->diffScrollArea->reset();
    }
}

void HistoryPage::jumpToRef(RefTreeItem *ref)
{
    HistorySelectionArg arg;
    arg.data = ref->data;
    switch (ref->type) {
        case RefTreeItem::Branch:
            arg.type = HistorySelectionArg::Head;
            break;
        case RefTreeItem::Remote:
            arg.type = HistorySelectionArg::Remote;
            break;
        case RefTreeItem::Tag:
            arg.type = HistorySelectionArg::Tag;
            break;
        default:
            Q_UNREACHABLE();
    }
    const QModelIndex &index = m_historyModel->searchCommit(arg);
    if (index.isValid()) {
        m_logWorker.cancel();
        ui->tableView->selectRow(index.row());
        ui->tableView->scrollTo(index);
    } else {
        m_historyModel->fetchMore(QModelIndex(), arg);
    }
}

void HistoryPage::selectTargetRow(const HistorySelectionArg &arg)
{
    if (arg.isSearchable()) {
        const QModelIndex &index = m_historyModel->searchCommit(arg);
        if (index.isValid()) {
            ui->tableView->selectRow(index.row());
            ui->tableView->scrollTo(index);
        }
    } else if (arg.type == HistorySelectionArg::First ||
               !ui->tableView->selectionModel()->hasSelection()) {
        ui->tableView->selectRow(0);
    }
}

void HistoryPage::onCommitSelected(const QModelIndex &current, const QModelIndex &previous)
{
    reset(Detail | Diff);
    const Commit &commit = current.data(HistoryTableModel::CommitRole).value<Commit>();
    const QString &projectPath = this->m_projectPath;

    m_currentCommit = commit;
    m_indicator->startHint();
    m_detailWorker = QtConcurrent::run(m_threadPool, [projectPath, commit](
                                                         QPromise<DetailResult> &promise) {
        DetailResult result;
        result.rawBody =
            global::getCmdResult("git log --format=%B -n1 " + commit.hash, projectPath);
        QString cmdResult;
        if (commit.parents.size() > 1) {
            cmdResult = global::getCmdResult(
                QString("git diff --name-status -M %1 %2").arg(commit.parents.first(), commit.hash),
                projectPath);
        } else {
            cmdResult = global::getCmdResult(
                QString("git diff-tree --name-status --no-commit-id -M -r --root %1")
                    .arg(commit.hash),
                projectPath);
        }
        QStringList lines = cmdResult.split('\n');
        for (int i = 0; i < lines.size(); ++i) {
            QString line = lines[i];
            if (line.isEmpty()) {
                continue;
            }
            QStringList parts = line.split('\t');
            if (parts[0].startsWith("R")) {
                result.fileList.emplaceBack(parts[2], "R");
            } else {
                result.fileList.emplaceBack(parts[1], parts[0]);
            }
        }
        promise.addResult(result);
    });
    m_detailWorker
        .then(this,
            [this](const DetailResult &result) {
                this->m_detailResult = result;
                this->m_indicator->stopHint();
                updateUI(Detail);
            })
        .onCanceled(this, [this] {
            this->m_indicator->stopHint();
        });
}

void HistoryPage::onFileSelected()
{
    reset(Diff);
    const QString &projectPath = this->m_projectPath;
    const Commit &commit = this->m_currentCommit;
    QModelIndexList indexes = ui->fileTable->selectionModel()->selectedIndexes();
    if (indexes.empty()) {
        return;
    }
    const GitFile &file = m_detailResult.fileList.at(indexes.first().row());
    QString cmd;
    if (commit.parents.size() > 1) {
        cmd =
            QString("git diff -M %1 %2 -- %3").arg(commit.parents.first(), commit.hash, file.path);
    } else {
        cmd = QString("git diff-tree -M -p --root %1 -- %2").arg(commit.hash, file.path);
    }
    m_indicator->startHint();
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
                this->m_diffResult = result;
                updateUI(Diff);
            })
        .onCanceled(this, [this] {
            this->m_indicator->stopHint();
        });
}

void HistoryPage::onParentLinkClicked(const QString &hash)
{
    qDebug() << "msg";
    HistorySelectionArg arg(HistorySelectionArg::Hash, hash);
    const QModelIndex &index = m_historyModel->searchCommit(arg);
    if (index.isValid()) {
        m_logWorker.cancel();
        ui->tableView->selectRow(index.row());
        ui->tableView->scrollTo(index);
    } else {
        m_historyModel->fetchMore(QModelIndex(), arg);
    }
}

void HistoryPage::onDisplayParamsChanged()
{
    HistorySelectionArg arg(HistorySelectionArg::Hash, m_currentCommit.hash);
    reset(All);
    m_historyModel->fetchMore(QModelIndex(), arg);
}

void HistoryPage::onCancelLoading()
{
    m_logWorker.cancel();
}

void HistoryPage::onTableMenuRequested(const QPoint &pos)
{
    QModelIndex index = ui->tableView->indexAt(pos);
    if (!index.isValid()) return;
    const Commit &commit = index.data(HistoryTableModel::CommitRole).value<Commit>();

    QMenu menu;
    menu.addAction("Checkout...", this, [&]() {
        CheckoutDialog dialog(this, m_projectPath, commit);
        if (dialog.exec()) {
            emit requestRefreshEvent();
        }
    });
    menu.addAction("Branch...", this, [&]() {
        BranchCommitDialog dialog(this, m_projectPath, commit);
        if (dialog.exec()) {
            emit requestRefreshEvent();
        }
    });
    menu.addAction("Reset...", this, [&]() {
        ResetDialog dialog(this, m_projectPath, commit);
        if (dialog.exec()) {
            emit requestRefreshEvent();
        }
    });
    menu.exec(ui->tableView->mapToGlobal(pos));
}

void HistoryPage::onTableDoubleClick(const QModelIndex &index)
{
    const Commit &commit = index.data(HistoryTableModel::CommitRole).value<Commit>();
    CheckoutDialog dialog(this, m_projectPath, commit);
    if (dialog.exec()) {
        emit requestRefreshEvent();
    }
}

void HistoryPage::onFetchMoreCommits(int skip, const HistorySelectionArg &arg)
{
    if (m_projectPath.isEmpty()) {
        return;
    }
    if (arg.isSearchable()) {
        m_logWorker.cancel();
    } else if (m_logWorker.isRunning() && !m_logWorker.isCanceled()) {
        return;
    }

    LogResult &result = this->m_logResult;
    QString &path = this->m_projectPath;
    int orderType = ui->orderComboBox->currentIndex();
    int branchType = ui->branchComboBox->currentIndex();
    bool showRemotes = ui->remotesCheckBox->isChecked();
    int pageSize = global::commitPageSize;
    int originalSkip = skip;

    m_indicator->startHint();
    if (arg.isSearchable()) {
        ui->tableView->setLoading(true);
        ui->tableView->updateLoadingLabel(arg, skip);
    }
    m_logWorker = QtConcurrent::run(m_logThreadPool, [=](QPromise<void> &promise) mutable {
        while (true) {
            if (promise.isCanceled()) {
                break;
            }
            result.commits.clear();
            result.graphTable.clear();
            bool searchHit = false;

            readCommits(promise, path, skip, pageSize, orderType, branchType, showRemotes, arg,
                result, searchHit);
            if (promise.isCanceled()) {
                break;
            }
            buildGraphTable(promise, result);
            if (promise.isCanceled()) {
                break;
            }

            emit logResult(result, arg, skip + result.commits.size());

            if (!arg.isSearchable() || searchHit || result.commits.size() < pageSize) {
                break;
            }
            skip += result.commits.size();
        }
    });
    m_logWorker
        .then(this,
            [this, arg]() {
                m_indicator->stopHint();
                if (arg.isSearchable()) {
                    ui->tableView->setLoading(false);
                }
                selectTargetRow(arg);
            })
        .onCanceled(this, [this, arg] {
            this->m_indicator->stopHint();
            if (arg.isSearchable()) {
                ui->tableView->setLoading(false);
            }
        });
}

void HistoryPage::onLogResult(LogResult result, HistorySelectionArg arg, int count)
{
    this->m_logResult = result;
    updateUI(Table);
    if (arg.isSearchable()) {
        ui->tableView->updateLoadingLabel(arg, count);
    }
}

void HistoryPage::readCommits(QPromise<void> &promise, const QString &projectPath, int skip,
    int maxCount, int orderType, int branchType, bool showRemotes, const HistorySelectionArg &arg,
    LogResult &result, bool &searchHit)
{
    QString cmdResult = global::getCmdResult(
        QString("git log --decorate=full --skip=%1 --max-count=%2%3%4%5 --branches --tags "
                "--full-history --format=%H¿%h¿%P¿%ci¿%cn¿%ce¿%ai¿%an¿%ae¿%d¿%s HEAD")
            .arg(QString::number(skip), QString::number(maxCount),
                orderType ? " --topo-order" : " --date-order", branchType ? " --branches" : "",
                showRemotes ? " --remotes" : ""),
        projectPath);
    QStringList lines = cmdResult.split('\n');
    QString line;
    foreach (line, lines) {
        if (promise.isCanceled()) {
            return;
        }
        if (line.startsWith("fatal:")) {
            qDebug() << "readCommits()" << line;
            return;
        }
        if (line.isEmpty()) {
            continue;
        }
        //        qDebug() << line;
        QStringList parts = line.split("¿");
        Commit &c = result.commits.emplaceBack();
        c.hash = parts.at(0);
        c.shortHash = parts.at(1);
        c.parents = parts.at(2).isEmpty() ? QStringList() : parts.at(2).split(" ");
        c.commitDate = parts.at(3);
        c.committer = parts.at(4);
        c.committerEmail = parts.at(5);
        c.authorDate = parts.at(6);
        c.author = parts.at(7);
        c.authorEmail = parts.at(8);
        c.isHEAD = false;
        if (!parts.at(9).isEmpty()) {
            QStringList refs = parts.at(9).trimmed().sliced(1).chopped(1).split(", ");
            for (QString r : refs) {
                if (r.startsWith("HEAD")) {
                    c.isHEAD = true;
                    if (r.startsWith("HEAD -> ")) {
                        r = r.sliced(8);
                    }
                }
                if (r.startsWith("tag: refs/tags/"))
                    c.tags << r.sliced(15);
                else if (r.startsWith("refs/heads/"))
                    c.heads << r.sliced(11);
                else if (r.startsWith("refs/remotes/"))
                    c.remotes << r.sliced(13);
            }
        }
        c.subject = parts.at(10);

        if (arg.match(c)) {
            searchHit = true;
        }
    }
}

void HistoryPage::buildGraphTable(QPromise<void> &promise, LogResult &result)
{
    QList<QString> &nextCommits = result.nextCommits;
    QList<int> &nextLaneIds = result.nextLaneIds;
    int &newLaneId = result.newLaneId;

    for (int idx = 0; idx < result.commits.size(); ++idx) {
        if (promise.isCanceled()) {
            return;
        }
        // Iterate on copies since we may do structural changes to original data
        QList<QString> nextCommitsTmp = nextCommits;
        QList<int> nextLaneIdsTmp = nextLaneIds;
        GraphTableRow &rowLanes = result.graphTable.emplaceBack();
        GraphTableRow rowRoots;
        GraphTableRow rowTwigs;
        QList<int> rowTwigSlots;

        int commitMatches = 0;
        int commitIdx;
        const Commit &commit = result.commits[idx];
        //        qDebug() << QString::number(idx + 1) + "/" + QString::number(commits.size()) <<
        //        commit.hash;

        for (int i = 0; i < nextCommitsTmp.size(); ++i) {
            if (commit.hash == nextCommitsTmp[i]) {
                commitMatches++;
                if (commitMatches == 1) {
                    commitIdx = i;
                    CommitLane *lane = new CommitLane;
                    rowLanes.emplaceBack(lane);
                    lane->laneId = nextLaneIdsTmp[i];
                    lane->slot = i;
                    if (commit.parents.isEmpty()) {
                        lane->isRoot = true;
                        commitMatches++;
                        nextCommits.remove(i);
                        nextLaneIds.remove(i);
                    } else {
                        nextCommits[i] = commit.parents[0];
                        if (commit.parents.size() > 1) {
                            TwigLane *lane2 = new TwigLane;
                            rowTwigs.emplaceBack(lane2);
                            lane2->targetSlot = i;
                            QString parent2 = commit.parents[1];
                            int foundIdx = nextCommitsTmp.indexOf(parent2);
                            if (foundIdx != -1) {
                                lane2->laneId = nextLaneIdsTmp[foundIdx];
                                lane2->slot = foundIdx;
                            } else {
                                lane2->laneId = newLaneId++;
                                lane2->slot = nextCommitsTmp.size();
                                nextCommits.append(parent2);
                                nextLaneIds.append(lane2->laneId);
                            }
                            rowTwigSlots.append(lane2->slot);
                        }
                    }
                } else {
                    // root
                    RootLane *lane = new RootLane;
                    rowRoots.emplaceBack(lane);
                    lane->laneId = nextLaneIdsTmp[i];
                    lane->slot = i;
                    lane->targetSlot = commitIdx;

                    for (int twigIdx = 0; twigIdx < rowTwigs.size(); ++twigIdx) {
                        const QSharedPointer<GraphLane> &twig = rowTwigs[twigIdx];
                        if (rowTwigSlots[twigIdx] > i) {
                            twig->slot--;
                        }
                    }

                    nextCommits.remove(i - commitMatches + 2);
                    nextLaneIds.remove(i - commitMatches + 2);
                }
            } else {
                VerticalLane *lane = new VerticalLane;
                rowLanes.emplaceBack(lane);
                lane->laneId = nextLaneIdsTmp[i];
                lane->slot = i;
                lane->indentation = qMax(0, commitMatches - 1);
            }
        }
        rowLanes.append(rowRoots);
        rowLanes.append(rowTwigs);

        if (commitMatches == 0) {
            CommitLane *lane = new CommitLane;
            rowLanes.emplaceBack(lane);
            lane->laneId = newLaneId++;
            lane->slot = nextCommits.size();
            lane->isHead = true;
            if (commit.parents.isEmpty()) {
                lane->isRoot = true;
            } else {
                nextCommits.append(commit.parents[0]);
                nextLaneIds.append(lane->laneId);
                if (commit.parents.size() > 1) {
                    TwigLane *lane2 = new TwigLane;
                    lane2->targetSlot = lane->slot;
                    rowLanes.emplaceBack(lane2);
                    QString parent2 = commit.parents[1];
                    int foundIdx = nextCommitsTmp.indexOf(parent2);
                    if (foundIdx != -1) {
                        lane2->laneId = nextLaneIdsTmp[foundIdx];
                        lane2->slot = foundIdx;
                    } else {
                        lane2->laneId = newLaneId++;
                        lane2->slot = nextCommitsTmp.size() + 1;
                        nextCommits.append(parent2);
                        nextLaneIds.append(lane2->laneId);
                    }
                }
            }
        }

        result.laneCount += rowLanes.size();
        if (result.laneCount > 2000 * 10000) {
            // Memory overflow, consider serializing lanes data to DB
            break;
        }
    }

    // Debug
    //    for (int i = 0; i < table.size(); ++i) {
    //        auto row = table[i];
    //        qDebug() << "\n====" << i << "====";
    //        for (int j = 0; j < row.size(); ++j) {
    //            qDebug() << *row[j];
    //        }
    //    }
}
