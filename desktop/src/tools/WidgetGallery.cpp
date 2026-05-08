#include "tools/WidgetGallery.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>

WidgetGallery::WidgetGallery(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void WidgetGallery::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // Header
    auto* headerRow = new QHBoxLayout();
    auto* header = new QLabel("Widget Gallery");
    header->setStyleSheet("font-weight: bold; font-size: 14px;");
    headerRow->addWidget(header, 1);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItem("All Categories");
    connect(categoryCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &WidgetGallery::onCategoryChanged);
    headerRow->addWidget(categoryCombo_);

    layout->addLayout(headerRow);

    countLabel_ = new QLabel("0 widgets");
    countLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(countLabel_);

    // Scrollable grid
    scrollArea_ = new QScrollArea();
    scrollArea_->setWidgetResizable(true);
    scrollArea_->setStyleSheet("QScrollArea { border: none; }");
    scrollArea_->setWidget(new QWidget());
    layout->addWidget(scrollArea_, 1);
}

void WidgetGallery::addCategory(const QString& name) {
    if (!categories_.contains(name)) {
        categories_.append(name);
        categoryCombo_->addItem(name);
    }
}

void WidgetGallery::addWidget(const QString& category, const QString& name, QWidget* widget) {
    addCategory(category);
    GalleryEntry entry;
    entry.category = category;
    entry.name = name;
    entry.widget = widget;
    entries_.append(entry);
    rebuildGrid();
}

void WidgetGallery::addWidget(const QString& category, const QString& name, const QString& description) {
    addCategory(category);
    GalleryEntry entry;
    entry.category = category;
    entry.name = name;
    entry.description = description;
    entries_.append(entry);
    rebuildGrid();
}

void WidgetGallery::showCategory(const QString& name) {
    currentCategory_ = name;
    int idx = categoryCombo_->findText(name);
    if (idx >= 0) categoryCombo_->setCurrentIndex(idx);
    rebuildGrid();
}

void WidgetGallery::showAll() {
    currentCategory_.clear();
    categoryCombo_->setCurrentIndex(0);
    rebuildGrid();
}

void WidgetGallery::onCategoryChanged(int index) {
    currentCategory_ = (index <= 0) ? "" : categoryCombo_->itemText(index);
    rebuildGrid();
}

void WidgetGallery::rebuildGrid() {
    auto* container = new QWidget();
    auto* grid = new QGridLayout(container);
    grid->setSpacing(12);
    grid->setContentsMargins(16, 16, 16, 16);

    int col = 0, row = 0;
    const int maxCols = 3;
    int count = 0;

    for (const auto& entry : entries_) {
        if (!currentCategory_.isEmpty() && entry.category != currentCategory_) continue;

        auto* card = new QWidget();
        card->setStyleSheet(
            "QWidget { background: palette(base); border: 1px solid palette(mid); "
            "border-radius: 8px; }"
        );
        card->setCursor(Qt::PointingHandCursor);
        auto* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(12, 10, 12, 10);

        // Category badge
        auto* catLabel = new QLabel(entry.category);
        catLabel->setStyleSheet(
            "font-size: 9px; color: #3b82f6; font-weight: bold; "
            "background: #dbeafe; padding: 1px 6px; border-radius: 3px;"
        );
        cardLayout->addWidget(catLabel);

        // Name
        auto* nameLabel = new QLabel(entry.name);
        nameLabel->setStyleSheet("font-weight: bold; font-size: 13px;");
        cardLayout->addWidget(nameLabel);

        // Description or embedded widget
        if (entry.widget) {
            entry.widget->setParent(card);
            cardLayout->addWidget(entry.widget);
        } else if (!entry.description.isEmpty()) {
            auto* descLabel = new QLabel(entry.description.left(100));
            descLabel->setWordWrap(true);
            descLabel->setStyleSheet("font-size: 11px; color: #64748b;");
            cardLayout->addWidget(descLabel);
        }

        // Click handler
        connect(card, &QWidget::mousePressEvent, this,
                [this, cat = entry.category, n = entry.name]() {
            emit widgetSelected(cat, n);
        });

        grid->addWidget(card, row, col);
        col++;
        if (col >= maxCols) { col = 0; row++; }
        count++;
    }

    scrollArea_->setWidget(container);
    countLabel_->setText(QString("%1 widgets").arg(count));
}
