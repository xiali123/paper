#include "citation/CitationExporter.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>

CitationExporter::CitationExporter(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void CitationExporter::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // Top row: format selector + buttons
    auto* topRow = new QHBoxLayout();
    topRow->addWidget(new QLabel("Format:"));
    formatCombo_ = new QComboBox();
    formatCombo_->addItems(supportedFormats());
    connect(formatCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CitationExporter::onFormatChanged);
    topRow->addWidget(formatCombo_, 1);

    auto* copyAllBtn = new QPushButton("Copy All");
    copyAllBtn->setStyleSheet("padding: 4px 12px;");
    connect(copyAllBtn, &QPushButton::clicked, this, &CitationExporter::onCopyAll);
    topRow->addWidget(copyAllBtn);

    auto* copySelBtn = new QPushButton("Copy Selected");
    connect(copySelBtn, &QPushButton::clicked, this, &CitationExporter::onCopySelected);
    topRow->addWidget(copySelBtn);

    auto* exportBtn = new QPushButton("Export File");
    exportBtn->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }"
    );
    connect(exportBtn, &QPushButton::clicked, this, &CitationExporter::onExportFile);
    topRow->addWidget(exportBtn);

    layout->addLayout(topRow);

    countLabel_ = new QLabel("0 papers");
    countLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(countLabel_);

    // Splitter: paper list | citation preview
    auto* splitter = new QSplitter(Qt::Horizontal);

    paperList_ = new QListWidget();
    paperList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 4px; }"
        "QListWidget::item:selected { background: #3b82f6; color: white; }"
    );
    connect(paperList_, &QListWidget::itemClicked, this, &CitationExporter::onItemClicked);
    splitter->addWidget(paperList_);

    citationList_ = new QListWidget();
    citationList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; "
        "font-family: Consolas; font-size: 11px; }"
        "QListWidget::item { padding: 6px; }"
        "QListWidget::item:selected { background: #dbeafe; }"
    );
    citationList_->setWordWrap(true);
    splitter->addWidget(citationList_);

    splitter->setSizes({250, 400});
    layout->addWidget(splitter, 1);

    previewLabel_ = new QLabel("");
    previewLabel_->setStyleSheet("font-size: 10px; color: #94a3b8;");
    layout->addWidget(previewLabel_);
}

void CitationExporter::setPapers(const QList<Paper>& papers) {
    papers_ = papers;
    refreshList();
}

void CitationExporter::setFormat(const QString& format) {
    currentFormat_ = format;
    int idx = supportedFormats().indexOf(format);
    if (idx >= 0) formatCombo_->setCurrentIndex(idx);
}

QStringList CitationExporter::supportedFormats() const {
    return {"APA", "MLA", "Chicago", "Harvard", "Vancouver", "IEEE", "AMA"};
}

QString CitationExporter::generateCitation(const Paper& paper, const QString& format) const {
    QString authors = paper.authors;
    QString year = paper.year;
    QString title = paper.title;
    QString journal = paper.journalFull.isEmpty() ? paper.journal : paper.journalFull;
    QString doi = paper.doiUrl;

    if (format == "APA") {
        return QString("%1 (%2). %3. %4%5")
            .arg(authors, year, title, journal,
                 doi.isEmpty() ? "" : " " + doi);
    } else if (format == "MLA") {
        return QString("%1. \"%2.\" %3 (%4)%5")
            .arg(authors, title, journal, year,
                 doi.isEmpty() ? "." : ". " + doi + ".");
    } else if (format == "Chicago") {
        return QString("%1. \"%2.\" %3 (%4)%5")
            .arg(authors, title, journal, year,
                 doi.isEmpty() ? "." : ". " + doi + ".");
    } else if (format == "Harvard") {
        return QString("%1 (%2) '%3', %4%5")
            .arg(authors, year, title, journal,
                 doi.isEmpty() ? "." : ". Available at: " + doi + ".");
    } else if (format == "Vancouver") {
        return QString("%1. %2. %3. %4%5")
            .arg(authors, title, journal, year,
                 doi.isEmpty() ? "." : " doi: " + doi + ".");
    } else if (format == "IEEE") {
        return QString("%1, \"%2,\" %3, %4%5")
            .arg(authors, title, journal, year,
                 doi.isEmpty() ? "." : ", doi: " + doi + ".");
    } else if (format == "AMA") {
        return QString("%1. %2. %3. %4%5")
            .arg(authors, title, journal, year,
                 doi.isEmpty() ? "." : " " + doi + ".");
    }
    return QString("%1 (%2). %3. %4").arg(authors, year, title, journal);
}

QString CitationExporter::generateAll(const QString& format) const {
    QStringList citations;
    for (const auto& p : papers_) {
        citations << generateCitation(p, format);
    }
    return citations.join("\n\n");
}

void CitationExporter::onFormatChanged(int) {
    currentFormat_ = formatCombo_->currentText();
    refreshList();
}

void CitationExporter::onCopyAll() {
    QString text = generateAll(currentFormat_);
    QApplication::clipboard()->setText(text);
    previewLabel_->setText(QString("Copied %1 citations (%2)").arg(papers_.size()).arg(currentFormat_));
    emit copiedToClipboard(text);
}

void CitationExporter::onCopySelected() {
    auto* item = paperList_->currentItem();
    if (!item) return;
    int idx = paperList_->row(item);
    if (idx >= 0 && idx < papers_.size()) {
        QString cite = generateCitation(papers_[idx], currentFormat_);
        QApplication::clipboard()->setText(cite);
        previewLabel_->setText("Copied citation");
        emit copiedToClipboard(cite);
    }
}

void CitationExporter::onExportFile() {
    QString path = QFileDialog::getSaveFileName(this, "Export Citations",
        "citations.txt", "Text (*.txt);;All (*)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << generateAll(currentFormat_);
        emit exportedToFile(path, papers_.size());
        previewLabel_->setText(QString("Exported to %1").arg(path));
    }
}

void CitationExporter::onItemClicked(QListWidgetItem* item) {
    if (!item) return;
    int idx = paperList_->row(item);
    if (idx >= 0 && idx < papers_.size()) {
        QString cite = generateCitation(papers_[idx], currentFormat_);
        // Update corresponding citation item
        if (idx < citationList_->count()) {
            citationList_->item(idx)->setSelected(true);
        }
        previewLabel_->setText(cite.left(200));
    }
}

void CitationExporter::refreshList() {
    paperList_->clear();
    citationList_->clear();
    for (int i = 0; i < papers_.size(); ++i) {
        paperList_->addItem(papers_[i].title.left(60));
        citationList_->addItem(generateCitation(papers_[i], currentFormat_));
    }
    countLabel_->setText(QString("%1 papers — %2 format").arg(papers_.size()).arg(currentFormat_));
}
