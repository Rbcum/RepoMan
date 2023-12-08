#ifndef WARNINGFILELISTDIALOG_H
#define WARNINGFILELISTDIALOG_H

#include <QDialog>

namespace Ui {
    class WarningFileListDialog;
}

class WarningFileListDialog : public QDialog
{
    Q_OBJECT

public:
    explicit WarningFileListDialog(QWidget *parent = nullptr);
    ~WarningFileListDialog();
    static bool confirm(
        QWidget *parent, const QString &title, const QString &msg, const QStringList &files);

private:
    Ui::WarningFileListDialog *ui;
};

#endif  // WARNINGFILELISTDIALOG_H
