#include "mainwindow.h"

#include <QActionGroup>
#include <QCompleter>
#include <QDesktopServices>
#include <QDomDocument>
#include <QFileDialog>
#include <QFileSystemModel>
#include <QInputDialog>
#include <QMessageBox>
#include <QSettings>
#include <QShortcut>
#include <QStandardPaths>
#include <QTextCodec>
#include <QToolButton>
#include <QtConcurrent>

#include "dialogs/cleandialog.h"
#include "dialogs/cmddialog.h"
#include "dialogs/fetchdialog.h"
#include "dialogs/pulldialog.h"
#include "dialogs/pushdialog.h"
#include "dialogs/reposyncdialog.h"
#include "global.h"
#include "ui_mainwindow.h"

using namespace global;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_threadPool(new QThreadPool(this))
{
    m_threadPool->setMaxThreadCount(1);

    ui->setupUi(this);
    ui->centerSplitter->setStretchFactor(1, 1);
    move(screen()->geometry().center() - frameGeometry().center());

    // MenuBar
    connect(ui->actionFile_Open, &QAction::triggered, this, &MainWindow::onAction);
    connect(ui->actionFile_Configurations, &QAction::triggered, this, &MainWindow::onAction);
    connect(ui->actionFile_Exit, &QAction::triggered, this, &MainWindow::onAction);
    connect(ui->actionRepo_Switch_manifest, &QAction::triggered, this, &MainWindow::onAction);
    connect(ui->actionRepo_Start, &QAction::triggered, this, &MainWindow::onAction);
    connect(ui->actionRepo_Sync, &QAction::triggered, this, &MainWindow::onAction);
    connect(ui->actionHelp_About, &QAction::triggered, this, &MainWindow::onAction);

    // ToolBar
    ui->toolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    QFrame *projectFrame = new QFrame(ui->toolBar);
    QLabel *projectLabel = new QLabel("Project:  ", projectFrame);
    m_projectListBox = new QComboBox(projectFrame);
    m_projectListBox->setFixedWidth(300);
    m_projectListBox->setEditable(true);
    m_projectListBox->setInsertPolicy(QComboBox::NoInsert);
    m_projectListBox->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_projectListBox->view()->setStyleSheet("QListView::item { height: 24px; }");
    m_projectListBox->installEventFilter(this);
    QCompleter *completer = new QCompleter(m_projectListBox);
    completer->setModel(m_projectListBox->model());
    completer->setFilterMode(Qt::MatchContains);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setMaxVisibleItems(10);
    completer->popup()->setStyleSheet("QListView::item { height: 24px; }");
    m_projectListBox->setCompleter(completer);
    connect(
        m_projectListBox, &QComboBox::currentIndexChanged, this, &MainWindow::onProjectSelected);

    QHBoxLayout *projectLayout = new QHBoxLayout(projectFrame);
    projectLayout->setContentsMargins(0, 0, 0, 0);
    projectLayout->setSpacing(0);
    projectLayout->addWidget(projectLabel);
    projectLayout->addWidget(m_projectListBox);
    ui->toolBar->addWidget(projectFrame);

    ui->toolBar->addSeparator();
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

    // Mode Buttons
    connect(ui->changesModeBtn, &QPushButton::toggled, this, &MainWindow::onChangeMode);
    connect(ui->historyModeBtn, &QPushButton::toggled, this, &MainWindow::onChangeMode);

    // Ref tree
    connect(ui->refTreeView, &QTreeView::clicked, this, &MainWindow::onRefClicked);
    connect(ui->refTreeView, &RefTreeView::requestRefreshEvent, this, [&](HistorySelectionArg arg) {
        refresh(arg);
    });

    m_changesPage = new ChangesPage(this);
    m_historyPage = new HistoryPage(this);
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

    // Bypass QComboBox default selection behavior during item insertions
    const int projectIndex = QSettings().value(getRepoSettingsKey("projectIndex")).toInt();

    // Init
    openRepo();

    if (QSettings().value(getRepoSettingsKey("modeIndex")).toInt()) {
        ui->historyModeBtn->setChecked(true);
    } else {
        ui->changesModeBtn->setChecked(true);
    }
    m_projectListBox->setCurrentIndex(projectIndex);
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_projectListBox) {
        if (event->type() == QEvent::FocusIn) {
            QTimer::singleShot(0, this, [this]() {
                m_projectListBox->lineEdit()->selectAll();
            });
        } else if (event->type() == QEvent::FocusOut) {
            QString currentProject = m_projectListBox->currentData(Qt::DisplayRole).toString();
            m_projectListBox->setCurrentText(currentProject);
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::onAction()
{
    QString path = getProjectFullPath(m_projectListBox->currentData().value<RepoProject>());
    QAction *action = qobject_cast<QAction *>(sender());
    if (action == ui->actionFile_Open) {
        onActionOpen();
    } else if (action == ui->actionFile_Exit) {
        qApp->quit();
    } else if (action == ui->actionFile_Configurations) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(QSettings().fileName()));
    } else if (action == ui->actionRepo_Sync) {
        RepoSyncDialog dialog(this);
        dialog.exec();
    } else if (action == ui->actionRepo_Start) {
        onActionRepoStart();
    } else if (action == ui->actionRepo_Switch_manifest) {
        onActionSwitchManifest();
    } else if (action == ui->actionHelp_About) {
        QMessageBox::about(this, "RepoMan", "Repo GUI front-end");
    } else if (action == m_actionPush) {
        PushDialog dialog(this, path);
        if (dialog.exec()) {
            refresh();
        }
    } else if (action == m_actionPull) {
        PullDialog dialog(this, path);
        if (dialog.exec()) {
            refresh();
        }
    } else if (action == m_actionFetch) {
        FetchDialog dialog(this, path);
        if (dialog.exec()) {
            refresh();
        }
    } else if (action == m_actionClean) {
        CleanDialog dialog(this, path);
        if (dialog.exec()) {
            refresh();
        }
    } else if (action == m_actionTerm) {
        QProcess p;
        p.setProgram("/usr/bin/x-terminal-emulator");
        p.setWorkingDirectory(path);
        p.startDetached();
    } else if (action == m_actionFolder) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    }
}

