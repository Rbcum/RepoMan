#include "reftreeview.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QMenu>
#include <QtConcurrent>

#include "dialogs/branchdialog.h"
#include "dialogs/cmddialog.h"
#include "dialogs/deleterefdialog.h"
#include "global.h"

RefTreeView::RefTreeView(QWidget *parent)
    : QTreeView(parent), m_remotesLoaded(false), m_tagsLoaded(false)
{
    m_model = new RefTreeModel(this);
    m_delegate = new RefTreeDelegate(this, this);

    setModel(m_model);
    setItemDelegate(m_delegate);
    setIconSize(QSize(22, 22));

    connect(this, &QTreeView::expanded, this, &RefTreeView::onExpanded);
    connect(this, &QTreeView::doubleClicked, this, &RefTreeView::onDoubleClicked);
    connect(this, &QTreeView::customContextMenuRequested, this, &RefTreeView::onMenuRequested);

    expand(m_model->index(RefTreeItem::Branch, 0));
}

RefTreeView::~RefTreeView()
{
    m_branchesFetcher.cancel();
    m_remotesFetcher.cancel();
    m_tagsFetcher.cancel();
}

void RefTreeView::setProjectPath(const QString &path)
{
    m_projectPath = path;
    m_remotesLoaded = false;
    m_tagsLoaded = false;

    collapse(m_model->index(RefTreeItem::Remote, 0));
    collapse(m_model->index(RefTreeItem::Tag, 0));
    m_model->reset();

    m_branchesFetcher.cancel();
    m_remotesFetcher.cancel();
    m_tagsFetcher.cancel();

    getBranchesAsync(m_projectPath);
}

void RefTreeView::refresh()
{
    m_model->reset();
    m_branchesFetcher.cancel();
    m_remotesFetcher.cancel();
    m_tagsFetcher.cancel();

    m_remotesLoaded = false;
    m_tagsLoaded = false;

    getBranchesAsync(m_projectPath);
    if (isExpanded(m_model->index(RefTreeItem::Remote, 0))) {
        getRemotesAsync(m_projectPath);
    }
    if (isExpanded(m_model->index(RefTreeItem::Tag, 0))) {
        getTagsAsync(m_projectPath);
    }
}

void RefTreeView::onExpanded(const QModelIndex &index)
{
    RefTreeItem *item = static_cast<RefTreeItem *>(index.internalPointer());
    if (item->type == RefTreeItem::Group) {
        if (item->row == RefTreeItem::Remote && !m_remotesLoaded) {
            getRemotesAsync(m_projectPath);
        } else if (item->row == RefTreeItem::Tag && !m_tagsLoaded) {
            getTagsAsync(m_projectPath);
        }
    }
}

void RefTreeView::onDoubleClicked(const QModelIndex &index)
{
    RefTreeItem *item = static_cast<RefTreeItem *>(index.internalPointer());
    if (item->type == RefTreeItem::Branch) {
        QString cmd = QString("git checkout %1").arg(item->data);
        CmdDialog dialog(parentWidget(), cmd, m_projectPath, true);
        if (dialog.exec() == 0) {
            HistorySelectionArg arg(HistorySelectionArg::Head, item->data);
            emit requestRefreshEvent(arg);
        }
    } else if (item->type == RefTreeItem::Remote) {
        BranchRemoteDialog dialog(this, m_projectPath, item->data);
        if (dialog.exec()) {
            HistorySelectionArg arg(HistorySelectionArg::Remote, item->data);
            emit requestRefreshEvent(arg);
        }
    } else if (item->type == RefTreeItem::Tag) {
        QString cmd = QString("git checkout %1").arg(item->data);
        CmdDialog dialog(parentWidget(), cmd, m_projectPath, true);
        if (dialog.exec() == 0) {
            HistorySelectionArg arg(HistorySelectionArg::Tag, item->data);
            emit requestRefreshEvent(arg);
        }
    }
}

void RefTreeView::onMenuRequested(const QPoint &pos)
{
    QModelIndex index = indexAt(pos);
    if (!index.isValid()) return;
    RefTreeItem *item = static_cast<RefTreeItem *>(index.internalPointer());
    if (item->type == RefTreeItem::Group || item->type == RefTreeItem::General) return;

    QMenu menu;
    menu.addAction("Checkout...", this, [&]() {
        onDoubleClicked(index);
    });
    menu.addAction("Delete...", this, [&]() {
        deleteRef(item);
    });
    menu.addSeparator();
    menu.addAction("Copy Ref Name", this, [&]() {
        QGuiApplication::clipboard()->setText(item->data);
    });
    menu.exec(mapToGlobal(pos));
}

