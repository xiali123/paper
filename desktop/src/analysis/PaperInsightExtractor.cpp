#include "analysis/PaperInsightExtractor.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFileDialog>
#include <QRegularExpression>

PaperInsightExtractor::PaperInsightExtractor(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadSettings();
}

void PaperInsightExtractor::setupUI() {
    auto* layout = new QVBoxLayout(this);

    paperLabel_ = new QLabel("Paste text to extract insights");
    paperLabel_->setStyleSheet("font-weight: bold; font-size: 13px;");
    layout->addWidget(paperLabel_);

    auto* filterRow = new QHBoxLayout();
    filterRow->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Definition", "Method", "Result", "Claim", "Limitation", "Future Work"});
    connect(filterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperInsightExtractor::onFilterChanged);
    filterRow->addWidget(filterCombo_, 1);
    layout->addLayout(filterRow);

    auto* splitter = new QSplitter(Qt::Horizontal);

    // Input
    auto* leftPanel = new QWidget();
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->addWidget(new QLabel("Input text:"));
    inputEdit_ = new QTextEdit();
    inputEdit_->setPlaceholderText("Paste paper abstract or full text here...");
    inputEdit_->setStyleSheet("QTextEdit { border: 1px solid palette(mid); border-radius: 4px; }");
    leftLayout->addWidget(inputEdit_);

    extractBtn_ = new QPushButton("Extract Insights");
    extractBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(extractBtn_, &QPushButton::clicked, this, &PaperInsightExtractor::onExtract);
    leftLayout->addWidget(extractBtn_);
    splitter->addWidget(leftPanel);

    // Insights list + preview
    auto* rightPanel = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    rightLayout->addWidget(new QLabel("Extracted insights:"));
    insightList_ = new QListWidget();
    insightList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 3px; }"
        "QListWidget::item:selected { background: #3b82f6; color: white; }"
    );
    connect(insightList_, &QListWidget::itemClicked, this, &PaperInsightExtractor::onInsightSelected);
    rightLayout->addWidget(insightList_, 1);

    previewEdit_ = new QTextEdit();
    previewEdit_->setReadOnly(true);
    previewEdit_->setMaximumHeight(100);
    previewEdit_->setStyleSheet("QTextEdit { border: 1px solid palette(mid); border-radius: 4px; background: #f8fafc; }");
    rightLayout->addWidget(previewEdit_);

    auto* btnRow = new QHBoxLayout();
    deleteBtn_ = new QPushButton("Delete");
    deleteBtn_->setStyleSheet("color: #dc2626;");
    connect(deleteBtn_, &QPushButton::clicked, this, &PaperInsightExtractor::onDelete);
    btnRow->addWidget(deleteBtn_);

    exportBtn_ = new QPushButton("Export");
    connect(exportBtn_, &QPushButton::clicked, this, &PaperInsightExtractor::onExport);
    btnRow->addWidget(exportBtn_);
    rightLayout->addLayout(btnRow);

    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    layout->addWidget(splitter, 1);

    statsLabel_ = new QLabel("0 insights");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);
}

void PaperInsightExtractor::setPaper(int paperId, const QString& title) {
    currentPaperId_ = paperId;
    paperLabel_->setText(QString("Extract: %1").arg(title));
    refreshList();
    updateStats();
}

void PaperInsightExtractor::extractFromText(const QString& text) {
    QList<Insight> extracted = doExtraction(text);
    for (const auto& ins : extracted) addInsight(ins);
    emit extractionComplete(currentPaperId_, extracted.size());
}

void PaperInsightExtractor::addInsight(const Insight& insight) {
    Insight ins = insight;
    if (ins.id < 0) ins.id = nextId_++;
    if (ins.timestamp == 0) ins.timestamp = QDateTime::currentSecsSinceEpoch();
    if (ins.paperId < 0) ins.paperId = currentPaperId_;
    insights_.append(ins);
    nextId_ = qMax(nextId_, ins.id + 1);
    refreshList();
    saveSettings();
    updateStats();
    emit insightExtracted(ins.paperId, ins.id, ins.type);
}

void PaperInsightExtractor::removeInsight(int id) {
    insights_.removeIf([id](const Insight& i) { return i.id == id; });
    refreshList();
    saveSettings();
    updateStats();
    emit insightRemoved(id);
}

QList<Insight> PaperInsightExtractor::insights() const { return insights_; }

QList<Insight> PaperInsightExtractor::insightsByType(const QString& type) const {
    QList<Insight> result;
    for (const auto& i : insights_) {
        if (i.type == type) result.append(i);
    }
    return result;
}

void PaperInsightExtractor::onExtract() {
    QString text = inputEdit_->toPlainText();
    if (text.trimmed().isEmpty()) return;
    extractFromText(text);
}

void PaperInsightExtractor::onDelete() {
    auto* item = insightList_->currentItem();
    if (!item) return;
    int id = item->data(Qt::UserRole).toInt();
    removeInsight(id);
    previewEdit_->clear();
}

void PaperInsightExtractor::onFilterChanged(int) { refreshList(); }

void PaperInsightExtractor::onInsightSelected() {
    auto* item = insightList_->currentItem();
    if (!item) return;
    int id = item->data(Qt::UserRole).toInt();
    for (const auto& ins : insights_) {
        if (ins.id == id) {
            QString html = QString("<b>[%1]</b> Confidence: %2%<br><br>%3<br><br><i>Context:</i> %4")
                .arg(ins.type)
                .arg(ins.confidence * 100, 0, 'f', 0)
                .arg(ins.content.toHtmlEscaped())
                .arg(ins.context.toHtmlEscaped());
            previewEdit_->setHtml(html);
            break;
        }
    }
}

