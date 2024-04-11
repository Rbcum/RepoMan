#include "mainwindow.h"

#include <QAbstractItemView>
#include <QActionGroup>
#include <QCompleter>
#include <QDesktopServices>
#include <QDomDocument>
#include <QFileDialog>
#include <QFileSystemModel>
#include <QInputDialog>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>
#include <QTextCodec>
#include <QToolButton>
#include <QtConcurrent>

#include "dialogs/cmddialog.h"
#include "dialogs/repoinitdialog.h"
#include "dialogs/reposyncdialog.h"
#include "dialogs/switchmanifestdialog.h"
#include "pages/newtabpage.h"
#include "ui_mainwindow.h"

using namespace global;

MainWindow::MainWindow(QString repoPath)
    : QMainWindow(nullptr), ui(new Ui::MainWindow), m_context(repoPath)
{
    ui->setupUi(this);
    move(screen()->geometry().center() - frameGeometry().center());

    // MenuBar
    connect(ui->actionFile_Open, &QAction::triggered, this, &MainWindow::onAction);
    connect(ui->actionFile_Configurations, &QAction::triggered, this, &MainWindow::onAction);
    connect(ui->actionFile_Exit, &QAction::triggered, this, &MainWindow::onAction);
    connect(ui->actionRepo_Init, &QAction::triggered, this, &MainWindow::onAction);
    connect(ui->actionRepo_Switch_manifest, &QAction::triggered, this, &MainWindow::onAction);
    connect(ui->actionRepo_Start, &QAction::triggered, this, &MainWindow::onAction);
    connect(ui->actionRepo_Sync, &QAction::triggered, this, &MainWindow::onAction);
    connect(ui->actionHelp_About, &QAction::triggered, this, &MainWindow::onAction);

    // ToolBar
    ui->toolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    m_actionRepoSwitchManifest = ui->toolBar->addAction(
        QIcon("://resources/icon_switch_manifest.svg"), "Manifest", this, &MainWindow::onAction);
    m_actionRepoSwitchManifest->setToolTip("Switch Manifest");
    m_actionRepoSync = ui->toolBar->addAction(
        QIcon("://resources/icon_sync.svg"), "Sync", this, &MainWindow::onAction);
    m_actionRepoSync->setToolTip("Repo Sync");
    ui->toolBar->widgetForAction(ui->toolBar->addWidget(new QWidget()))->setFixedSize(40, 0);
    m_actionPush = ui->toolBar->addAction(
        QIcon("://resources/action_push.svg"), "Push", this, &MainWindow::onAction);
    m_actionPull = ui->toolBar->addAction(
        QIcon("://resources/action_pull.svg"), "Pull", this, &MainWindow::onAction);
    m_actionFetch = ui->toolBar->addAction(
        QIcon("://resources/action_fetch.svg"), "Fetch", this, &MainWindow::onAction);
    m_actionClean = ui->toolBar->addAction(
        QIcon("://resources/action_clean.svg"), "Clean", this, &MainWindow::onAction);
    ui->toolBar->widgetForAction(ui->toolBar->addWidget(new QWidget()))
        ->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_actionTerm = ui->toolBar->addAction(
        QIcon("://resources/action_term.svg"), "Terminal", this, &MainWindow::onAction);
    m_actionFolder = ui->toolBar->addAction(
        QIcon("://resources/action_folder.svg"), "Explorer", this, &MainWindow::onAction);

    // StatusBar
    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("QLabel{font-size: 13px;}");
    ui->statusbar->addWidget(m_statusLabel);

    // Tab
    connect(ui->tabWidget, &TabWidgetEx::tabAddRequested, this, [this] {
        int newIndex = addTab(Project(), true);
        ui->tabWidget->setCurrentIndex(newIndex);
    });
    connect(ui->tabWidget, &TabWidgetEx::tabCloseRequested, this, [this](int index) {
        closeTab(index);
        if (ui->tabWidget->count() == 0) {
            addTab(Project(), true);
        }
        saveTabs();
    });

    if (m_context.isValid()) {
        updateUI();
        restoreTabs();
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::onAction()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action == ui->actionFile_Open) {
        onActionOpen();
    } else if (action == ui->actionFile_Exit) {
        qApp->quit();
    } else if (action == ui->actionFile_Configurations) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_context.settings().fileName()));
    } else if (action == ui->actionRepo_Init) {
        RepoInitDialog dialog(this);
        if (dialog.exec()) {
            openRepo(dialog.initDir());
        }
    } else if (action == ui->actionRepo_Sync || action == m_actionRepoSync) {
        onActionRepoSync();
    } else if (action == ui->actionRepo_Start) {
        onActionRepoStart();
    } else if (action == ui->actionRepo_Switch_manifest || action == m_actionRepoSwitchManifest) {
        SwitchManifestDialog dialog(this, m_context);
        if (dialog.exec()) {
            m_context.loadManifest();
            updateUI();
            if (dialog.syncAfterSwitch()) {
                onActionRepoSync();
            }
        }
    } else if (action == ui->actionHelp_About) {
        QMessageBox::about(this, "RepoMan", "Repo GUI front-end");
    } else if (action == m_actionPush) {
        onProjectAction(&PageHost::onActionPush);
    } else if (action == m_actionPull) {
        onProjectAction(&PageHost::onActionPull);
    } else if (action == m_actionFetch) {
        onProjectAction(&PageHost::onActionFetch);
    } else if (action == m_actionClean) {
        onProjectAction(&PageHost::onActionClean);
    } else if (action == m_actionTerm) {
        onProjectAction(&PageHost::onActionTerm);
    } else if (action == m_actionFolder) {
        onProjectAction(&PageHost::onActionFolder);
    }
}

