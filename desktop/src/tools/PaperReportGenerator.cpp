#include "tools/PaperReportGenerator.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSplitter>
#include <QFileDialog>
#include <QApplication>
#include <QDate>
#include <QTextStream>

PaperReportGenerator::PaperReportGenerator(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void PaperReportGenerator::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel("Format:"));
    formatCombo_ = new QComboBox();
    formatCombo_->addItems({"HTML", "Markdown", "Plain Text", "LaTeX"});
    connect(formatCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperReportGenerator::onFormatChanged);
    toolbar->addWidget(formatCombo_);

    toolbar->addWidget(new QLabel("Section:"));
    sectionCombo_ = new QComboBox();
    sectionCombo_->addItems({"All", "Summary Only", "Detailed", "With Abstracts", "Citation List"});
    toolbar->addWidget(sectionCombo_);

    generateBtn_ = new QPushButton("Generate Report");
    generateBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 6px 16px; "
        "border-radius: 6px; font-weight: bold; }"
    );
    connect(generateBtn_, &QPushButton::clicked, this, &PaperReportGenerator::onGenerate);
    toolbar->addWidget(generateBtn_);

    copyBtn_ = new QPushButton("Copy");
    connect(copyBtn_, &QPushButton::clicked, this, &PaperReportGenerator::onCopy);
    toolbar->addWidget(copyBtn_);

    exportBtn_ = new QPushButton("Export");
    connect(exportBtn_, &QPushButton::clicked, this, &PaperReportGenerator::onExport);
    toolbar->addWidget(exportBtn_);

    layout->addLayout(toolbar);

    auto* splitter = new QSplitter(Qt::Vertical);

    // Paper table
    paperTable_ = new QTableWidget();
    paperTable_->setColumnCount(5);
    paperTable_->setHorizontalHeaderLabels({"Title", "Authors", "Year", "Journal", "Rating"});
    paperTable_->horizontalHeader()->setStretchLastSection(true);
    paperTable_->setColumnWidth(0, 250);
    paperTable_->setColumnWidth(1, 150);
    paperTable_->setColumnWidth(2, 50);
    paperTable_->setColumnWidth(3, 120);
    paperTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    paperTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    splitter->addWidget(paperTable_);

    auto* btnRow = new QHBoxLayout();
    removeBtn_ = new QPushButton("Remove Selected");
    removeBtn_->setStyleSheet("color: #dc2626;");
    connect(removeBtn_, &QPushButton::clicked, this, &PaperReportGenerator::onRemovePaper);
    btnRow->addWidget(removeBtn_);
    btnRow->addStretch();

    statsLabel_ = new QLabel("0 papers");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    btnRow->addWidget(statsLabel_);

    auto* tableWidget = new QWidget();
    auto* tableLayout = new QVBoxLayout(tableWidget);
    tableLayout->setContentsMargins(0, 0, 0, 0);
    tableLayout->addWidget(paperTable_);
    tableLayout->addLayout(btnRow);
    splitter->addWidget(tableWidget);

    // Preview
    previewEdit_ = new QTextEdit();
    previewEdit_->setReadOnly(true);
    previewEdit_->setStyleSheet("QTextEdit { padding: 8px; }");
    splitter->addWidget(previewEdit_);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    layout->addWidget(splitter, 1);
}

void PaperReportGenerator::addPaper(const ReportPaper& paper) {
    for (const auto& p : papers_) if (p.id == paper.id) return;
    papers_.append(paper);
    refreshTable();
}

void PaperReportGenerator::addPapers(const QList<ReportPaper>& list) {
    for (const auto& p : list) addPaper(p);
}

void PaperReportGenerator::clearPapers() {
    papers_.clear();
    refreshTable();
    previewEdit_->clear();
}

QString PaperReportGenerator::generateReport() const {
    switch (currentFormat_) {
        case 0: return formatHTML();
        case 1: return formatMarkdown();
        case 2: return formatPlainText();
        case 3: return formatLatex();
    }
    return "";
}

