#include "paper/PaperCompareDialog.hpp"
#include "core/PaperTypes.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSplitter>
#include <QScrollBar>
#include <QRegularExpression>

PaperCompareDialog::PaperCompareDialog(const Paper& left, const Paper& right, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QString("Compare: %1 vs %2")
        .arg(left.title.left(30), right.title.left(30)));
    resize(1000, 700);
    setupUI(left, right);
}

void PaperCompareDialog::setupUI(const Paper& left, const Paper& right) {
    auto* layout = new QVBoxLayout(this);

    // Headers
    auto* headerRow = new QHBoxLayout();

    auto* leftHeader = new QLabel(QString("<b>%1</b>").arg(left.title.left(60)));
    leftHeader->setWordWrap(true);
    leftHeader->setStyleSheet("padding: 8px; background: #eff6ff; border-radius: 6px;");
    headerRow->addWidget(leftHeader);

    auto* vsLabel = new QLabel("vs");
    vsLabel->setAlignment(Qt::AlignCenter);
    vsLabel->setStyleSheet("font-weight: bold; color: palette(mid); font-size: 16px;");
    vsLabel->setFixedWidth(40);
    headerRow->addWidget(vsLabel);

    auto* rightHeader = new QLabel(QString("<b>%1</b>").arg(right.title.left(60)));
    rightHeader->setWordWrap(true);
    rightHeader->setStyleSheet("padding: 8px; background: #f0fdf4; border-radius: 6px;");
    headerRow->addWidget(rightHeader);

    layout->addLayout(headerRow);

    // Comparison table
    auto* table = new QTableWidget();
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels({"Field", "Paper A", "Paper B"});
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->setStyleSheet(
        "QTableWidget { font-size: 12px; }"
        "QHeaderView::section { font-weight: bold; padding: 6px; }"
    );

    auto addRow = [&](const QString& field, const QString& lv, const QString& rv) {
        int row = table->rowCount();
        table->insertRow(row);

        auto* fieldItem = new QTableWidgetItem(field);
        QFont fieldFont = fieldItem->font();
        fieldFont.setBold(true);
        fieldItem->setFont(fieldFont);
        table->setItem(row, 0, fieldItem);

        auto* leftItem = new QTableWidgetItem(lv);
        leftItem->setToolTip(lv);
        table->setItem(row, 1, leftItem);

        auto* rightItem = new QTableWidgetItem(rv);
        rightItem->setToolTip(rv);
        table->setItem(row, 2, rightItem);

        // Highlight differences
        if (lv != rv && !lv.isEmpty() && !rv.isEmpty()) {
            QColor diffBg(255, 253, 208);
            leftItem->setBackground(diffBg);
            rightItem->setBackground(diffBg);
        }
    };

    addRow("Title", left.title, right.title);
    addRow("Authors", left.authors, right.authors);
    addRow("Year", left.year, right.year);
    addRow("Journal", left.journalFull.isEmpty() ? left.journal : left.journalFull,
                          right.journalFull.isEmpty() ? right.journal : right.journalFull);
    addRow("DOI", left.doiUrl, right.doiUrl);
    addRow("Type", left.type, right.type);
    addRow("Level", left.level, right.level);
    addRow("Citations", QString::number(left.citationCount), QString::number(right.citationCount));
    addRow("PDF URL", left.pdfUrl, right.pdfUrl);
    addRow("Keywords", left.keywords.join(", "), right.keywords.join(", "));
    addRow("Source", left.source, right.source);

    // Abstract comparison
    int row = table->rowCount();
    table->insertRow(row);
    auto* fieldItem = new QTableWidgetItem("Abstract");
    QFont fieldFont = fieldItem->font();
    fieldFont.setBold(true);
    fieldItem->setFont(fieldFont);
    table->setItem(row, 0, fieldItem);
    table->setItem(row, 1, new QTableWidgetItem(left.abstract));
    table->setItem(row, 2, new QTableWidgetItem(right.abstract));
    table->setRowHeight(row, 120);

    table->resizeColumnsToContents();
    table->resizeRowsToContents();
    layout->addWidget(table, 1);

    // Close
    auto* closeRow = new QHBoxLayout();
    closeRow->addStretch();
    auto* closeBtn = new QPushButton("Close");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);
    closeRow->addWidget(closeBtn);
    layout->addLayout(closeRow);
}