void RefTreeView::getBranchesAsync(const QString &projectPath)
{
    m_delegate->setBranchesLoading(true);
    m_branchesFetcher = QtConcurrent::run([projectPath](QPromise<QStringList> &promise) {
        QStringList result;
        QString currentBranch;
        QStringList lines = global::getCmdResult("git branch", projectPath).split('\n');
        if (promise.isCanceled()) {
            return;
        }
        for (const QString &line : lines) {
            QString trLine = line.trimmed();
            if (trLine.size() > 0 && !trLine.startsWith("* (HEAD detached")) {
                if (trLine.startsWith("*")) {
                    trLine = trLine.mid(2);
                    currentBranch = trLine;
                }
                result << trLine;
            }
        }
        result << currentBranch;
        promise.addResult(result);
    });
    QPointer thisPtr(this);
    m_branchesFetcher
        .then(this,
            [this](const QStringList &result) {
                m_delegate->setBranchesLoading(false);
                m_delegate->setCurrentBranch(result.last());
                m_model->addRefData(result.mid(0, result.size() - 1), RefTreeItem::Branch);
            })
        .onCanceled(qApp, [thisPtr] {
            if (thisPtr.isNull()) return;
            thisPtr->m_delegate->setBranchesLoading(false);
        });
}

void RefTreeView::getRemotesAsync(const QString &projectPath)
{
    m_delegate->setRemotesLoading(true);
    m_remotesFetcher = QtConcurrent::run([projectPath](QPromise<QStringList> &promise) {
        QStringList result;
        QStringList lines = global::getCmdResult("git branch -r", projectPath).split('\n');
        if (promise.isCanceled()) {
            return;
        }
        for (const QString &line : lines) {
            QString trLine = line.trimmed();
            if (trLine.contains(" -> ")) {
                trLine = trLine.split(" -> ").last();
            }
            if (trLine.size() > 0) {
                result << trLine;
            }
        }
        promise.addResult(result);
    });
    QPointer thisPtr(this);
    m_remotesFetcher
        .then(this,
            [this](const QStringList &result) {
                m_remotesLoaded = true;
                m_delegate->setRemotesLoading(false);
                m_model->addRefData(result, RefTreeItem::Remote);
            })
        .onCanceled(qApp, [thisPtr] {
            if (thisPtr.isNull()) return;
            thisPtr->m_delegate->setBranchesLoading(false);
        });
}

void RefTreeView::getTagsAsync(const QString &projectPath)
{
    m_delegate->setTagsLoading(true);
    m_tagsFetcher = QtConcurrent::run([projectPath](QPromise<QStringList> &promise) {
        QStringList result;
        QStringList lines = global::getCmdResult("git tag", projectPath).split('\n');
        if (promise.isCanceled()) {
            return;
        }
        for (const QString &line : lines) {
            QString trLine = line.trimmed();
            if (trLine.size() > 0) {
                result << trLine;
            }
        }
        promise.addResult(result);
    });
    QPointer thisPtr(this);
    m_tagsFetcher
        .then(this,
            [this](const QStringList &result) {
                m_tagsLoaded = true;
                m_delegate->setTagsLoading(false);
                m_model->addRefData(result, RefTreeItem::Tag);
            })
        .onCanceled(qApp, [thisPtr] {
            if (thisPtr.isNull()) return;
            thisPtr->m_delegate->setBranchesLoading(false);
        });
}

void RefTreeView::deleteRef(RefTreeItem *item)
{
    if (item->type == RefTreeItem::Branch) {
        DeleteLocalBranchDialog dialog(this, m_projectPath, item->data);
        if (dialog.exec()) {
            emit requestRefreshEvent();
        }
    } else if (item->type == RefTreeItem::Remote) {
        DeleteRemoteBranchDialog dialog(this, m_projectPath, item->data);
        if (dialog.exec()) {
            emit requestRefreshEvent();
        }
    } else if (item->type == RefTreeItem::Tag) {
        DeleteTagDialog dialog(this, m_projectPath, item->data);
        if (dialog.exec()) {
            emit requestRefreshEvent();
        }
    }
}
