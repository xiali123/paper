#include "citation/BibliographyBuilderWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QFileDialog>
#include <QApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

BibliographyBuilderWidget::BibliographyBuilderWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void BibliographyBuilderWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    formatCombo_ = new QComboBox();
    formatCombo_->addItems({"APA 7th", "MLA 9th", "Chicago", "IEEE", "BibTeX", "Vancouver"});
    connect(formatCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BibliographyBuilderWidget::onFormatChanged);
    toolbar->addWidget(new QLabel("Format:"));
    toolbar->addWidget(formatCombo_, 1);

    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 6px 16px; "
        "border-radius: 6px; font-weight: bold; }"
    );
    connect(generateBtn_, &QPushButton::clicked, this, &BibliographyBuilderWidget::onGenerate);
    toolbar->addWidget(generateBtn_);

    copyBtn_ = new QPushButton("Copy");
    connect(copyBtn_, &QPushButton::clicked, this, &BibliographyBuilderWidget::onCopy);
    toolbar->addWidget(copyBtn_);

    exportBtn_ = new QPushButton("Export File");
    connect(exportBtn_, &QPushButton::clicked, this, &BibliographyBuilderWidget::onExport);
    toolbar->addWidget(exportBtn_);

    layout->addLayout(toolbar);

    auto* splitter = new QSplitter(Qt::Horizontal);

    // Left: paper list
    auto* leftPanel = new QVBoxLayout();
    countLabel_ = new QLabel("0 papers");
    countLabel_->setStyleSheet("font-weight: bold; font-size: 12px;");
    leftPanel->addWidget(countLabel_);

    paperList_ = new QListWidget();
    paperList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 4px; }"
        "QListWidget::item:selected { background: #3b82f6; color: white; }"
    );
    leftPanel->addWidget(paperList_, 1);

    auto* btnRow = new QHBoxLayout();
    upBtn_ = new QPushButton("↑");
    upBtn_->setFixedSize(32, 32);
    connect(upBtn_, &QPushButton::clicked, this, &BibliographyBuilderWidget::onMoveUp);
    btnRow->addWidget(upBtn_);

    downBtn_ = new QPushButton("↓");
    downBtn_->setFixedSize(32, 32);
    connect(downBtn_, &QPushButton::clicked, this, &BibliographyBuilderWidget::onMoveDown);
    btnRow->addWidget(downBtn_);

    removeBtn_ = new QPushButton("Remove");
    removeBtn_->setStyleSheet("color: #dc2626;");
    connect(removeBtn_, &QPushButton::clicked, this, &BibliographyBuilderWidget::onRemove);
    btnRow->addWidget(removeBtn_);

    leftPanel->addLayout(btnRow);

    auto* leftWidget = new QWidget();
    leftWidget->setLayout(leftPanel);
    splitter->addWidget(leftWidget);

    // Right: output
    auto* rightPanel = new QVBoxLayout();
    rightPanel->addWidget(new QLabel("Bibliography Output:"));
    outputEdit_ = new QTextEdit();
    outputEdit_->setReadOnly(true);
    outputEdit_->setFont(QFont("Consolas", 10));
    outputEdit_->setStyleSheet("QTextEdit { padding: 8px; }");
    rightPanel->addWidget(outputEdit_, 1);

    auto* rightWidget = new QWidget();
    rightWidget->setLayout(rightPanel);
    splitter->addWidget(rightWidget);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    layout->addWidget(splitter, 1);

    statusLabel_ = new QLabel("Add papers and generate bibliography");
    statusLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statusLabel_);
}

void BibliographyBuilderWidget::addPaper(const BibPaper& paper) {
    for (const auto& p : papers_) if (p.id == paper.id) return;
    papers_.append(paper);
    refreshList();
    updateCount();
    emit paperAdded(paper.id);
}

void BibliographyBuilderWidget::addPapers(const QList<BibPaper>& list) {
    for (const auto& p : list) addPaper(p);
}

