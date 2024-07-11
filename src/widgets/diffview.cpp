#include "diffview.h"

#include <QMenu>
#include <QPushButton>
#include <QSettings>
#include <QTextBlock>
#include <QTimer>

#include "themes/icon.h"
#include "themes/theme.h"
#include "ui_diffview.h"
#include "widgets/difftextedit.h"

#define SPLIT_ICON ":/icons/diff-split.png"

#define SETTINGS_DIFF_SPLIT_MODE "diffSplitMode"
#define SETTINGS_CTX_LINES_IDX "diffContextLinesIndex"

#define INDEX_UNIFIED 0
#define INDEX_SPLIT 1

using namespace utils;

DiffView::DiffView(QWidget *parent) : QWidget(parent), ui(new Ui::DiffView)
{
    ui->setupUi(this);

    ui->splitBtn->setIcon(Icon({{SPLIT_ICON, Theme::IconsBaseColor}}, Icon::Tint).icon());
    ui->splitBtn->setStyleSheet("QToolButton:checked:!hover {border: none}");
    ui->splitBtn->setChecked(QSettings().value(SETTINGS_DIFF_SPLIT_MODE, false).toBool());
    connect(ui->splitBtn, &QToolButton::toggled, this, [&](bool checked) {
        saveScrollPosition();
        QSettings settings;
        settings.setValue(SETTINGS_DIFF_SPLIT_MODE, checked);
        updateUI();
    });

    ui->menuButton->setIcon(Icon({{":/icons/menu.png", Theme::IconsBaseColor}}, Icon::Tint).icon());
    ui->menuButton->setStyleSheet("QToolButton::menu-indicator {image: none}");
    auto menu = new QMenu(ui->menuButton);
    m_contextLinesGroup = new QActionGroup(menu);
    menu->addAction("Context lines")->setEnabled(false);
    menu->addAction("3", this, &DiffView::onMenuAction)->setActionGroup(m_contextLinesGroup);
    menu->addAction("10", this, &DiffView::onMenuAction)->setActionGroup(m_contextLinesGroup);
    menu->addAction("25", this, &DiffView::onMenuAction)->setActionGroup(m_contextLinesGroup);
    menu->addAction("50", this, &DiffView::onMenuAction)->setActionGroup(m_contextLinesGroup);
    menu->addAction("75", this, &DiffView::onMenuAction)->setActionGroup(m_contextLinesGroup);
    menu->addAction("100", this, &DiffView::onMenuAction)->setActionGroup(m_contextLinesGroup);
    for (auto action : m_contextLinesGroup->actions()) {
        action->setCheckable(true);
    }
    m_contextLinesGroup->actions()
        .at(QSettings().value(SETTINGS_CTX_LINES_IDX, 0).toInt())
        ->setChecked(true);
    ui->menuButton->setMenu(menu);

    ui->diffTextEdit->setMode(Unified);
    ui->leftDiffTextEdit->setMode(SplitOld);
    ui->leftDiffTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->rightDiffTextEdit->setMode(SplitNew);
    syncScrollBar(
        ui->leftDiffTextEdit->verticalScrollBar(), ui->rightDiffTextEdit->verticalScrollBar());
    syncScrollBar(
        ui->leftDiffTextEdit->horizontalScrollBar(), ui->rightDiffTextEdit->horizontalScrollBar());
}

DiffView::~DiffView()
{
    delete ui;
}

void DiffView::setDiffHunks(const GitFile &file, const QList<DiffHunk> &hunks)
{
    m_file = file;
    m_hunks = hunks;
    updateUI();
}

int DiffView::getContextLines()
{
    return m_contextLinesGroup->checkedAction()->text().toInt();
}

void DiffView::updateUI()
{
    m_updatingUI = true;

    ui->fileLabel->setText(m_file.path);
    if (ui->splitBtn->isChecked()) {
        ui->splitBtn->setIcon(Icon({{SPLIT_ICON, Theme::PaletteHighlight}}, Icon::Tint).icon());
        ui->stackedWidget->setCurrentIndex(INDEX_SPLIT);
        ui->leftDiffTextEdit->setDiffHunks(m_hunks);
        ui->rightDiffTextEdit->setDiffHunks(m_hunks);
    } else {
        ui->splitBtn->setIcon(Icon({{SPLIT_ICON, Theme::IconsBaseColor}}, Icon::Tint).icon());
        ui->stackedWidget->setCurrentIndex(INDEX_UNIFIED);
        ui->diffTextEdit->setDiffHunks(m_hunks);
    }
    restoreScrollPosition();

    m_updatingUI = false;
}

