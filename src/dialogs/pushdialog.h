#ifndef PUSHDIALOG_H
#define PUSHDIALOG_H

#include <QDialog>
#include <QSettings>

#include "widgets/QProgressIndicator.h"

namespace Ui {
    class PushDialog;
}

class PushDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PushDialog(QWidget *parent, const QString &projectPath);
    ~PushDialog();

public slots:
    void accept() override;

private slots:
    void updateUrlUI();
    void updateTargetBranchUI();

private:
    struct InitData
    {
        QString head;
        QStringList localBranches;
        QMap<QString, QStringList> remoteBranches;
    };

    Ui::PushDialog *ui;
    QProgressIndicator *m_indicator;
    QString m_projectPath;
    QMap<QString, QStringList> m_remoteBranches;
};

#endif  // PUSHDIALOG_H
