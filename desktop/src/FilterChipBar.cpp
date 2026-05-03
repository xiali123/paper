#include "FilterChipBar.hpp"
#include <QScrollArea>

FilterChipBar::FilterChipBar(QWidget* parent)
    : QFrame(parent)
{
    setStyleSheet(
        "QFrame { background: palette(window); border: 1px solid palette(mid); "
        "border-radius: 8px; padding: 4px; }"
    );

    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(8, 4, 8, 4);
    mainLayout->setSpacing(6);

    auto* label = new QLabel("Filters:");
    label->setStyleSheet("font-weight: bold; font-size: 12px;");
    mainLayout->addWidget(label);

    auto* scrollArea = new QScrollArea();
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet("QScrollArea { border: none; background: transparent; }");
    scrollArea->setFixedHeight(36);

    auto* scrollWidget = new QWidget();
    chipLayout_ = new QHBoxLayout(scrollWidget);
    chipLayout_->setContentsMargins(0, 0, 0, 0);
    chipLayout_->setSpacing(4);
    chipLayout_->addStretch();
    scrollArea->setWidget(scrollWidget);
    scrollArea->setWidgetResizable(true);

    mainLayout->addWidget(scrollArea, 1);

    auto* clearBtn = new QPushButton("Clear All");
    clearBtn->setStyleSheet(
        "QPushButton { background: transparent; color: #dc2626; border: none; "
        "font-size: 11px; text-decoration: underline; }"
    );
    connect(clearBtn, &QPushButton::clicked, this, [this]() {
        for (auto& chip : chips_) chip.active = false;
        rebuildChips();
    });
    mainLayout->addWidget(clearBtn);
}

void FilterChipBar::addChip(const QString& key, const QString& label, const QString& value) {
    for (auto& chip : chips_) {
        if (chip.key == key && chip.value == value) {
            chip.active = true;
            rebuildChips();
            return;
        }
    }

    FilterChip chip;
    chip.key = key;
    chip.label = label;
    chip.value = value;
    chip.active = true;
    chips_.append(chip);
    rebuildChips();
    emit chipAdded(key, value);
}

void FilterChipBar::removeChip(const QString& key, const QString& value) {
    for (int i = 0; i < chips_.size(); ++i) {
        if (chips_[i].key == key && chips_[i].value == value) {
            chips_[i].active = false;
            rebuildChips();
            emit chipRemoved(key, value);
            return;
        }
    }
}

void FilterChipBar::clearChips() {
    chips_.clear();
    rebuildChips();
}

void FilterChipBar::setAvailableFilters(const QMap<QString, QStringList>& filters) {
    availableFilters_ = filters;
    chips_.clear();
    for (auto it = filters.constBegin(); it != filters.constEnd(); ++it) {
        for (const auto& val : it.value()) {
            FilterChip chip;
            chip.key = it.key();
            chip.label = it.key() + ": " + val;
            chip.value = val;
            chip.active = false;
            chips_.append(chip);
        }
    }
    rebuildChips();
}

QStringList FilterChipBar::activeFilters(const QString& key) const {
    QStringList result;
    for (const auto& chip : chips_) {
        if (chip.key == key && chip.active) result << chip.value;
    }
    return result;
}

QMap<QString, QStringList> FilterChipBar::allActiveFilters() const {
    QMap<QString, QStringList> result;
    for (const auto& chip : chips_) {
        if (chip.active) result[chip.key] << chip.value;
    }
    return result;
}

void FilterChipBar::rebuildChips() {
    while (chipLayout_->count() > 1) {
        auto* item = chipLayout_->takeAt(0);
        delete item->widget();
        delete item;
    }

    for (const auto& chip : chips_) {
        auto* btn = new QPushButton(chip.active ? chip.label : chip.label);
        QString bg = chip.active ? "#3b82f6" : "palette(base)";
        QString fg = chip.active ? "white" : "palette(text)";
        QString border = chip.active ? "#2563eb" : "palette(mid)";
        btn->setStyleSheet(QString(
            "QPushButton { background: %1; color: %2; border: 1px solid %3; "
            "border-radius: 12px; padding: 2px 10px; font-size: 11px; }"
            "QPushButton:hover { opacity: 0.9; }"
        ).arg(bg, fg, border));
        btn->setCheckable(true);
        btn->setChecked(chip.active);

        connect(btn, &QPushButton::toggled, this,
                [this, key = chip.key, value = chip.value](bool checked) {
            onChipToggle(key, value, checked);
        });

        chipLayout_->insertWidget(chipLayout_->count() - 1, btn);
    }
}

void FilterChipBar::onChipToggle(const QString& key, const QString& value, bool active) {
    for (auto& chip : chips_) {
        if (chip.key == key && chip.value == value) {
            chip.active = active;
            break;
        }
    }
    emit filterChanged(allActiveFilters());
}
