#include "warningfilelistdialog.h"

#include <QScrollBar>

#include "ui_warningfilelistdialog.h"

WarningFileListDialog::WarningFileListDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::WarningFileListDialog)
{
    ui->setupUi(this);
}

WarningFileListDialog::~WarningFileListDialog()
{
    delete ui;
}

bool WarningFileListDialog::confirm(
    QWidget *parent, const QString &title, const QString &msg, const QStringList &files)
{
    WarningFileListDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.ui->msgLabel->setText(msg);
    for (const QString &f : files) {
        dialog.ui->filesEdit->appendPlainText(f);
    }
    dialog.ui->filesEdit->verticalScrollBar()->setValue(0);
    return dialog.exec() == QDialog::Accepted;
}