int PaperReportGenerator::paperCount() const { return papers_.size(); }

void PaperReportGenerator::onGenerate() {
    if (papers_.isEmpty()) return;
    QString report = generateReport();
    previewEdit_->setHtml(report);
    statsLabel_->setText(QString("Generated: %1 papers, %2 format")
        .arg(papers_.size()).arg(formatCombo_->currentText()));
    emit reportGenerated(formatCombo_->currentText(), papers_.size());
}

void PaperReportGenerator::onCopy() {
    QString text = previewEdit_->toPlainText();
    if (!text.isEmpty()) QApplication::clipboard()->setText(text);
}

void PaperReportGenerator::onExport() {
    QString ext = (currentFormat_ == 0) ? "HTML (*.html)" :
                  (currentFormat_ == 3) ? "LaTeX (*.tex)" : "Text (*.txt)";
    QString path = QFileDialog::getSaveFileName(this, "Export Report", "report", ext);
    if (path.isEmpty()) return;

    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        if (currentFormat_ == 0)
            out << previewEdit_->toHtml();
        else
            out << previewEdit_->toPlainText();
    }
    emit reportExported(path);
}

void PaperReportGenerator::onFormatChanged(int index) { currentFormat_ = index; }

void PaperReportGenerator::onRemovePaper() {
    int row = paperTable_->currentRow();
    if (row < 0 || row >= papers_.size()) return;
    papers_.removeAt(row);
    refreshTable();
}

void PaperReportGenerator::refreshTable() {
    paperTable_->setRowCount(papers_.size());
    int row = 0;
    for (const auto& p : papers_) {
        paperTable_->setItem(row, 0, new QTableWidgetItem(p.title));
        paperTable_->setItem(row, 1, new QTableWidgetItem(p.authors.join(", ")));
        paperTable_->setItem(row, 2, new QTableWidgetItem(QString::number(p.year)));
        paperTable_->setItem(row, 3, new QTableWidgetItem(p.journal));

        QString stars;
        for (int i = 0; i < p.rating; ++i) stars += "*";
        auto* ratingItem = new QTableWidgetItem(stars);
        ratingItem->setForeground(QColor(245, 158, 11));
        paperTable_->setItem(row, 4, ratingItem);
        row++;
    }
    statsLabel_->setText(QString("%1 paper(s)").arg(papers_.size()));
}

QString PaperReportGenerator::formatHTML() const {
    QString html;
    html += "<h1>Paper Research Report</h1>";
    html += QString("<p>Generated: %1 | Papers: %2</p>").arg(QDate::currentDate().toString()).arg(papers_.size());
    html += "<hr>";

    // Summary table
    html += "<h2>Overview</h2>";
    html += "<table border='1' cellpadding='8' cellspacing='0' style='border-collapse:collapse'>";
    html += "<tr><th>#</th><th>Title</th><th>Authors</th><th>Year</th><th>Journal</th><th>Rating</th></tr>";
    for (int i = 0; i < papers_.size(); ++i) {
        const auto& p = papers_[i];
        html += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
            .arg(i + 1).arg(p.title).arg(p.authors.join(", "))
            .arg(p.year).arg(p.journal).arg(QString("*").repeated(p.rating));
    }
    html += "</table>";

    if (sectionCombo_->currentIndex() >= 2) {
        html += "<h2>Detailed Analysis</h2>";
        for (int i = 0; i < papers_.size(); ++i) {
            const auto& p = papers_[i];
            html += QString("<h3>%1. %2</h3>").arg(i + 1).arg(p.title);
            html += QString("<p><b>Authors:</b> %1</p>").arg(p.authors.join(", "));
            html += QString("<p><b>Journal:</b> %1 (%2)</p>").arg(p.journal).arg(p.year);
            if (!p.doi.isEmpty()) html += QString("<p><b>DOI:</b> %1</p>").arg(p.doi);
            if (sectionCombo_->currentIndex() >= 3 && !p.abstractText.isEmpty())
                html += QString("<p><b>Abstract:</b> %1</p>").arg(p.abstractText);
        }
    }

    // Year distribution
    QMap<int, int> yearDist;
    for (const auto& p : papers_) yearDist[p.year]++;
    html += "<h2>Year Distribution</h2><ul>";
    for (auto it = yearDist.begin(); it != yearDist.end(); ++it)
        html += QString("<li>%1: %2 paper(s)</li>").arg(it.key()).arg(it.value());
    html += "</ul>";

    return html;
}

