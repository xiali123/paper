#include "PaperReferenceExtractor.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFileDialog>
#include <QRegularExpression>

PaperReferenceExtractor::PaperReferenceExtractor(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadSettings();
}

void PaperReferenceExtractor::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* filterRow = new QHBoxLayout();
    filterRow->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Unresolved", "Resolved", "Journal", "Conference", "Book", "Unknown"});
    connect(filterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperReferenceExtractor::onFilterChanged);
    filterRow->addWidget(filterCombo_, 1);
    layout->addLayout(filterRow);

    auto* splitter = new QSplitter(Qt::Horizontal);

    // Input
    auto* leftPanel = new QWidget();
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->addWidget(new QLabel("Paste reference section:"));
    inputEdit_ = new QTextEdit();
    inputEdit_->setPlaceholderText("Paste bibliography / references section here...");
    leftLayout->addWidget(inputEdit_, 1);

    extractBtn_ = new QPushButton("Extract References");
    extractBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(extractBtn_, &QPushButton::clicked, this, &PaperReferenceExtractor::onExtract);
    leftLayout->addWidget(extractBtn_);
    splitter->addWidget(leftPanel);

    // Ref tree + preview
    auto* rightPanel = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    rightLayout->addWidget(new QLabel("Extracted references:"));
    refTree_ = new QTreeWidget();
    refTree_->setHeaderLabels({"#", "Authors", "Title", "Year", "Type"});
    refTree_->setColumnWidth(0, 30);
    refTree_->setColumnWidth(1, 150);
    refTree_->setColumnWidth(2, 200);
    refTree_->setColumnWidth(3, 50);
    refTree_->setStyleSheet(
        "QTreeWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QTreeWidget::item { padding: 2px; }"
        "QTreeWidget::item:selected { background: #3b82f6; color: white; }"
    );
    connect(refTree_, &QTreeWidget::itemClicked, this, &PaperReferenceExtractor::onRefSelected);
    rightLayout->addWidget(refTree_, 1);

    previewEdit_ = new QTextEdit();
    previewEdit_->setReadOnly(true);
    previewEdit_->setMaximumHeight(80);
    previewEdit_->setStyleSheet("QTextEdit { border: 1px solid palette(mid); border-radius: 4px; background: #f8fafc; }");
    rightLayout->addWidget(previewEdit_);

    auto* btnRow = new QHBoxLayout();
    resolveBtn_ = new QPushButton("Resolve");
    resolveBtn_->setStyleSheet("color: #059669;");
    connect(resolveBtn_, &QPushButton::clicked, this, &PaperReferenceExtractor::onResolve);
    btnRow->addWidget(resolveBtn_);

    deleteBtn_ = new QPushButton("Delete");
    deleteBtn_->setStyleSheet("color: #dc2626;");
    connect(deleteBtn_, &QPushButton::clicked, this, &PaperReferenceExtractor::onDelete);
    btnRow->addWidget(deleteBtn_);

    exportBtn_ = new QPushButton("Export BibTeX");
    connect(exportBtn_, &QPushButton::clicked, this, &PaperReferenceExtractor::onExportBib);
    btnRow->addWidget(exportBtn_);
    rightLayout->addLayout(btnRow);

    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    layout->addWidget(splitter, 1);

    statsLabel_ = new QLabel("0 references");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);
}

void PaperReferenceExtractor::extractFromText(const QString& text) {
    QList<ExtractedRef> extracted = parseReferences(text);
    for (const auto& r : extracted) addReference(r);
    emit extractionComplete(extracted.size());
}

void PaperReferenceExtractor::addReference(const ExtractedRef& ref) {
    ExtractedRef r = ref;
    if (r.id < 0) r.id = nextId_++;
    refs_.append(r);
    nextId_ = qMax(nextId_, r.id + 1);
    refreshTree();
    saveSettings();
    updateStats();
}

