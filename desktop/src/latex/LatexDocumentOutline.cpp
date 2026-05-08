#include "latex/LatexDocumentOutline.hpp"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QRegularExpression>

LatexDocumentOutline::LatexDocumentOutline(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void LatexDocumentOutline::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    outlineTree_ = new QTreeWidget();
    outlineTree_->setHeaderLabel("Document Outline");
    outlineTree_->header()->setStretchLastSection(true);
    outlineTree_->setRootIsDecorated(true);
    outlineTree_->setIndentation(16);
    outlineTree_->setMaximumWidth(220);
    connect(outlineTree_, &QTreeWidget::itemClicked, this, &LatexDocumentOutline::onItemClicked);
    layout->addWidget(outlineTree_);

    emptyLabel_ = new QLabel("No sections found");
    emptyLabel_->setAlignment(Qt::AlignCenter);
    emptyLabel_->setStyleSheet("color: palette(mid); font-size: 11px; padding: 20px;");
    layout->addWidget(emptyLabel_);
    emptyLabel_->hide();
}

void LatexDocumentOutline::parseDocument(const QString& content) {
    items_.clear();
    outlineTree_->clear();

    QRegularExpression sectionRe(
        "\\\\(part|chapter|section|subsection|subsubsection|paragraph)\\*?\\{([^}]*)\\}"
    );

    auto iter = sectionRe.globalMatch(content);
    while (iter.hasNext()) {
        auto match = iter.next();
        QString cmd = match.captured(1);
        QString title = match.captured(2);

        int level = 5;
        if (cmd == "part") level = 1;
        else if (cmd == "chapter") level = 2;
        else if (cmd == "section") level = 3;
        else if (cmd == "subsection") level = 4;
        else if (cmd == "subsubsection") level = 5;
        else level = 6;

        // Calculate line number
        int pos = match.capturedStart();
        int line = content.left(pos).count('\n') + 1;

        items_.append({level, title, line});
    }

    if (items_.isEmpty()) {
        emptyLabel_->show();
        outlineTree_->hide();
        return;
    }

    emptyLabel_->hide();
    outlineTree_->show();

    // Build tree with proper nesting
    QTreeWidgetItem* lastItems[7] = {nullptr}; // index by level
    QTreeWidgetItem* root = outlineTree_->invisibleRootItem();

    for (const auto& item : items_) {
        auto* treeItem = new QTreeWidgetItem({item.title});
        treeItem->setData(0, Qt::UserRole, item.line);
        treeItem->setToolTip(0, QString("Line %1").arg(item.line));

        // Set icon based on level
        QString prefix;
        switch (item.level) {
            case 1: prefix = "P "; break;
            case 2: prefix = "C "; break;
            case 3: prefix = "S "; break;
            case 4: prefix = "SS "; break;
            case 5: prefix = "SSS "; break;
            default: prefix = "P "; break;
        }
        treeItem->setText(0, prefix + item.title);

        // Find parent: last item at a lower level
        QTreeWidgetItem* parent = root;
        for (int l = item.level - 1; l >= 1; --l) {
            if (lastItems[l]) {
                parent = lastItems[l];
                break;
            }
        }

        parent->addChild(treeItem);
        lastItems[item.level] = treeItem;

        // Clear deeper levels
        for (int l = item.level + 1; l <= 6; ++l) {
            lastItems[l] = nullptr;
        }
    }

    outlineTree_->expandAll();
}

void LatexDocumentOutline::clear() {
    items_.clear();
    outlineTree_->clear();
    emptyLabel_->show();
    outlineTree_->hide();
}

void LatexDocumentOutline::onItemClicked(QTreeWidgetItem* item, int) {
    if (!item) return;
    int line = item->data(0, Qt::UserRole).toInt();
    if (line > 0) {
        emit navigateToLine(line);
    }
}