QString PaperReportGenerator::formatMarkdown() const {
    QString md;
    md += "# Paper Research Report\n\n";
    md += QString("Generated: %1 | Papers: %2\n\n---\n\n").arg(QDate::currentDate().toString()).arg(papers_.size());

    md += "| # | Title | Authors | Year | Journal | Rating |\n";
    md += "|---|---|---|---|---|---|\n";
    for (int i = 0; i < papers_.size(); ++i) {
        const auto& p = papers_[i];
        md += QString("| %1 | %2 | %3 | %4 | %5 | %6 |\n")
            .arg(i + 1).arg(p.title).arg(p.authors.join(", "))
            .arg(p.year).arg(p.journal).arg(QString("*").repeated(p.rating));
    }

    if (sectionCombo_->currentIndex() >= 2) {
        md += "\n## Detailed Analysis\n\n";
        for (int i = 0; i < papers_.size(); ++i) {
            const auto& p = papers_[i];
            md += QString("### %1. %2\n\n").arg(i + 1).arg(p.title);
            md += QString("- **Authors:** %1\n").arg(p.authors.join(", "));
            md += QString("- **Journal:** %1 (%2)\n").arg(p.journal).arg(p.year);
            if (sectionCombo_->currentIndex() >= 3 && !p.abstractText.isEmpty())
                md += QString("\n> %1\n\n").arg(p.abstractText);
        }
    }

    return md;
}

QString PaperReportGenerator::formatPlainText() const {
    QString txt;
    txt += "PAPER RESEARCH REPORT\n";
    txt += QString("Generated: %1\n").arg(QDate::currentDate().toString());
    txt += QString("Total Papers: %1\n\n").arg(papers_.size());

    for (int i = 0; i < papers_.size(); ++i) {
        const auto& p = papers_[i];
        txt += QString("---\n[%1] %2\n").arg(i + 1).arg(p.title);
        txt += QString("    Authors: %1\n").arg(p.authors.join(", "));
        txt += QString("    Journal: %1 (%2)\n").arg(p.journal).arg(p.year);
        if (!p.doi.isEmpty()) txt += QString("    DOI: %1\n").arg(p.doi);
        if (sectionCombo_->currentIndex() >= 3 && !p.abstractText.isEmpty())
            txt += QString("    Abstract: %1\n").arg(p.abstractText.left(200));
    }
    return txt;
}

QString PaperReportGenerator::formatLatex() const {
    QString tex;
    tex += "\\documentclass{article}\n\\usepackage[utf8]{inputenc}\n\\begin{document}\n\n";
    tex += "\\title{Paper Research Report}\n\\date{" + QDate::currentDate().toString("yyyy-MM-dd") + "}\n\\maketitle\n\n";

    tex += "\\section{Overview}\n\\begin{itemize}\n";
    tex += QString("\\item Total papers: %1\n").arg(papers_.size());
    tex += "\\end{itemize}\n\n";

    tex += "\\section{Paper List}\n\\begin{enumerate}\n";
    for (const auto& p : papers_) {
        tex += QString("\\item \\textbf{%1} — %2 (%3, %4)\n")
            .arg(p.title).arg(p.authors.join(", ")).arg(p.journal).arg(p.year);
    }
    tex += "\\end{enumerate}\n\n";

    tex += "\\end{document}\n";
    return tex;
}