void BibliographyBuilderWidget::removePaper(int paperId) {
    papers_.removeIf([paperId](const BibPaper& p) { return p.id == paperId; });
    refreshList();
    updateCount();
    emit paperRemoved(paperId);
}

void BibliographyBuilderWidget::clearPapers() {
    papers_.clear();
    refreshList();
    updateCount();
    outputEdit_->clear();
}

void BibliographyBuilderWidget::setFormat(const QString& format) {
    int idx = formatCombo_->findText(format);
    if (idx >= 0) formatCombo_->setCurrentIndex(idx);
}

QString BibliographyBuilderWidget::generateBibliography() const {
    QStringList entries;
    for (const auto& p : papers_) {
        switch (currentFormat_) {
            case 0: entries << formatAPA(p); break;
            case 1: entries << formatMLA(p); break;
            case 2: entries << formatChicago(p); break;
            case 3: entries << formatIEEE(p); break;
            case 4: entries << formatBibTeX(p); break;
            case 5: entries << formatVancouver(p); break;
        }
    }
    return entries.join("\n\n");
}

QList<BibPaper> BibliographyBuilderWidget::papers() const { return papers_; }

void BibliographyBuilderWidget::onAdd() {
    // Placeholder — typically called from MainWindow with actual paper data
}

void BibliographyBuilderWidget::onRemove() {
    int row = paperList_->currentRow();
    if (row < 0 || row >= papers_.size()) return;
    int id = papers_[row].id;
    removePaper(id);
}

void BibliographyBuilderWidget::onMoveUp() {
    int row = paperList_->currentRow();
    if (row <= 0) return;
    std::swap(papers_[row], papers_[row - 1]);
    refreshList();
    paperList_->setCurrentRow(row - 1);
}

void BibliographyBuilderWidget::onMoveDown() {
    int row = paperList_->currentRow();
    if (row < 0 || row >= papers_.size() - 1) return;
    std::swap(papers_[row], papers_[row + 1]);
    refreshList();
    paperList_->setCurrentRow(row + 1);
}

void BibliographyBuilderWidget::onGenerate() {
    if (papers_.isEmpty()) {
        statusLabel_->setText("No papers added");
        return;
    }
    QString bib = generateBibliography();
    outputEdit_->setPlainText(bib);
    statusLabel_->setText(QString("Generated %1 entries in %2 format")
        .arg(papers_.size()).arg(formatCombo_->currentText()));
    emit bibliographyGenerated(formatCombo_->currentText(), papers_.size());
}

void BibliographyBuilderWidget::onCopy() {
    QString text = outputEdit_->toPlainText();
    if (!text.isEmpty()) {
        QApplication::clipboard()->setText(text);
        statusLabel_->setText("Copied to clipboard");
    }
}

void BibliographyBuilderWidget::onExport() {
    if (papers_.isEmpty()) return;
    QString ext = (currentFormat_ == 4) ? "BibTeX (*.bib)" : "Text (*.txt)";
    QString path = QFileDialog::getSaveFileName(this, "Export Bibliography", "bibliography", ext);
    if (path.isEmpty()) return;

    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << generateBibliography();
        statusLabel_->setText("Exported to " + path);
    }
}

void BibliographyBuilderWidget::onFormatChanged(int index) {
    currentFormat_ = index;
    if (!papers_.isEmpty()) onGenerate();
}

void BibliographyBuilderWidget::refreshList() {
    paperList_->clear();
    for (const auto& p : papers_) {
        QString display = QString("[%1] %2 (%3) — %4")
            .arg(p.id)
            .arg(p.title.left(50))
            .arg(p.year)
            .arg(p.authors.isEmpty() ? "" : p.authors.first().left(15));
        paperList_->addItem(display);
    }
}

void BibliographyBuilderWidget::updateCount() {
    countLabel_->setText(QString("%1 paper(s) selected").arg(papers_.size()));
}

