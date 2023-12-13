#ifndef SWITCHMANIFESTDIALOG_H
#define SWITCHMANIFESTDIALOG_H

#include <QDialog>
#include <QFileSystemModel>
#include "widgets/QProgressIndicator.h"

namespace Ui {
    class SwitchManifestDialog;
}

class SwitchManifestDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SwitchManifestDialog(QWidget *parent = nullptr);
    ~SwitchManifestDialog();

private slots:
    void onFileDoubleClicked(const QModelIndex &index);
    void onPathClicked(const QString &path);

public slots:
    void accept() override;

private:
    Ui::SwitchManifestDialog *ui;
    QProgressIndicator *m_indicator;

    QFileSystemModel *m_model;
};

#endif  // SWITCHMANIFESTDIALOG_H
