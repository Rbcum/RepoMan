#ifndef CLEANDIALOG_H
#define CLEANDIALOG_H

#include <QDialog>

#include "widgets/QProgressIndicator.h"

namespace Ui {
    class CleanDialog;
    class CleanConfirmDialog;
}  // namespace Ui

class CleanDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CleanDialog(QWidget *parent, const QString &projectPath);
    ~CleanDialog();

private:
    Ui::CleanDialog *ui;
    QProgressIndicator *m_indicator;

    QString m_projectPath;

public slots:
    void accept() override;
};

#endif  // CLEANDIALOG_H