// void MainWindow::onProjectFilter(const QString &pattern)
//{
//     QList<QAction *> actions = m_ui->projectSearchEdit->actions();
//     if (!pattern.isEmpty()) {
//         if (actions.isEmpty()) {
//             QAction *action = m_ui->projectSearchEdit->addAction(
//                 style()->standardIcon(QStyle::SP_LineEditClearButton),
//                 QLineEdit::TrailingPosition);
//             connect(action, &QAction::triggered, this, [&]() {
//                 m_ui->projectSearchEdit->clear();
//             });
//         }
//         m_ui->projectListView->selectionModel()->clear();
//     } else {
//         if (actions.size() > 0) {
//             m_ui->projectSearchEdit->removeAction(actions.first());
//         }
//     }
// }

void MainWindow::onRefClicked(const QModelIndex &index)
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

void MainWindow::onActionSwitchManifest()
{
    QString path = QFileDialog::getOpenFileName(
        this, "Select manifest file", manifest.filePath, "Xmls (*.xml)");
    if (path.isEmpty()) {
        return;
    }
    QString cwd = QSettings().value("cwd").toString();
    QString manifestDir = QDir::cleanPath(cwd + "/.repo/manifests/");
    if (!path.startsWith(manifestDir)) {
        QMessageBox msgBox;
        msgBox.warning(this, "Error", "Manifest file does not belong to this repo");
        return;
    }
    QString arg = path.remove(0, manifestDir.length() + 1);
    QString cmd = QString("repo init -m %1").arg(arg);
    CmdDialog dialog(this, cmd, cwd);
    if (dialog.exec() == 0) {
        openRepo();
    }
}

void MainWindow::onActionRepoStart()
{
    QString targetManifest = QFileInfo(manifest.filePath).symLinkTarget();
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
            QString cwd = QSettings().value("cwd").toString();
            CmdDialog dialog(this, cmd, cwd);
            dialog.exec();
        }
    }
}

void MainWindow::onActionOpen()
{
    QString cwd = QSettings().value("cwd", QDir::homePath()).toString();
    QString path = QFileDialog::getExistingDirectory(this, "Open repo", cwd);
    if (path.isEmpty()) {
        return;
    }
    if (!QDir(QDir::cleanPath(path + "/.repo")).exists()) {
        QMessageBox msgBox;
        msgBox.warning(this, "Error", path + " is not a valid repo");
        return;
    }
    QSettings().setValue("cwd", path);
    openRepo();
}

