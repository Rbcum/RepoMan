#include "pagehost.h"

#include <QDesktopServices>
#include <QShortcut>

#include "dialogs/cleandialog.h"
#include "dialogs/fetchdialog.h"
#include "dialogs/pulldialog.h"
#include "dialogs/pushdialog.h"
#include "ui_pagehost.h"

using namespace global;

PageHost::PageHost(const RepoProject &project)
    : QWidget(nullptr), ui(new Ui::PageHost), m_project(project)
{
    ui->setupUi(this);
    ui->centerSplitter->setStretchFactor(1, 1);

    // Mode Buttons
    connect(ui->changesModeBtn, &QPushButton::toggled, this, &PageHost::onChangeMode);
    connect(ui->historyModeBtn, &QPushButton::toggled, this, &PageHost::onChangeMode);

    // Ref tree
    connect(ui->refTreeView, &QTreeView::clicked, this, &PageHost::onRefClicked);
    connect(ui->refTreeView, &RefTreeView::requestRefreshEvent, this, [&](HistorySelectionArg arg) {
        refresh(arg);
    });

    ui->refTreeView->setProjectPath(m_project.absPath);

    m_changesPage = new ChangesPage(this, m_project);
    m_historyPage = new HistoryPage(this, m_project);
    ui->rightPanel->insertWidget(0, m_changesPage);
    ui->rightPanel->insertWidget(1, m_historyPage);
    connect(m_changesPage, &ChangesPage::commitEvent, this, [&](HistorySelectionArg arg) {
        refresh(arg);
        ui->historyModeBtn->setChecked(true);
    });
    connect(m_changesPage, &ChangesPage::newChangesEvent, this, [&](int count) {
        QString text = "Changes" + (count ? " (" + QString::number(count) + ")" : "");
        ui->changesModeBtn->setText(text);
    });
    connect(m_historyPage, &HistoryPage::requestRefreshEvent, this, [&]() {
        refresh();
    });

    // Shortcuts
    QShortcut *shortcut = new QShortcut(QKeySequence::Refresh, this);
    connect(shortcut, &QShortcut::activated, this, [&]() {
        refresh();
    });

    ui->changesModeBtn->setChecked(true);
}

PageHost::~PageHost()
{
    delete ui;
}

void PageHost::onActionPush()
{
    PushDialog dialog(this, m_project.absPath);
    if (dialog.exec()) {
        refresh();
    }
}

void PageHost::onActionPull()
{
    PullDialog dialog(this, m_project.absPath);
    if (dialog.exec()) {
        refresh();
    }
}

void PageHost::onActionFetch()
{
    FetchDialog dialog(this, m_project.absPath);
    if (dialog.exec()) {
        refresh();
    }
}

void PageHost::onActionClean()
{
    CleanDialog dialog(this, m_project.absPath);
    if (dialog.exec()) {
        refresh();
    }
}

void PageHost::onActionTerm()
{
    QProcess p;
    p.setProgram("/usr/bin/x-terminal-emulator");
    p.setWorkingDirectory(m_project.absPath);
    p.startDetached();
}

void PageHost::onActionFolder()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_project.absPath));
}

void PageHost::refresh(const HistorySelectionArg &arg)
{
    ui->refTreeView->refresh();
    m_changesPage->refresh();
    m_historyPage->refresh(arg);
}

void PageHost::onRefClicked(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }
    RefTreeItem *item = static_cast<RefTreeItem *>(index.internalPointer());
    if (item->type >= RefTreeItem::Branch) {
        ui->historyModeBtn->setChecked(true);
        m_historyPage->jumpToRef(item);
    }
}

void PageHost::onChangeMode()
{
    QPushButton *btn = static_cast<QPushButton *>(sender());
    if (!btn->isChecked()) return;

    ui->rightPanel->setCurrentWidget(
        btn == ui->changesModeBtn ? m_changesPage : (QWidget *)m_historyPage);
}
