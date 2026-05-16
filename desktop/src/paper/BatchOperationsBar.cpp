#include "paper/BatchOperationsBar.hpp"
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>

BatchOperationsBar::BatchOperationsBar(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    hide();
}

void BatchOperationsBar::setupUI() {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(8);

    setStyleSheet(
        "QWidget { background: palette(window); border-bottom: 1px solid palette(mid); }"
    );

    countLabel_ = new QLabel("0 selected");
    countLabel_->setStyleSheet("font-weight: bold; font-size: 12px; color: palette(text);");
    layout->addWidget(countLabel_);

    layout->addStretch();

    favBtn_ = new QPushButton("Favorite");
    favBtn_->setStyleSheet(
        "QPushButton { background: #eab308; color: white; border-radius: 4px; "
        "padding: 4px 12px; font-weight: bold; font-size: 11px; }"
        "QPushButton:hover { background: #ca8a04; }"
    );
    connect(favBtn_, &QPushButton::clicked, this, &BatchOperationsBar::onFavorite);
    layout->addWidget(favBtn_);

    unfavBtn_ = new QPushButton("Unfavorite");
    unfavBtn_->setStyleSheet(
        "QPushButton { background: palette(mid); color: palette(text); border-radius: 4px; "
        "padding: 4px 12px; font-size: 11px; }"
    );
    connect(unfavBtn_, &QPushButton::clicked, this, &BatchOperationsBar::onUnfavorite);
    layout->addWidget(unfavBtn_);

    exportBtn_ = new QPushButton("Export");
    exportBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; border-radius: 4px; "
        "padding: 4px 12px; font-weight: bold; font-size: 11px; }"
        "QPushButton:hover { background: #2563eb; }"
    );
    connect(exportBtn_, &QPushButton::clicked, this, &BatchOperationsBar::onExport);
    layout->addWidget(exportBtn_);

    tagBtn_ = new QPushButton("Add Tags");
    tagBtn_->setStyleSheet(
        "QPushButton { background: #8b5cf6; color: white; border-radius: 4px; "
        "padding: 4px 12px; font-size: 11px; }"
        "QPushButton:hover { background: #7c3aed; }"
    );
    connect(tagBtn_, &QPushButton::clicked, this, &BatchOperationsBar::onAddTags);
    layout->addWidget(tagBtn_);

    deleteBtn_ = new QPushButton("Delete");
    deleteBtn_->setStyleSheet(
        "QPushButton { background: #dc2626; color: white; border-radius: 4px; "
        "padding: 4px 12px; font-size: 11px; }"
        "QPushButton:hover { background: #b91c1c; }"
    );
    connect(deleteBtn_, &QPushButton::clicked, this, &BatchOperationsBar::onDelete);
    layout->addWidget(deleteBtn_);

    clearBtn_ = new QPushButton("X");
    clearBtn_->setFixedSize(24, 24);
    clearBtn_->setToolTip("Clear selection");
    clearBtn_->setStyleSheet(
        "QPushButton { border: 1px solid palette(mid); border-radius: 12px; "
        "font-weight: bold; font-size: 12px; }"
        "QPushButton:hover { background: palette(mid); }"
    );
    connect(clearBtn_, &QPushButton::clicked, this, &BatchOperationsBar::onClear);
    layout->addWidget(clearBtn_);
}

void BatchOperationsBar::setSelection(const QList<int>& paperIds) {
    selectedIds_ = paperIds;
    countLabel_->setText(QString("%1 selected").arg(selectedIds_.size()));
    show();
}

void BatchOperationsBar::clearSelection() {
    selectedIds_.clear();
    countLabel_->setText("0 selected");
    hide();
    emit selectionCleared();
}

void BatchOperationsBar::onFavorite() {
    if (!selectedIds_.isEmpty()) emit batchFavorite(selectedIds_, true);
}

void BatchOperationsBar::onUnfavorite() {
    if (!selectedIds_.isEmpty()) emit batchFavorite(selectedIds_, false);
}

void BatchOperationsBar::onExport() {
    if (!selectedIds_.isEmpty()) emit batchExport(selectedIds_);
}

void BatchOperationsBar::onDelete() {
    if (selectedIds_.isEmpty()) return;
    auto result = QMessageBox::question(this, "Delete",
        QString("Delete %1 papers?").arg(selectedIds_.size()));
    if (result == QMessageBox::Yes) {
        emit batchDelete(selectedIds_);
    }
}

void BatchOperationsBar::onAddTags() {
    if (selectedIds_.isEmpty()) return;
    bool ok;
    QString tagsStr = QInputDialog::getText(this, "Add Tags",
        "Enter tags (comma separated):", QLineEdit::Normal, "", &ok);
    if (!ok || tagsStr.trimmed().isEmpty()) return;

    QStringList tags = tagsStr.split(',', Qt::SkipEmptyParts);
    for (auto& t : tags) t = t.trimmed();
    emit batchAddTags(selectedIds_, tags);
}

void BatchOperationsBar::onClear() {
    clearSelection();
}
