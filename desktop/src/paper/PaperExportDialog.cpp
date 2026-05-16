#include "paper/PaperExportDialog.hpp"
#include "core/PaperTypes.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QDialogButtonBox>
#include <QJsonDocument>
#include <QJsonArray>

PaperExportDialog::PaperExportDialog(const QList<Paper>& papers, QWidget* parent)
    : QDialog(parent)
    , papers_(papers)
{
    setWindowTitle("Export Papers");
    resize(700, 550);
    setupUI();
}

void PaperExportDialog::setupUI() {
    auto* layout = new QVBoxLayout(this);

    countLabel_ = new QLabel(QString("%1 papers to export").arg(papers_.size()));
    countLabel_->setStyleSheet("font-weight: bold; font-size: 12px;");
    layout->addWidget(countLabel_);

    // Format selection
    auto* formGroup = new QGroupBox("Export Settings");
    auto* form = new QFormLayout(formGroup);

    formatCombo_ = new QComboBox();
    formatCombo_->addItems({"CSV", "BibTeX", "Markdown", "JSON", "EndNote", "RIS"});
    connect(formatCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperExportDialog::onFormatChanged);
    form->addRow("Format:", formatCombo_);

    filenameEdit_ = new QLineEdit("papers_export");
    form->addRow("Filename:", filenameEdit_);

    includeAbstractCheck_ = new QCheckBox("Include abstracts");
    includeAbstractCheck_->setChecked(true);
    form->addRow("", includeAbstractCheck_);

    includeKeywordsCheck_ = new QCheckBox("Include keywords");
    includeKeywordsCheck_->setChecked(true);
    form->addRow("", includeKeywordsCheck_);

    layout->addWidget(formGroup);

    // Preview
    auto* previewGroup = new QGroupBox("Preview");
    auto* previewLayout = new QVBoxLayout(previewGroup);

    previewEdit_ = new QTextEdit();
    previewEdit_->setReadOnly(true);
    previewEdit_->setFont(QFont("Consolas", 10));
    previewEdit_->setMaximumHeight(250);
    previewLayout->addWidget(previewEdit_);

    auto* previewBtn = new QPushButton("Refresh Preview");
    connect(previewBtn, &QPushButton::clicked, this, &PaperExportDialog::onPreview);
    previewLayout->addWidget(previewBtn);

    layout->addWidget(previewGroup, 1);

    // Buttons
    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();

    auto* exportBtn = new QPushButton("Export to File");
    exportBtn->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 8px 20px; "
        "border-radius: 6px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }"
    );
    connect(exportBtn, &QPushButton::clicked, this, &PaperExportDialog::onExport);
    btnRow->addWidget(exportBtn);

    auto* closeBtn = new QPushButton("Close");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnRow->addWidget(closeBtn);

    layout->addLayout(btnRow);

    onPreview();
}

void PaperExportDialog::onFormatChanged(int) {
    onPreview();
}

void PaperExportDialog::onPreview() {
    QString preview;
    switch (formatCombo_->currentIndex()) {
        case 0: preview = generateCsv(); break;
        case 1: preview = generateBibtex(); break;
        case 2: preview = generateMarkdown(); break;
        case 3: preview = generateJson(); break;
        case 4: preview = generateEndNote(); break;
        case 5: preview = generateRis(); break;
    }
    previewEdit_->setPlainText(preview.left(5000));
    if (preview.length() > 5000) {
        previewEdit_->append("\n\n... (truncated preview)");
    }
}

void PaperExportDialog::onExport() {
    QStringList filters;
    QString ext;
    switch (formatCombo_->currentIndex()) {
        case 0: filters << "CSV (*.csv)"; ext = ".csv"; break;
        case 1: filters << "BibTeX (*.bib)"; ext = ".bib"; break;
        case 2: filters << "Markdown (*.md)"; ext = ".md"; break;
        case 3: filters << "JSON (*.json)"; ext = ".json"; break;
        case 4: filters << "EndNote (*.enw)"; ext = ".enw"; break;
        case 5: filters << "RIS (*.ris)"; ext = ".ris"; break;
    }

    QString path = QFileDialog::getSaveFileName(this, "Export",
        filenameEdit_->text() + ext, filters.join(";;"));
    if (path.isEmpty()) return;

    QString content;
    switch (formatCombo_->currentIndex()) {
        case 0: content = generateCsv(); break;
        case 1: content = generateBibtex(); break;
        case 2: content = generateMarkdown(); break;
        case 3: content = generateJson(); break;
        case 4: content = generateEndNote(); break;
        case 5: content = generateRis(); break;
    }

    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << content;
        accept();
    }
}