void PaperReferenceExtractor::removeReference(int refId) {
    refs_.removeIf([refId](const ExtractedRef& r) { return r.id == refId; });
    refreshTree();
    saveSettings();
    updateStats();
}

QList<ExtractedRef> PaperReferenceExtractor::references() const { return refs_; }

QList<ExtractedRef> PaperReferenceExtractor::unresolved() const {
    QList<ExtractedRef> result;
    for (const auto& r : refs_) if (!r.resolved) result.append(r);
    return result;
}

void PaperReferenceExtractor::exportBibTeX(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return;
    for (const auto& r : refs_) {
        QString key = r.authors.split(",").first().split(" ").first().toLower().trimmed();
        if (!r.year.isEmpty()) key += r.year;
        QString type = (r.refType == "book") ? "book" : "article";
        QString bib = QString("@%1{%2,\n  title={%3},\n  author={%4},\n  year={%5},\n  journal={%6}\n}\n\n")
            .arg(type, key, r.title, r.authors, r.year, r.journal);
        f.write(bib.toUtf8());
    }
}

void PaperReferenceExtractor::onExtract() {
    QString text = inputEdit_->toPlainText();
    if (text.trimmed().isEmpty()) return;
    extractFromText(text);
}

void PaperReferenceExtractor::onDelete() {
    if (selectedId_ < 0) return;
    removeReference(selectedId_);
    selectedId_ = -1;
    previewEdit_->clear();
}

void PaperReferenceExtractor::onResolve() {
    if (selectedId_ < 0) return;
    for (auto& r : refs_) {
        if (r.id == selectedId_) {
            r.resolved = true;
            refreshTree();
            saveSettings();
            emit referenceResolved(r.id);
            break;
        }
    }
}

void PaperReferenceExtractor::onExportBib() {
    QString path = QFileDialog::getSaveFileName(this, "Export BibTeX", "", "BibTeX (*.bib)");
    if (path.isEmpty()) return;
    exportBibTeX(path);
}

void PaperReferenceExtractor::onFilterChanged(int) { refreshTree(); }

void PaperReferenceExtractor::onRefSelected() {
    auto* item = refTree_->currentItem();
    if (!item) return;
    selectedId_ = item->data(0, Qt::UserRole).toInt();
    for (const auto& r : refs_) {
        if (r.id == selectedId_) {
            QString html = QString("<b>%1</b> (%2)<br>%3<br><i>%4</i>%5%6")
                .arg(r.title, r.year, r.authors, r.journal,
                     r.doi.isEmpty() ? "" : "<br>DOI: " + r.doi,
                     r.resolved ? "<br><font color='green'>Resolved</font>" : "");
            previewEdit_->setHtml(html);
            emit referenceClicked(r.id, r.title);
            break;
        }
    }
}

QList<ExtractedRef> PaperReferenceExtractor::parseReferences(const QString& text) {
    QList<ExtractedRef> result;
    QStringList lines = text.split('\n');

    QString current;
    for (const auto& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) {
            if (!current.isEmpty()) {
                ExtractedRef ref;
                ref.rawText = current;

                QRegularExpression yearRe("\\b(19|20)\\d{2}\\b");
                auto yearMatch = yearRe.match(current);
                if (yearMatch.hasMatch()) ref.year = yearMatch.captured();

                QRegularExpression doiRe("10\\.\\d{4,}/[^\\s,;]+");
                auto doiMatch = doiRe.match(current);
                if (doiMatch.hasMatch()) ref.doi = doiMatch.captured();

                if (current.contains("Proc.") || current.contains("Conf.") || current.contains("Proceedings"))
                    ref.refType = "conference";
                else if (current.contains("IEEE") || current.contains("ACM") || current.contains("Nature"))
                    ref.refType = "journal";
                else if (current.contains("Book") || current.contains("Publisher"))
                    ref.refType = "book";
                else
                    ref.refType = "unknown";

                // Extract authors (before first period or comma-heavy region)
                int firstPeriod = current.indexOf('.');
                if (firstPeriod > 5) {
                    ref.authors = current.left(firstPeriod).trimmed();
                    ref.title = current.mid(firstPeriod + 1).split('.').first().trimmed();
                } else {
                    QStringList parts = current.split('.', Qt::SkipEmptyParts);
                    if (parts.size() >= 2) {
                        ref.authors = parts[0].trimmed();
                        ref.title = parts[1].trimmed();
                    } else {
                        ref.title = current.left(80);
                    }
                }

                result.append(ref);
                current.clear();
            }
        } else {
            if (!current.isEmpty()) current += " ";
            current += trimmed;
        }
    }
    if (!current.isEmpty()) {
        ExtractedRef ref;
        ref.rawText = current;
        QRegularExpression yearRe("\\b(19|20)\\d{2}\\b");
        auto m = yearRe.match(current);
        if (m.hasMatch()) ref.year = m.captured();
        ref.title = current.left(60);
        ref.refType = "unknown";
        result.append(ref);
    }
    return result;
}

