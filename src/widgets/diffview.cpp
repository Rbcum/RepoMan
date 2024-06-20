#include "diffview.h"

#include <QMenu>
#include <QPushButton>

#include "themes/icon.h"
#include "themes/theme.h"
#include "ui_diffview.h"
#include "widgets/difftextedit.h"

#define SPLIT_ICON ":/icons/diff-split.png"

using namespace utils;

DiffView::DiffView(QWidget *parent) : QWidget(parent), ui(new Ui::DiffView)
{
    ui->setupUi(this);

    ui->splitBtn->setIcon(Icon({{SPLIT_ICON, Theme::IconsBaseColor}}, Icon::Tint).icon());
    ui->splitBtn->setStyleSheet("QToolButton:checked:!hover {border: none}");
    connect(ui->splitBtn, &QToolButton::toggled, this, &DiffView::updateUI);

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
