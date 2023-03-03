#ifndef QHISTORYTABLEVIEW_H
#define QHISTORYTABLEVIEW_H

#include <QLabel>
#include <QPushButton>
#include <QTableView>

#include "pages/historytablemodel.h"

class QHistoryTableView : public QTableView
{
    Q_OBJECT
public:
    static const QStringList HEADERS;

    QHistoryTableView(QWidget *parent = nullptr);

    bool eventFilter(QObject *watched, QEvent *event) override;
    bool event(QEvent *event) override;
    void setLoading(bool loading);
    void updateLoadingLabel(const HistorySelectionArg &arg, int count);

private:
    QFrame *m_loadingFrame;
    QLabel *m_loadingLabel;
    QPushButton *m_cancelButton;

    int m_loading;
    QList<int> m_colWidths;
    bool m_widthSet;

signals:
    void cancelLoading();
};

#endif  // QHISTORYTABLEVIEW_H