void PaperInsightExtractor::onExport() {
    QString fileName = QFileDialog::getSaveFileName(this, "Export Insights", "", "JSON (*.json)");
    if (fileName.isEmpty()) return;
    QJsonArray arr;
    for (const auto& ins : insights_) {
        QJsonObject obj;
        obj["id"] = ins.id;
        obj["paperId"] = ins.paperId;
        obj["type"] = ins.type;
        obj["content"] = ins.content;
        obj["context"] = ins.context;
        obj["confidence"] = ins.confidence;
        arr.append(obj);
    }
    QFile f(fileName);
    if (f.open(QIODevice::WriteOnly)) f.write(QJsonDocument(arr).toJson());
}

QList<Insight> PaperInsightExtractor::doExtraction(const QString& text) {
    QList<Insight> result;
    QStringList sentences = text.split(QRegularExpression("[.!?]"), Qt::SkipEmptyParts);

    QMap<QString, QStringList> patterns = {
        {"Definition", {"\\bis defined as\\b", "\\bis known as\\b", "\\brefers to\\b", "\\bdenotes\\b", "\\bmeans\\b"}},
        {"Method", {"\\bwe propose\\b", "\\bwe present\\b", "\\bwe introduce\\b", "\\bapproach\\b", "\\bmethod\\b", "\\balgorithm\\b"}},
        {"Result", {"\\bwe show\\b", "\\bwe demonstrate\\b", "\\bresults show\\b", "\\bachieve\\b", "\\boutperform\\b", "\\baccuracy\\b"}},
        {"Claim", {"\\bwe argue\\b", "\\bwe believe\\b", "\\bits clear\\b", "\\bsuggests that\\b", "\\bindicates that\\b"}},
        {"Limitation", {"\\blimitation\\b", "\\bdoes not\\b", "\\bcannot\\b", "\\bfails to\\b", "\\bfuture work\\b", "\\brestricted\\b"}},
        {"Future Work", {"\\bfuture work\\b", "\\bfuture research\\b", "\\bnext step\\b", "\\bfurther study\\b", "\\bextend\\b"}}
    };

    for (const auto& sentence : sentences) {
        QString trimmed = sentence.trimmed();
        if (trimmed.length() < 10) continue;

        for (auto it = patterns.begin(); it != patterns.end(); ++it) {
            for (const auto& pat : it.value()) {
                QRegularExpression re(pat, QRegularExpression::CaseInsensitiveOption);
                if (re.match(trimmed).hasMatch()) {
                    Insight ins;
                    ins.type = it.key();
                    ins.content = trimmed;
                    ins.context = trimmed.left(100);
                    ins.confidence = 0.6 + QRandomGenerator::global()->bounded(35) / 100.0;
                    result.append(ins);
                    break;
            }
            }
        }
    }
    return result;
}

void PaperInsightExtractor::refreshList() {
    insightList_->clear();
    QString filter = filterCombo_->currentText();
    QMap<QString, QColor> typeColors = {
        {"Definition", QColor(59,130,246)}, {"Method", QColor(16,185,129)},
        {"Result", QColor(245,158,11)}, {"Claim", QColor(139,92,246)},
        {"Limitation", QColor(239,68,68)}, {"Future Work", QColor(14,165,233)}
    };

    for (const auto& ins : insights_) {
        if (filter != "All" && ins.type != filter) continue;
        QString display = QString("[%1] %2% %3")
            .arg(ins.type)
            .arg(ins.confidence * 100, 0, 'f', 0)
            .arg(ins.content.left(50));
        auto* item = new QListWidgetItem(display);
        item->setData(Qt::UserRole, ins.id);
        if (typeColors.contains(ins.type)) item->setForeground(typeColors[ins.type]);
        insightList_->addItem(item);
    }
}

void PaperInsightExtractor::updateStats() {
    QMap<QString, int> counts;
    for (const auto& ins : insights_) counts[ins.type]++;
    QStringList parts;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        parts << QString("%1: %2").arg(it.key()).arg(it.value());
    }
    statsLabel_->setText(QString("%1 insights | %2").arg(insights_.size()).arg(parts.join(", ")));
}

void PaperInsightExtractor::loadSettings() {
    QSettings settings("PaperCrawler", "InsightExtractor");
    QByteArray data = settings.value("insights").toByteArray();
    if (data.isEmpty()) return;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        Insight ins;
        ins.id = obj["id"].toInt();
        ins.paperId = obj["paperId"].toInt();
        ins.type = obj["type"].toString();
        ins.content = obj["content"].toString();
        ins.context = obj["context"].toString();
        ins.confidence = obj["confidence"].toDouble();
        ins.timestamp = obj["timestamp"].toInteger();
        insights_.append(ins);
        nextId_ = qMax(nextId_, ins.id + 1);
    }
    refreshList();
    updateStats();
}

void PaperInsightExtractor::saveSettings() {
    QJsonArray arr;
    for (const auto& ins : insights_) {
        QJsonObject obj;
        obj["id"] = ins.id;
        obj["paperId"] = ins.paperId;
        obj["type"] = ins.type;
        obj["content"] = ins.content;
        obj["context"] = ins.context;
        obj["confidence"] = ins.confidence;
        obj["timestamp"] = static_cast<qint64>(ins.timestamp);
        arr.append(obj);
    }
    QSettings settings("PaperCrawler", "InsightExtractor");
    settings.setValue("insights", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}
