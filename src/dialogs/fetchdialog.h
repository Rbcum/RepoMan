#ifndef FETCHDIALOG_H
#define FETCHDIALOG_H

#include <QDialog>

namespace Ui {
    class FetchDialog;
}

class FetchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FetchDialog(QWidget *parent, const QString &projectPath);
    ~FetchDialog();

public slots:
    void accept() override;

private:
    Ui::FetchDialog *ui;
    QString m_projectPath;
};

#endif  // FETCHDIALOG_H