void PaperReferenceExtractor::refreshTree() {
    refTree_->clear();
    int filter = filterCombo_->currentIndex();
    QMap<QString, QColor> typeColors = {
        {"journal", QColor(59,130,246)}, {"conference", QColor(16,185,129)},
        {"book", QColor(245,158,11)}, {"unknown", QColor(148,163,184)}
    };

    for (int i = 0; i < refs_.size(); ++i) {
        const auto& r = refs_[i];
        if (filter == 1 && r.resolved) continue;
        if (filter == 2 && !r.resolved) continue;
        if (filter >= 3 && filter <= 5 && r.refType != filterCombo_->itemText(filter).toLower()) continue;

        auto* item = new QTreeWidgetItem({
            QString::number(i + 1),
            r.authors.left(25),
            r.title.left(35),
            r.year,
            r.refType
        });
        item->setData(0, Qt::UserRole, r.id);
        if (typeColors.contains(r.refType)) item->setForeground(4, typeColors[r.refType]);
        if (r.resolved) {
            QFont f = item->font(0);
            f.setItalic(true);
            item->setFont(0, f);
        }
        refTree_->addTopLevelItem(item);
    }
}

void PaperReferenceExtractor::updateStats() {
    int resolved = 0;
    for (const auto& r : refs_) if (r.resolved) resolved++;
    statsLabel_->setText(QString("%1 refs (%2 resolved, %3 pending)")
        .arg(refs_.size()).arg(resolved).arg(refs_.size() - resolved));
}

void PaperReferenceExtractor::loadSettings() {
    QSettings settings("PaperCrawler", "RefExtractor");
    QByteArray data = settings.value("refs").toByteArray();
    if (data.isEmpty()) return;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        ExtractedRef r;
        r.id = obj["id"].toInt();
        r.rawText = obj["rawText"].toString();
        r.authors = obj["authors"].toString();
        r.title = obj["title"].toString();
        r.year = obj["year"].toString();
        r.journal = obj["journal"].toString();
        r.doi = obj["doi"].toString();
        r.refType = obj["refType"].toString();
        r.resolved = obj["resolved"].toBool();
        refs_.append(r);
        nextId_ = qMax(nextId_, r.id + 1);
    }
    refreshTree();
    updateStats();
}

void PaperReferenceExtractor::saveSettings() {
    QJsonArray arr;
    for (const auto& r : refs_) {
        QJsonObject obj;
        obj["id"] = r.id;
        obj["rawText"] = r.rawText;
        obj["authors"] = r.authors;
        obj["title"] = r.title;
        obj["year"] = r.year;
        obj["journal"] = r.journal;
        obj["doi"] = r.doi;
        obj["refType"] = r.refType;
        obj["resolved"] = r.resolved;
        arr.append(obj);
    }
    QSettings settings("PaperCrawler", "RefExtractor");
    settings.setValue("refs", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}