void DiffView::reset()
{
    m_file = {};
    m_hunks = {};
    ui->fileLabel->clear();
    ui->diffTextEdit->reset();
    ui->leftDiffTextEdit->reset();
    ui->rightDiffTextEdit->reset();
}

void DiffView::onMenuAction()
{
    QAction *action = qobject_cast<QAction *>(sender());
    int contextLinesIndex = m_contextLinesGroup->actions().indexOf(action);
    if (contextLinesIndex >= 0) {
        saveScrollPosition();
        QSettings settings;
        settings.setValue(SETTINGS_CTX_LINES_IDX, contextLinesIndex);
        emit diffParametersChanged();
    }
}

void DiffView::saveScrollPosition()
{
    m_scrollPosition = PositionInfo();

    bool isSplit = ui->stackedWidget->currentIndex() == INDEX_SPLIT;
    DiffTextEdit *textEdit = isSplit ? ui->leftDiffTextEdit : ui->diffTextEdit;
    int topBlockNum = textEdit->firstVisibleBlockNumber();
    int hunkIndex;
    int lineIndex;
    findTargetHunk(isSplit ? SplitOld : Unified, m_hunks, topBlockNum, hunkIndex, lineIndex);
    const DiffHunk &hunk = m_hunks[hunkIndex];
    const QList<int> &oldLNs = isSplit ? hunk.splitOldLNs : hunk.unifiedOldLNs;
    const QList<int> &newLNs = isSplit ? hunk.splitNewLNs : hunk.unifiedNewLNs;

    while (!oldLNs[lineIndex] && !newLNs[lineIndex]) {
        m_scrollPosition->offset++;
        lineIndex++;
    }
    m_scrollPosition->topLN.first = oldLNs[lineIndex];
    m_scrollPosition->topLN.second = newLNs[lineIndex];
    qDebug() << m_scrollPosition->topLN << m_scrollPosition->offset;
}

void DiffView::restoreScrollPosition()
{
    if (!m_scrollPosition) return;

    bool isSplit = ui->stackedWidget->currentIndex() == INDEX_SPLIT;
    int blockNumber = 0;
    for (auto &hunk : m_hunks) {
        const QList<int> &oldLNs = isSplit ? hunk.splitOldLNs : hunk.unifiedOldLNs;
        const QList<int> &newLNs = isSplit ? hunk.splitNewLNs : hunk.unifiedNewLNs;
        if ((hunk.oldStart <= m_scrollPosition->topLN.first &&
                m_scrollPosition->topLN.first < hunk.oldStart + hunk.oldTotal) ||
            (hunk.newStart <= m_scrollPosition->topLN.second &&
                m_scrollPosition->topLN.second < hunk.newStart + hunk.newTotal)) {
            for (int i = 0;; ++i, ++blockNumber) {
                if ((oldLNs[i] && oldLNs[i] == m_scrollPosition->topLN.first) ||
                    (newLNs[i] && newLNs[i] == m_scrollPosition->topLN.second)) {
                    goto out;
                }
            }
        } else {
            blockNumber += oldLNs.size();
        }
    }
out:
    blockNumber -= m_scrollPosition->offset;

    DiffTextEdit *textEdit = isSplit ? ui->leftDiffTextEdit : ui->diffTextEdit;
    QTimer::singleShot(0, this, [=]() {
        textEdit->verticalScrollBar()->setValue(blockNumber);
    });

    m_scrollPosition.reset();
}

void DiffView::syncScrollBar(QScrollBar *leftScrollBar, QScrollBar *rightScrollBar)
{
    connect(leftScrollBar, &QScrollBar::valueChanged, this, [=](int value) {
        if (!m_updatingUI) {
            rightScrollBar->setValue(value);
        }
    });
    connect(rightScrollBar, &QScrollBar::valueChanged, this, [=](int value) {
        if (!m_updatingUI) {
            leftScrollBar->setValue(value);
        }
    });
}
