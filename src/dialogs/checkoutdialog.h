#ifndef CHECKOUTDIALOG_H
#define CHECKOUTDIALOG_H

#include <QDialog>

#include "global.h"

namespace Ui {
    class CheckoutDialog;
}

class CheckoutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CheckoutDialog(QWidget *parent, const QString &projectPath, const Commit &commit);
    ~CheckoutDialog();

public slots:
    void accept() override;

private:
    Ui::CheckoutDialog *ui;
    QString m_projectPath;
    Commit m_commit;
};

#endif  // CHECKOUTDIALOG_H