QString PaperExportDialog::generateCsv() const {
    QStringList lines;
    QStringList headers = {"ID", "Title", "Authors", "Year", "Journal", "DOI", "Source"};
    if (includeAbstractCheck_->isChecked()) headers << "Abstract";
    if (includeKeywordsCheck_->isChecked()) headers << "Keywords";
    lines << headers.join(",");

    for (const auto& p : papers_) {
        QStringList row;
        row << QString::number(p.id);
        row << QString("\"%1\"").arg(p.title.replace("\"", "\"\""));
        row << QString("\"%1\"").arg(p.authors.replace("\"", "\"\""));
        row << p.year;
        row << QString("\"%1\"").arg(p.journal.replace("\"", "\"\""));
        row << p.doiUrl;
        row << p.source;
        if (includeAbstractCheck_->isChecked())
            row << QString("\"%1\"").arg(p.abstract.left(500).replace("\"", "\"\""));
        if (includeKeywordsCheck_->isChecked())
            row << p.keywords.join(";");
        lines << row.join(",");
    }
    return lines.join("\n");
}

QString PaperExportDialog::generateBibtex() const {
    QString result;
    for (const auto& p : papers_) {
        QString key = p.authors.split(",").first().trimmed().split(" ").last().toLower()
                    + p.year + p.title.left(10).split(" ").last().toLower();
        result += QString("@article{%1,\n").arg(key);
        result += QString("  title = {%1},\n").arg(p.title);
        result += QString("  author = {%1},\n").arg(p.authors);
        result += QString("  year = {%1},\n").arg(p.year);
        result += QString("  journal = {%1},\n").arg(p.journal);
        if (!p.doiUrl.isEmpty()) result += QString("  doi = {%1},\n").arg(p.doiUrl);
        if (includeAbstractCheck_->isChecked() && !p.abstract.isEmpty())
            result += QString("  abstract = {%1},\n").arg(p.abstract.left(300));
        result += "}\n\n";
    }
    return result;
}

QString PaperExportDialog::generateMarkdown() const {
    QString result = "# Exported Papers\n\n";
    for (const auto& p : papers_) {
        result += QString("## %1\n").arg(p.title);
        result += QString("- **Authors:** %1\n").arg(p.authors);
        result += QString("- **Year:** %1\n").arg(p.year);
        result += QString("- **Journal:** %1\n").arg(p.journal);
        if (!p.doiUrl.isEmpty()) result += QString("- **DOI:** %1\n").arg(p.doiUrl);
        if (includeKeywordsCheck_->isChecked() && !p.keywords.isEmpty())
            result += QString("- **Keywords:** %1\n").arg(p.keywords.join(", "));
        if (includeAbstractCheck_->isChecked() && !p.abstract.isEmpty())
            result += QString("\n> %1\n").arg(p.abstract.left(300));
        result += "\n---\n\n";
    }
    return result;
}

QString PaperExportDialog::generateJson() const {
    QJsonArray arr;
    for (const auto& p : papers_) {
        QJsonObject obj;
        obj["id"] = p.id;
        obj["title"] = p.title;
        obj["authors"] = p.authors;
        obj["year"] = p.year;
        obj["journal"] = p.journal;
        obj["doi"] = p.doiUrl;
        if (includeAbstractCheck_->isChecked()) obj["abstract"] = p.abstract;
        if (includeKeywordsCheck_->isChecked()) {
            QJsonArray kw;
            for (const auto& k : p.keywords) kw.append(k);
            obj["keywords"] = kw;
        }
        arr.append(obj);
    }
    return QJsonDocument(arr).toJson(QJsonDocument::Indented);
}

QString PaperExportDialog::generateEndNote() const {
    QString result;
    for (const auto& p : papers_) {
        result += "%0 Journal Article\n";
        result += QString("%T %1\n").arg(p.title);
        result += QString("%A %1\n").arg(p.authors.replace("; ", "\n%A "));
        result += QString("%D %1\n").arg(p.year);
        result += QString("%J %1\n").arg(p.journal);
        if (!p.doiUrl.isEmpty()) result += QString("%R %1\n").arg(p.doiUrl);
        if (includeAbstractCheck_->isChecked() && !p.abstract.isEmpty())
            result += QString("%X %1\n").arg(p.abstract.left(300));
        result += "\n";
    }
    return result;
}

QString PaperExportDialog::generateRis() const {
    QString result;
    for (const auto& p : papers_) {
        result += "TY  - JOUR\n";
        result += QString("TI  - %1\n").arg(p.title);
        result += QString("AU  - %1\n").arg(p.authors.replace("; ", "\nAU  - "));
        result += QString("PY  - %1\n").arg(p.year);
        result += QString("JO  - %1\n").arg(p.journal);
        if (!p.doiUrl.isEmpty()) result += QString("DO  - %1\n").arg(p.doiUrl);
        if (includeAbstractCheck_->isChecked() && !p.abstract.isEmpty())
            result += QString("AB  - %1\n").arg(p.abstract.left(300));
        result += "ER  - \n\n";
    }
    return result;
}