void MainWindow::openRepo()
{
    QString cwd = QSettings().value("cwd").toString();
    if (cwd.isEmpty()) {
        return;
    }

    parseManifest(QDir::cleanPath(cwd + "/.repo/manifest.xml"));
    updateUI();
}

void MainWindow::onChangeMode()
{
    QPushButton *btn = static_cast<QPushButton *>(sender());
    if (!btn->isChecked()) return;

    ui->rightPanel->setCurrentWidget(
        btn == ui->changesModeBtn ? m_changesPage : (QWidget *)m_historyPage);

    QSettings().setValue(getRepoSettingsKey("modeIndex"), ui->rightPanel->currentIndex());

    QString projectPath = getProjectFullPath(m_projectListBox->currentData().value<RepoProject>());
    if (btn == ui->changesModeBtn) {
        m_changesPage->setCurrentProjectPath(projectPath);
    } else {
        m_historyPage->setCurrentProjectPath(projectPath);
    }
}

void MainWindow::onProjectSelected()
{
    QSettings().setValue(getRepoSettingsKey("projectIndex"), m_projectListBox->currentIndex());
    QString projectPath = getProjectFullPath(m_projectListBox->currentData().value<RepoProject>());
    m_changesPage->setCurrentProjectPath(projectPath);
    if (ui->rightPanel->currentWidget() == m_historyPage) {
        m_historyPage->setCurrentProjectPath(projectPath);
    }
    ui->refTreeView->setCurrentProjectPath(projectPath);
}

void MainWindow::refresh(const HistorySelectionArg &arg)
{
    ui->refTreeView->refresh();
    m_changesPage->refresh();
    m_historyPage->refresh(arg);
}

void MainWindow::updateUI()
{
    QString cwd = QSettings().value("cwd").toString();
    setWindowTitle(QString("%1 - %2").arg(cwd, QApplication::applicationName()));

    QString targetManifest = QFileInfo(manifest.filePath).symLinkTarget();
    m_statusLabel->setText(
        QString("  %1  |  %2").arg(QFileInfo(targetManifest).fileName(), manifest.revision));

    m_projectListBox->clear();
    for (int i = 0; i < manifest.projectList.size(); ++i) {
        const RepoProject &project = manifest.projectList[i];
        QString path = project.path.isEmpty() ? project.name : project.path;
        m_projectListBox->addItem(path, QVariant::fromValue(project));
        m_projectListBox->setItemData(i, path, Qt::ToolTipRole);
    }
}

QString MainWindow::getProjectFullPath(const RepoProject &project)
{
    QString path = project.path.isEmpty() ? project.name : project.path;
    QString cwd = QSettings().value("cwd").toString();
    return QDir::cleanPath(cwd + "/" + path);
}

void MainWindow::parseManifest(const QString &manPath)
{
    QFile manFile(manPath);
    QDomDocument doc("");
    if (!manFile.open(QIODevice::ReadOnly)) return;
    if (!doc.setContent(&manFile)) {
        manFile.close();
        return;
    }
    manFile.close();

    manifest = {};
    manifest.filePath = manPath;

    QDomNode n = doc.documentElement().firstChild();
    while (!n.isNull()) {
        QDomElement e = n.toElement();
        if (e.nodeName() == "include") {
            QString cwd = QSettings().value("cwd").toString();
            QString manPath2 = QDir::cleanPath(cwd + "/.repo/manifests/" + e.attribute("name"));
            parseManifest(manPath2);
        } else if (e.nodeName() == "default") {
            manifest.remote = e.attribute("remote");
            manifest.revision = e.attribute("revision");
            manifest.syncJ = e.attribute("sync-j").toInt();
        } else if (e.nodeName() == "project") {
            QString name = e.attribute("name");
            QString path = e.attribute("path");
            manifest.projectList.emplace_back(name, path);
            manifest.projectMap.insert(path, RepoProject(name, path));
        }
        n = n.nextSibling();
    }
    std::sort(manifest.projectList.begin(), manifest.projectList.end(),
        [](const RepoProject &p1, const RepoProject &p2) {
            return (p1.path.isEmpty() ? p1.name : p1.path) <
                   (p2.path.isEmpty() ? p2.name : p2.path);
        });
}
