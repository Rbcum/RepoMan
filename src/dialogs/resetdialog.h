#ifndef RESETDIALOG_H
#define RESETDIALOG_H

#include <QDialog>

#include "global.h"
#include "widgets/QProgressIndicator.h"

namespace Ui {
    class ResetDialog;
}

class ResetDialog : public QDialog
{
    Q_OBJECT

public:
    ResetDialog(QWidget *parent, const QString &projectPath, const Commit &commit = Commit());
    ~ResetDialog();

public slots:
    void accept() override;

private:
    Ui::ResetDialog *ui;
    QProgressIndicator *m_indicator;

    QString m_projectPath;
    Commit m_commit;
};

#endif  // RESETDIALOG_H
