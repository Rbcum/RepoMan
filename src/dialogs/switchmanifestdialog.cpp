#include "switchmanifestdialog.h"

#include <QScrollBar>
#include <QSettings>
#include <QTimer>
#include <QtConcurrent>

#include "cmddialog.h"
#include "global.h"
#include "reposyncdialog.h"
#include "ui_switchmanifestdialog.h"

using namespace global;

SwitchManifestDialog::SwitchManifestDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::SwitchManifestDialog)
{
    ui->setupUi(this);
    m_indicator = new QProgressIndicator(this);

    QString cwd = QSettings().value("cwd").toString();
    QString manPath = QFileInfo(manifest.filePath).symLinkTarget();
    QString manDir = QFileInfo(manPath).absoluteDir().path();

    m_model = new QFileSystemModel(this);
    m_model->setRootPath(cwd + "/.repo/manifests");
    ui->listView->setModel(m_model);
    ui->listView->setRootIndex(m_model->index(manDir));
    ui->pathFinder->setHiddenParent(cwd + "/.repo");
    ui->pathFinder->setPath(manDir);

    connect(
        ui->listView, &QListView::doubleClicked, this, &SwitchManifestDialog::onFileDoubleClicked);
    connect(
        ui->pathFinder, &PathFinderView::pathClicked, this, &SwitchManifestDialog::onPathClicked);

    setEnabled(false);
    m_indicator->startHint();
    QtConcurrent::run([=]() {
        global::getCmdCode("git pull", cwd + "/.repo/manifests");
    }).then(this, [=]() {
        setEnabled(true);
        m_indicator->stopHint();
        ui->listView->scrollTo(m_model->index(manPath), QAbstractItemView::PositionAtCenter);
        ui->listView->setCurrentIndex(m_model->index(manPath));
    });
}

SwitchManifestDialog::~SwitchManifestDialog()
{
    delete ui;
}

void SwitchManifestDialog::onFileDoubleClicked(const QModelIndex &index)
{
    QString path = m_model->data(index, QFileSystemModel::FilePathRole).toString();
    QFileInfo fileInfo(path);
    if (fileInfo.isDir()) {
        ui->pathFinder->setPath(path);
        ui->listView->setRootIndex(m_model->index(path));
    } else if (fileInfo.isFile()) {
        accept();
    }
}

void SwitchManifestDialog::onPathClicked(const QString &path)
{
    ui->listView->setRootIndex(m_model->index(path));
}

void SwitchManifestDialog::accept()
{
    if (!ui->listView->selectionModel()->hasSelection()) return;
    QString path = ui->listView->selectionModel()
                       ->selectedRows()
                       .first()
                       .data(QFileSystemModel::FilePathRole)
                       .toString();
    if (!QFileInfo(path).isFile()) return;

    QString cwd = QSettings().value("cwd").toString();
    QString manifestDir = QDir::cleanPath(cwd + "/.repo/manifests/");
    QString arg = path.remove(0, manifestDir.length() + 1);
    QString cmd = QString("repo init -m %1").arg(arg);
    int code = CmdDialog::execute(this, cmd, cwd);
    done(code == 0 ? QDialog::Accepted : QDialog::Rejected);

    if (ui->syncSwitch->isChecked()) {
        RepoSyncDialog dialog(parentWidget());
        dialog.exec();
    }
}
