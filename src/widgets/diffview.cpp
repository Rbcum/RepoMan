#include "diffview.h"

#include <QMenu>
#include <QPushButton>
#include <QSettings>
#include <QTimer>

#include "themes/icon.h"
#include "themes/theme.h"
#include "ui_diffview.h"
#include "widgets/difftextedit.h"

#define SPLIT_ICON ":/icons/diff-split.png"

#define SETTINGS_DIFF_SPLIT_MODE "diffSplitMode"
#define SETTINGS_CTX_LINES_IDX "diffContextLinesIndex"

using namespace utils;

DiffView::DiffView(QWidget *parent) : QWidget(parent), ui(new Ui::DiffView)
{
    ui->setupUi(this);

    ui->splitBtn->setIcon(Icon({{SPLIT_ICON, Theme::IconsBaseColor}}, Icon::Tint).icon());
    ui->splitBtn->setStyleSheet("QToolButton:checked:!hover {border: none}");
    ui->splitBtn->setChecked(QSettings().value(SETTINGS_DIFF_SPLIT_MODE, false).toBool());
    connect(ui->splitBtn, &QToolButton::toggled, this, [&](bool checked) {
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
        ui->stackedWidget->setCurrentIndex(1);
        ui->leftDiffTextEdit->setDiffHunks(m_hunks);
        ui->rightDiffTextEdit->setDiffHunks(m_hunks);
    } else {
        ui->splitBtn->setIcon(Icon({{SPLIT_ICON, Theme::IconsBaseColor}}, Icon::Tint).icon());
        ui->stackedWidget->setCurrentIndex(0);
        ui->diffTextEdit->setDiffHunks(m_hunks);
    }
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
        QSettings settings;
        settings.setValue(SETTINGS_CTX_LINES_IDX, contextLinesIndex);
        emit diffParametersChanged();
    }
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
