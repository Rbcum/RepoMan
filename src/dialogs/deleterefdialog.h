#ifndef DELETEREFDIALOG_H
#define DELETEREFDIALOG_H

#include <QDialog>

namespace Ui {
    class DeleteLocalBranchDialog;
    class DeleteRemoteBranchDialog;
    class DeleteTagDialog;
}  // namespace Ui

class DeleteLocalBranchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DeleteLocalBranchDialog(
        QWidget *parent, const QString &projectPath, const QString &branch);
    ~DeleteLocalBranchDialog();

public slots:
    void accept() override;

private:
    Ui::DeleteLocalBranchDialog *ui;
    QString m_projectPath;
    QString m_branch;
};

class DeleteRemoteBranchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DeleteRemoteBranchDialog(
        QWidget *parent, const QString &projectPath, const QString &branch);
    ~DeleteRemoteBranchDialog();

public slots:
    void accept() override;

private:
    Ui::DeleteRemoteBranchDialog *ui;
    QString m_projectPath;
    QString m_branch;
};

class DeleteTagDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DeleteTagDialog(QWidget *parent, const QString &projectPath, const QString &tag);
    ~DeleteTagDialog();

public slots:
    void accept() override;

private:
    Ui::DeleteTagDialog *ui;
    QString m_projectPath;
    QString m_tag;
};

#endif  // DELETEREFDIALOG_H