QString BibliographyBuilderWidget::formatAPA(const BibPaper& p) const {
    QString authors = p.authors.join(", ");
    QString entry = QString("%1 (%2). %3. %4")
        .arg(authors)
        .arg(p.year)
        .arg(p.title)
        .arg(p.journal);
    if (!p.volume.isEmpty()) entry += ", " + p.volume;
    if (!p.pages.isEmpty()) entry += ", " + p.pages;
    if (!p.doi.isEmpty()) entry += ". https://doi.org/" + p.doi;
    return entry;
}

QString BibliographyBuilderWidget::formatMLA(const BibPaper& p) const {
    QString firstAuthor = p.authors.isEmpty() ? "Unknown" : p.authors.first();
    QString others = p.authors.size() > 1 ? ", et al" : "";
    QString entry = QString("%1%2. \"%3.\" %4")
        .arg(firstAuthor).arg(others).arg(p.title).arg(p.journal);
    if (!p.volume.isEmpty()) entry += ", vol. " + p.volume;
    entry += QString(", %1").arg(p.year);
    if (!p.pages.isEmpty()) entry += ", pp. " + p.pages;
    return entry + ".";
}

QString BibliographyBuilderWidget::formatChicago(const BibPaper& p) const {
    QString authors = p.authors.join(", ");
    QString entry = QString("%1. \"%2.\" %3")
        .arg(authors).arg(p.title).arg(p.journal);
    if (!p.volume.isEmpty()) entry += " " + p.volume;
    if (!p.pages.isEmpty()) entry += ": " + p.pages;
    entry += QString(" (%1)").arg(p.year);
    return entry + ".";
}

QString BibliographyBuilderWidget::formatIEEE(const BibPaper& p) const {
    QString authors = p.authors.join(", ");
    QString entry = QString("%1, \"%2,\" %3")
        .arg(authors).arg(p.title).arg(p.journal);
    if (!p.volume.isEmpty()) entry += ", vol. " + p.volume;
    if (!p.pages.isEmpty()) entry += ", pp. " + p.pages;
    entry += QString(", %1").arg(p.year);
    if (!p.doi.isEmpty()) entry += ", doi: " + p.doi;
    return entry + ".";
}

QString BibliographyBuilderWidget::formatBibTeX(const BibPaper& p) const {
    QString key = p.authors.isEmpty() ? "unknown" : p.authors.first().split(" ").last().toLower();
    key += QString::number(p.year);
    QString entry = QString("@article{%1,\n").arg(key);
    entry += QString("  title={%1},\n").arg(p.title);
    entry += QString("  author={%1},\n").arg(p.authors.join(" and "));
    entry += QString("  year={%1},\n").arg(p.year);
    if (!p.journal.isEmpty()) entry += QString("  journal={%1},\n").arg(p.journal);
    if (!p.volume.isEmpty()) entry += QString("  volume={%1},\n").arg(p.volume);
    if (!p.pages.isEmpty()) entry += QString("  pages={%1},\n").arg(p.pages);
    if (!p.doi.isEmpty()) entry += QString("  doi={%1},\n").arg(p.doi);
    entry.chop(2);
    return entry + "\n}";
}

QString BibliographyBuilderWidget::formatVancouver(const BibPaper& p) const {
    QString authors;
    for (int i = 0; i < p.authors.size() && i < 6; ++i) {
        if (i > 0) authors += ", ";
        QStringList parts = p.authors[i].split(" ");
        if (parts.size() >= 2)
            authors += parts.last() + " " + parts.first().left(1).toUpper();
        else authors += p.authors[i];
    }
    if (p.authors.size() > 6) authors += ", et al";
    QString entry = QString("%1. %2. %3").arg(authors).arg(p.title).arg(p.journal);
    if (!p.volume.isEmpty()) entry += ". " + p.volume;
    if (!p.pages.isEmpty()) entry += ":" + p.pages;
    entry += QString(" (%1)").arg(p.year);
    return entry + ".";
}

void BibliographyBuilderWidget::loadSettings() {}
void BibliographyBuilderWidget::saveSettings() {}
