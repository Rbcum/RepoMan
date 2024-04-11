#include "newtabpage.h"

#include <QPainter>
#include <QSortFilterProxyModel>
#include <QTimer>

#include "ui_newtabpage.h"

using namespace global;

NewTabPage::NewTabPage(const RepoContext &context)
    : QWidget(nullptr), ui(new Ui::NewTabPage), m_context(context)
{
    ui->setupUi(this);
    auto filterModel = new ProjectListFilterModel(this);
    filterModel->setSourceModel(new ProjectListModel(this, context));
    filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    ui->listView->setModel(filterModel);
    ui->listView->setItemDelegate(new ProjectListDelegate(this));
    connect(ui->listView, &QListView::doubleClicked, this, [this](const QModelIndex &index) {
        emit projectDoubleClicked(index.data(Qt::UserRole).value<Project>());
    });
    connect(ui->searchEdit, &QLineEdit::textChanged, filterModel,
        &QSortFilterProxyModel::setFilterFixedString);

    QTimer::singleShot(0, ui->searchEdit, SLOT(setFocus()));
}

NewTabPage::~NewTabPage()
{
    delete ui;
}

ProjectListModel::ProjectListModel(QObject *parent, const RepoContext &context)
    : QAbstractListModel(parent), m_context(context)
{
}

int ProjectListModel::rowCount(const QModelIndex &parent) const
{
    return m_context.manifest().projectList.size();
}

QVariant ProjectListModel::data(const QModelIndex &index, int role) const
{
    const Project &project = m_context.manifest().projectList[index.row()];
    if (role == Qt::DisplayRole) {  // Used when sorting
        return project.path;
    }
    if (role == Qt::UserRole) {
        return QVariant::fromValue(project);
    }

    return QVariant();
}

ProjectListDelegate::ProjectListDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
}

#define LINE_SPACING 1
#define PATH_FONT_SIZE 11
#define NAME_FONT_SIZE 10

void ProjectListDelegate::paint(
    QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QRect rect = option.rect;
    const QWidget *widget = option.widget;
    QStyle *style = widget ? widget->style() : QApplication::style();

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    // Selection
    opt.text = nullptr;
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter);

    // Text
    QFont font = painter->font();
    font.setPointSize(PATH_FONT_SIZE);
    const int pathHeight = QFontMetrics(font).height();
    font.setPointSize(NAME_FONT_SIZE);
    const int nameHeight = QFontMetrics(font).height();
    const int top = (rect.height() - pathHeight - LINE_SPACING - nameHeight) / 2;
    const Project &project = index.data(Qt::UserRole).value<Project>();

    // Name
    QPalette::ColorRole textRole =
        opt.state & QStyle::State_Selected ? QPalette::HighlightedText : QPalette::Text;
    font.setPointSize(PATH_FONT_SIZE);
    painter->setFont(font);
    style->drawItemText(painter, rect.adjusted(10, top, -10, 0), Qt::AlignTop, opt.palette, true,
        project.path, textRole);

    // Path
    textRole = opt.state & QStyle::State_Selected ? QPalette::HighlightedText : QPalette::Dark;
    font.setPointSize(NAME_FONT_SIZE);
    painter->setFont(font);
    style->drawItemText(painter, rect.adjusted(10, top + pathHeight + LINE_SPACING, -10, 0),
        Qt::AlignTop, opt.palette, true, project.name, textRole);
}

QSize ProjectListDelegate::sizeHint(
    const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    return QSize(0, 40);
}

ProjectListFilterModel::ProjectListFilterModel(QObject *parent)
{
}

bool ProjectListFilterModel::filterAcceptsRow(
    int source_row, const QModelIndex &source_parent) const
{
    const QModelIndex &index = sourceModel()->index(source_row, 0, source_parent);
    const Project &project = index.data(Qt::UserRole).value<Project>();
    return project.name.contains(filterRegularExpression()) ||
           project.path.contains(filterRegularExpression());
}
