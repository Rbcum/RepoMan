#ifndef PULLDIALOG_H
#define PULLDIALOG_H

#include <QDialog>
#include <QMap>

#include "widgets/QProgressIndicator.h"

namespace Ui {
    class PullDialog;
}

class PullDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PullDialog(QWidget *parent, const QString &projectPath);
    ~PullDialog();
public slots:
    void accept() override;

private:
    struct InitData
    {
        QString head;
        QMap<QString, QStringList> remoteBranches;
    };
    Ui::PullDialog *ui;
    QProgressIndicator *m_indicator;

    QString m_projectPath;
    QMap<QString, QStringList> m_remoteBranches;

    void updateBranchUI();
};

#endif  // PULLDIALOG_H