void MainWindow::onProjectAction(void (PageHost::*func)())
{
    int index = ui->tabWidget->currentIndex();
    if (index < 0) {
        return;
    }
    auto data = ui->tabWidget->tabData(index).value<TabData>();
    if (!data.isNewTab) {
        (static_cast<PageHost *>(data.page)->*func)();
    }
}

int MainWindow::addTab(const Project &project, bool isNewTab, int index)
{
    QString title;
    TabData data;
    data.isNewTab = isNewTab;
    data.project = project;
    if (isNewTab) {
        title = "New tab";
        auto page = new NewTabPage(m_context);
        data.page = page;
        connect(page, &NewTabPage::projectDoubleClicked, this, &MainWindow::openProject);
    } else {
        title = project.path;
        data.page = new PageHost(m_context, project);
    }
    int newIndex;
    if (index != -1) {
        newIndex = ui->tabWidget->insertTab(index, data.page, title);
    } else {
        newIndex = ui->tabWidget->addTab(data.page, title);
    }
    ui->tabWidget->setTabData(newIndex, QVariant::fromValue(data));
    return newIndex;
}

void MainWindow::closeTab(int index)
{
    auto data = ui->tabWidget->tabData(index).value<TabData>();
    ui->tabWidget->removeTab(index);
    data.page->deleteLater();
}

void MainWindow::openProject(const Project &project)
{
    int index = 0;
    int count = ui->tabWidget->count();
    for (int i = 0; i < count; ++i) {
        auto data = ui->tabWidget->tabData(i).value<TabData>();
        if (data.page == sender()) {
            index = i;
            break;
        }
    }
    addTab(project, false, index + 1);
    closeTab(index);
    saveTabs();
}

void MainWindow::saveTabs()
{
    QString value;
    int count = ui->tabWidget->count();
    for (int i = 0; i < count; ++i) {
        auto data = ui->tabWidget->tabData(i).value<TabData>();
        if (data.isNewTab) continue;
        value.append(data.project.path);
        if (i < count - 1) value.append(";");
    }

    auto settings = m_context.settings();
    settings.setValue("tabs", value);
}

void MainWindow::restoreTabs()
{
    QString value = m_context.settings().value("tabs").toString();
    if (value.isEmpty()) {
        addTab(Project(), true);
    } else {
        for (const QString &path : value.split(";")) {
            addTab(m_context.manifest().projectMap.value(path));
        }
    }
}

void MainWindow::closeAllTabs()
{
    while (ui->tabWidget->count()) {
        closeTab(0);
    }
}

void MainWindow::openRepo(const QString &path)
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Open Repo");
    msgBox.setText("Where to open this repo in?");
    msgBox.setIcon(QMessageBox::Question);
    auto currentBtn = msgBox.addButton("Current Window", QMessageBox::ActionRole);
    auto newBtn = msgBox.addButton("New Window", QMessageBox::ActionRole);
    msgBox.addButton(QMessageBox::Cancel);
    msgBox.setDefaultButton(currentBtn);
    msgBox.exec();
    if (msgBox.clickedButton() == currentBtn) {
        m_context = RepoContext(path);
        updateUI();
        closeAllTabs();
        restoreTabs();
    } else if (msgBox.clickedButton() == newBtn) {
        auto win = new MainWindow(path);
        win->show();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QMainWindow::closeEvent(event);
    QSettings().setValue("cwd", m_context.isValid() ? m_context.repoPath() : "");
}

void MainWindow::onActionRepoSync()
{
    QList<Project> projects;
    int currentIndex = -1;
    for (int i = 0; i < ui->tabWidget->count(); ++i) {
        auto data = ui->tabWidget->tabData(i).value<TabData>();
        if (!data.isNewTab) {
            projects.append(data.project);
            if (i == ui->tabWidget->currentIndex()) {
                currentIndex = projects.size() - 1;
            }
        }
    }
    RepoSyncDialog dialog(this, m_context, currentIndex, projects);
    dialog.exec();
}

void MainWindow::onActionRepoStart()
{
    QString targetManifest = QFileInfo(m_context.manifest().filePath).symLinkTarget();
    QString defName = QFileInfo(targetManifest).fileName().split(".").first();

    QInputDialog inputDlg(this);
    inputDlg.setWindowTitle("Repo start");
    inputDlg.setLabelText("New branch name:");
    inputDlg.setTextValue(defName);
    inputDlg.resize(500, 0);

    if (inputDlg.exec()) {
        QString branch = inputDlg.textValue();
        if (!branch.isEmpty()) {
            QString cmd = QString("repo start --all %1").arg(branch);
            CmdDialog::execute(this, cmd, m_context.repoPath());
        }
    }
}

void MainWindow::onActionOpen()
{
    QString path = QFileDialog::getExistingDirectory(this, "Open repo", m_context.repoPath());
    if (path.isEmpty()) {
        return;
    }
    if (!QDir(QDir::cleanPath(path + "/.repo")).exists()) {
        QMessageBox msgBox;
        msgBox.warning(this, "Error", path + " is not a valid repo");
        return;
    }
    openRepo(path);
}

void MainWindow::updateUI()
{
    setWindowTitle(QString("%1 - %2").arg(m_context.repoPath(), QApplication::applicationName()));

    QString targetManifest = QFileInfo(m_context.manifest().filePath).symLinkTarget();
    m_statusLabel->setText(
        QString("  %1  |  %2")
            .arg(QFileInfo(targetManifest).fileName(), m_context.manifest().revision));
}
