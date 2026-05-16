#include "visualization/PaperComparisonSlider.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>

PaperComparisonSlider::PaperComparisonSlider(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void PaperComparisonSlider::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Paper");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperComparisonSlider::onAddPaper);
    toolbar->addWidget(addBtn_);

    removeBtn_ = new QPushButton("Remove Last");
    removeBtn_->setStyleSheet("color: #dc2626;");
    connect(removeBtn_, &QPushButton::clicked, this, &PaperComparisonSlider::onRemovePaper);
    toolbar->addWidget(removeBtn_);

    toolbar->addStretch();
    layout->addLayout(toolbar);

    compareTable_ = new QTableWidget();
    compareTable_->setSelectionBehavior(QAbstractItemView::SelectItems);
    compareTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    compareTable_->setStyleSheet(
        "QTableWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QTableWidget::item { padding: 6px; }"
    );
    compareTable_->verticalHeader()->setDefaultSectionSize(32);
    connect(compareTable_, &QTableWidget::cellClicked, this, &PaperComparisonSlider::onCompareField);
    layout->addWidget(compareTable_, 1);

    infoLabel_ = new QLabel("Add 2+ papers to compare");
    infoLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(infoLabel_);

    fieldLabels_ = {"Title", "Authors", "Year", "Journal", "DOI", "Abstract", "Keywords",
                    "Citations", "Impact Factor", "Pages", "Venue", "Publisher"};
}

void PaperComparisonSlider::setPapers(const QList<ComparePaper>& papers) {
    papers_ = papers;
    refreshTable();
}

void PaperComparisonSlider::addPaper(const ComparePaper& paper) {
    papers_.append(paper);
    refreshTable();
}

void PaperComparisonSlider::clear() {
    papers_.clear();
    refreshTable();
}

int PaperComparisonSlider::paperCount() const { return papers_.size(); }

void PaperComparisonSlider::onAddPaper() {
    // Placeholder — typically called with real data from MainWindow
    infoLabel_->setText("Use this from main window with real paper data");
}

void PaperComparisonSlider::onRemovePaper() {
    if (papers_.isEmpty()) return;
    papers_.removeLast();
    refreshTable();
}

void PaperComparisonSlider::onCompareField(int row, int col) {
    Q_UNUSED(row); Q_UNUSED(col);
    if (row < fieldLabels_.size()) emit fieldCompared(fieldLabels_[row]);
}

void PaperComparisonSlider::refreshTable() {
    int n = papers_.size();
    if (n == 0) {
        compareTable_->setRowCount(0);
        compareTable_->setColumnCount(0);
        infoLabel_->setText("Add 2+ papers to compare");
        return;
    }

    compareTable_->setRowCount(fieldLabels_.size());
    compareTable_->setColumnCount(n + 1);
    compareTable_->setHorizontalHeaderItem(0, new QTableWidgetItem("Field"));
    for (int i = 0; i < n; ++i) {
        compareTable_->setHorizontalHeaderItem(i + 1,
            new QTableWidgetItem(QString("Paper %1").arg(i + 1)));
    }
    compareTable_->setColumnWidth(0, 100);

    for (int i = 0; i < fieldLabels_.size(); ++i) {
        auto* labelItem = new QTableWidgetItem(fieldLabels_[i]);
        QFont font = labelItem->font();
        font.setBold(true);
        labelItem->setFont(font);
        labelItem->setBackground(QColor(248, 250, 252));
        compareTable_->setItem(i, 0, labelItem);

        for (int j = 0; j < n; ++j) {
            const auto& p = papers_[j];
            QString value;
            switch (i) {
                case 0: value = p.title; break;
                case 1: value = p.authors; break;
                case 2: value = QString::number(p.year); break;
                case 3: value = p.journal; break;
                case 4: value = p.doi; break;
                case 5: value = p.abstractText.left(120); break;
                case 6: value = p.keywords; break;
                case 7: value = QString::number(p.citationCount); break;
                case 8: value = QString::number(p.impactFactor, 'f', 2); break;
                case 9: value = QString::number(p.pageCount); break;
                case 10: value = p.venue; break;
                case 11: value = p.publisher; break;
            }
            auto* item = new QTableWidgetItem(value);
            compareTable_->setItem(i, j + 1, item);
        }
    }

    highlightDifferences();
    infoLabel_->setText(QString("Comparing %1 paper(s) across %2 fields").arg(n).arg(fieldLabels_.size()));
}

void PaperComparisonSlider::highlightDifferences() {
    if (papers_.size() < 2) return;

    for (int row = 0; row < fieldLabels_.size(); ++row) {
        QStringList values;
        for (int col = 0; col < papers_.size(); ++col) {
            auto* item = compareTable_->item(row, col + 1);
            if (item) values << item->text().toLower().trimmed();
        }

        bool allSame = true;
        QString first = values.value(0);
        for (int i = 1; i < values.size(); ++i) {
            if (values[i] != first) { allSame = false; break; }
        }

        for (int col = 0; col < papers_.size(); ++col) {
            auto* item = compareTable_->item(row, col + 1);
            if (!item) continue;
            if (allSame) {
                item->setBackground(QColor(240, 253, 244));
            } else {
                item->setBackground(QColor(254, 243, 199));
            }
        }
    }
}
