#include "visualization/PaperComparisonMatrix.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>

PaperComparisonMatrix::PaperComparisonMatrix(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void PaperComparisonMatrix::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // Header
    auto* headerRow = new QHBoxLayout();
    auto* titleLabel = new QLabel("Paper Comparison Matrix");
    titleLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    headerRow->addWidget(titleLabel, 1);

    highlightBtn_ = new QPushButton("Highlight: ON");
    highlightBtn_->setCheckable(true);
    highlightBtn_->setChecked(true);
    highlightBtn_->setStyleSheet(
        "QPushButton:checked { background: #3b82f6; color: white; }"
    );
    connect(highlightBtn_, &QPushButton::toggled, this, &PaperComparisonMatrix::onHighlightToggled);
    headerRow->addWidget(highlightBtn_);

    exportBtn_ = new QPushButton("Export");
    connect(exportBtn_, &QPushButton::clicked, this, &PaperComparisonMatrix::onExport);
    headerRow->addWidget(exportBtn_);

    layout->addLayout(headerRow);

    statsLabel_ = new QLabel("No papers loaded");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);

    matrixTable_ = new QTableWidget();
    matrixTable_->setStyleSheet(
        "QTableWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QTableWidget::item { padding: 4px; }"
        "QHeaderView::section { background: #f1f5f9; font-weight: bold; padding: 4px; border: 1px solid #e2e8f0; }"
    );
    matrixTable_->horizontalHeader()->setStretchLastSection(true);
    matrixTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(matrixTable_, &QTableWidget::cellClicked, this, &PaperComparisonMatrix::onCellClicked);
    layout->addWidget(matrixTable_, 1);
}

void PaperComparisonMatrix::setPapers(const QList<Paper>& papers) {
    papers_ = papers.mid(0, maxPapers_);
    if (fields_.isEmpty()) {
        fields_ = {"Title", "Authors", "Year", "Journal", "DOI", "Source", "Abstract"};
    }
    buildMatrix();
}

QList<Paper> PaperComparisonMatrix::papers() const {
    return papers_;
}

void PaperComparisonMatrix::setComparisonFields(const QStringList& fields) {
    fields_ = fields;
    if (!papers_.isEmpty()) buildMatrix();
}

QStringList PaperComparisonMatrix::comparisonFields() const {
    return fields_;
}

void PaperComparisonMatrix::highlightDifferences(bool enable) {
    highlightEnabled_ = enable;
    if (!papers_.isEmpty()) applyHighlighting();
}

void PaperComparisonMatrix::onCellClicked(int row, int col) {
    if (row <= 0) return;
    int paperIdx = col - 1;
    if (paperIdx >= 0 && paperIdx < papers_.size()) {
        emit paperClicked(papers_[paperIdx].id);
    }
}

void PaperComparisonMatrix::onHighlightToggled(bool enabled) {
    highlightEnabled_ = enabled;
    highlightBtn_->setText(enabled ? "Highlight: ON" : "Highlight: OFF");
    applyHighlighting();
}

void PaperComparisonMatrix::onExport() {
    QString path = QFileDialog::getSaveFileName(this, "Export Matrix", "comparison.csv", "CSV (*.csv)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        // Header
        QStringList headers;
        headers << "Field";
        for (const auto& p : papers_) headers << p.title.left(40);
        out << headers.join(",") << "\n";

        // Rows
        for (int i = 0; i < fields_.size(); ++i) {
            QStringList row;
            row << fields_[i];
            for (const auto& p : papers_) {
                QString val;
                if (fields_[i] == "Title") val = p.title;
                else if (fields_[i] == "Authors") val = p.authors;
                else if (fields_[i] == "Year") val = p.year;
                else if (fields_[i] == "Journal") val = p.journal;
                else if (fields_[i] == "DOI") val = p.doiUrl;
                else if (fields_[i] == "Source") val = p.source;
                else if (fields_[i] == "Abstract") val = p.abstract.left(100);
                row << QString("\"%1\"").arg(val.replace("\"", "\"\""));
            }
            out << row.join(",") << "\n";
        }
        emit exportRequested("csv");
    }
}

void PaperComparisonMatrix::buildMatrix() {
    int cols = papers_.size() + 1;
    int rows = fields_.size() + 1;

    matrixTable_->setRowCount(rows);
    matrixTable_->setColumnCount(cols);

    // Top-left corner
    matrixTable_->setItem(0, 0, new QTableWidgetItem(""));

    // Column headers (paper titles)
    for (int i = 0; i < papers_.size(); ++i) {
        auto* item = new QTableWidgetItem(papers_[i].title.left(30));
        item->setToolTip(papers_[i].title);
        QFont font;
        font.setBold(true);
        item->setFont(font);
        matrixTable_->setItem(0, i + 1, item);
        matrixTable_->setColumnWidth(i + 1, 180);
    }

    // Row headers + data
    for (int r = 0; r < fields_.size(); ++r) {
        auto* fieldItem = new QTableWidgetItem(fields_[r]);
        QFont font;
        font.setBold(true);
        fieldItem->setFont(font);
        fieldItem->setBackground(QColor(241, 245, 249));
        matrixTable_->setItem(r + 1, 0, fieldItem);

        for (int i = 0; i < papers_.size(); ++i) {
            QString val;
            const auto& p = papers_[i];
            if (fields_[r] == "Title") val = p.title;
            else if (fields_[r] == "Authors") val = p.authors;
            else if (fields_[r] == "Year") val = p.year;
            else if (fields_[r] == "Journal") val = p.journal;
            else if (fields_[r] == "DOI") val = p.doiUrl;
            else if (fields_[r] == "Source") val = p.source;
            else if (fields_[r] == "Abstract") val = p.abstract.left(100);

            matrixTable_->setItem(r + 1, i + 1, new QTableWidgetItem(val));
        }
    }

    matrixTable_->setColumnWidth(0, 100);
    statsLabel_->setText(QString("Comparing %1 papers across %2 fields")
        .arg(papers_.size()).arg(fields_.size()));

    applyHighlighting();
}

void PaperComparisonMatrix::applyHighlighting() {
    if (!highlightEnabled_) {
        // Reset all backgrounds
        for (int r = 1; r < matrixTable_->rowCount(); ++r) {
            for (int c = 1; c < matrixTable_->columnCount(); ++c) {
                auto* item = matrixTable_->item(r, c);
                if (item) item->setBackground(Qt::white);
            }
        }
        return;
    }

    // Highlight cells that differ from other papers in same row
    for (int r = 1; r < matrixTable_->rowCount(); ++r) {
        QSet<QString> values;
        for (int c = 1; c < matrixTable_->columnCount(); ++c) {
            auto* item = matrixTable_->item(r, c);
            if (item) values.insert(item->text().trimmed().toLower());
        }

        bool allSame = (values.size() <= 1);
        for (int c = 1; c < matrixTable_->columnCount(); ++c) {
            auto* item = matrixTable_->item(r, c);
            if (!item) continue;
            if (allSame) {
                item->setBackground(Qt::white);
            } else {
                // Check if this value is unique
                QString val = item->text().trimmed().toLower();
                bool isUnique = false;
                int sameCount = 0;
                for (int c2 = 1; c2 < matrixTable_->columnCount(); ++c2) {
                    auto* other = matrixTable_->item(r, c2);
                    if (other && other->text().trimmed().toLower() == val) sameCount++;
                }
                if (sameCount == 1) {
                    item->setBackground(QColor(254, 243, 199)); // yellow
                } else {
                    item->setBackground(QColor(219, 234, 254)); // light blue
                }
            }
        }
    }
}
