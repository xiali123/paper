#include "LanguageDetectorWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSplitter>
#include <QPainter>

LanguageDetectorWidget::LanguageDetectorWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void LanguageDetectorWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // Input
    layout->addWidget(new QLabel("Input Text:"));
    inputEdit_ = new QTextEdit();
    inputEdit_->setMaximumHeight(120);
    inputEdit_->setPlaceholderText("Paste paper title, abstract, or full text...");
    inputEdit_->setStyleSheet("QTextEdit { font-size: 13px; padding: 6px; }");
    layout->addWidget(inputEdit_);

    // Buttons
    auto* btnRow = new QHBoxLayout();
    detectBtn_ = new QPushButton("Detect Language");
    detectBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 6px 16px; "
        "border-radius: 6px; font-weight: bold; }"
    );
    connect(detectBtn_, &QPushButton::clicked, this, &LanguageDetectorWidget::onDetect);
    btnRow->addWidget(detectBtn_);

    batchBtn_ = new QPushButton("Batch Detect (split by paragraph)");
    connect(batchBtn_, &QPushButton::clicked, this, &LanguageDetectorWidget::onBatchDetect);
    btnRow->addWidget(batchBtn_);

    clearBtn_ = new QPushButton("Clear");
    connect(clearBtn_, &QPushButton::clicked, this, &LanguageDetectorWidget::onClear);
    btnRow->addWidget(clearBtn_);

    btnRow->addStretch();
    layout->addLayout(btnRow);

    // Results
    auto* splitter = new QSplitter(Qt::Vertical);

    auto* topPanel = new QVBoxLayout();
    topPanel->addWidget(new QLabel("Detection Result:"));
    resultTable_ = new QTableWidget();
    resultTable_->setColumnCount(4);
    resultTable_->setHorizontalHeaderLabels({"Language", "Code", "Confidence", "Characters"});
    resultTable_->horizontalHeader()->setStretchLastSection(true);
    resultTable_->setColumnWidth(0, 140);
    resultTable_->setColumnWidth(1, 60);
    resultTable_->setColumnWidth(2, 80);
    resultTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    resultTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    topPanel->addWidget(resultTable_);

    auto* topWidget = new QWidget();
    topWidget->setLayout(topPanel);
    splitter->addWidget(topWidget);

    auto* bottomPanel = new QVBoxLayout();
    bottomPanel->addWidget(new QLabel("Language Distribution (Batch):"));
    distTable_ = new QTableWidget();
    distTable_->setColumnCount(3);
    distTable_->setHorizontalHeaderLabels({"Language", "Count", "Percentage"});
    distTable_->horizontalHeader()->setStretchLastSection(true);
    distTable_->setColumnWidth(0, 140);
    distTable_->setColumnWidth(1, 80);
    distTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    distTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    bottomPanel->addWidget(distTable_);

    auto* bottomWidget = new QWidget();
    bottomWidget->setLayout(bottomPanel);
    splitter->addWidget(bottomWidget);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    layout->addWidget(splitter, 1);

    statusLabel_ = new QLabel("Ready");
    statusLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statusLabel_);
}

LanguageResult LanguageDetectorWidget::detect(const QString& text) const {
    LanguageResult result;
    if (text.trimmed().isEmpty()) {
        result.language = "Unknown";
        result.code = "unk";
        return result;
    }

    int cjk = 0, latin = 0, cyrillic = 0, arabic = 0, devanagari = 0, thai = 0, korean = 0, total = 0;
    bool hasHiragana = false, hasKatakana = false;

    for (const QChar& ch : text) {
        ushort code = ch.unicode();
        if (ch.isSpace() || ch.isPunct()) continue;
        total++;

        if ((code >= 0x4E00 && code <= 0x9FFF) || (code >= 0x3400 && code <= 0x4DBF)) cjk++;
        else if (code >= 0x3040 && code <= 0x309F) { hasHiragana = true; cjk++; }
        else if (code >= 0x30A0 && code <= 0x30FF) { hasKatakana = true; cjk++; }
        else if (code >= 0xAC00 && code <= 0xD7AF) korean++;
        else if ((code >= 'A' && code <= 'Z') || (code >= 'a' && code <= 'z')) latin++;
        else if (code >= 0x0400 && code <= 0x04FF) cyrillic++;
        else if (code >= 0x0600 && code <= 0x06FF) arabic++;
        else if (code >= 0x0900 && code <= 0x097F) devanagari++;
        else if (code >= 0x0E00 && code <= 0x0E7F) thai++;
    }

    if (total == 0) {
        result.language = "Unknown";
        result.code = "unk";
        result.confidence = 0;
        return result;
    }

    struct LangScore { QString name; QString code; int count; };
    QList<LangScore> scores;
    scores.append({"Chinese", "zh", cjk});
    scores.append({"Korean", "ko", korean});
    scores.append({"English/Latin", "en", latin});
    scores.append({"Russian/Cyrillic", "ru", cyrillic});
    scores.append({"Arabic", "ar", arabic});
    scores.append({"Hindi/Devanagari", "hi", devanagari});
    scores.append({"Thai", "th", thai});

    std::sort(scores.begin(), scores.end(),
        [](const LangScore& a, const LangScore& b) { return a.count > b.count; });

    // Japanese if hiragana/katakana present
    if (hasHiragana || hasKatakana) {
        result.language = "Japanese";
        result.code = "ja";
        result.confidence = qMin(1.0, static_cast<double>(cjk) / total);
    } else {
        result.language = scores.first().name;
        result.code = scores.first().code;
        result.confidence = qMin(1.0, static_cast<double>(scores.first().count) / total);
    }

    result.charCount = text.length();
    return result;
}

QList<LanguageResult> LanguageDetectorWidget::detectMulti(const QString& text) const {
    QList<LanguageResult> results;
    QStringList segments = text.split(QRegularExpression("\n\n+"), Qt::SkipEmptyParts);
    if (segments.isEmpty()) segments << text;

    int totalChars = 0;
    QMap<QString, int> langCounts;
    for (const auto& seg : segments) {
        totalChars += seg.length();
        LanguageResult r = detect(seg);
        langCounts[r.language]++;
    }

    for (auto it = langCounts.begin(); it != langCounts.end(); ++it) {
        LanguageResult r;
        r.language = it.key();
        r.percentage = (totalChars > 0) ? it.value() * 100.0 / segments.size() : 0;
        r.charCount = 0;
        for (const auto& seg : segments) {
            if (detect(seg).language == it.key()) r.charCount += seg.length();
        }
        results.append(r);
    }

    std::sort(results.begin(), results.end(),
        [](const LanguageResult& a, const LanguageResult& b) { return a.percentage > b.percentage; });

    return results;
}

QMap<QString, int> LanguageDetectorWidget::batchDetect(const QList<QString>& texts) const {
    QMap<QString, int> dist;
    for (const auto& text : texts) {
        LanguageResult r = detect(text);
        dist[r.language]++;
    }
    return dist;
}

void LanguageDetectorWidget::setText(const QString& text) {
    inputEdit_->setPlainText(text);
}

void LanguageDetectorWidget::setBatchTexts(const QList<QString>& texts) {
    inputEdit_->setPlainText(texts.join("\n\n"));
}

void LanguageDetectorWidget::onDetect() {
    QString text = inputEdit_->toPlainText();
    if (text.trimmed().isEmpty()) {
        statusLabel_->setText("Enter text first");
        return;
    }

    LanguageResult primary = detect(text);
    QList<LanguageResult> multi = detectMulti(text);
    if (multi.isEmpty()) multi << primary;

    refreshResults(multi);
    statusLabel_->setText(QString("Primary: %1 (%2% confidence)")
        .arg(primary.language).arg(primary.confidence * 100, 0, 'f', 0));
    emit detectionCompleted(primary);
}

void LanguageDetectorWidget::onBatchDetect() {
    QStringList paragraphs = inputEdit_->toPlainText().split(QRegularExpression("\n\n+"), Qt::SkipEmptyParts);
    if (paragraphs.isEmpty()) {
        statusLabel_->setText("Enter text with paragraph breaks");
        return;
    }

    QMap<QString, int> dist = batchDetect(paragraphs);
    refreshDistribution(dist);
    statusLabel_->setText(QString("Batch: %1 segments analyzed").arg(paragraphs.size()));
    emit batchCompleted(dist);
}

void LanguageDetectorWidget::onClear() {
    inputEdit_->clear();
    resultTable_->setRowCount(0);
    distTable_->setRowCount(0);
    statusLabel_->setText("Cleared");
}

void LanguageDetectorWidget::refreshResults(const QList<LanguageResult>& results) {
    resultTable_->setRowCount(results.size());
    for (int i = 0; i < results.size(); ++i) {
        const auto& r = results[i];
        resultTable_->setItem(i, 0, new QTableWidgetItem(r.language));
        resultTable_->setItem(i, 1, new QTableWidgetItem(r.code));

        auto* confItem = new QTableWidgetItem(QString::number(r.confidence * 100, 'f', 0) + "%");
        if (r.confidence >= 0.8) confItem->setForeground(QColor(5, 150, 105));
        else if (r.confidence >= 0.5) confItem->setForeground(QColor(245, 158, 11));
        else confItem->setForeground(QColor(220, 38, 38));
        resultTable_->setItem(i, 2, confItem);

        resultTable_->setItem(i, 3, new QTableWidgetItem(QString::number(r.charCount)));
    }
}

void LanguageDetectorWidget::refreshDistribution(const QMap<QString, int>& dist) {
    int total = 0;
    for (auto it = dist.begin(); it != dist.end(); ++it) total += it.value();

    distTable_->setRowCount(dist.size());
    int row = 0;
    for (auto it = dist.begin(); it != dist.end(); ++it) {
        distTable_->setItem(row, 0, new QTableWidgetItem(it.key()));
        distTable_->setItem(row, 1, new QTableWidgetItem(QString::number(it.value())));
        double pct = (total > 0) ? it.value() * 100.0 / total : 0;
        distTable_->setItem(row, 2, new QTableWidgetItem(QString::number(pct, 'f', 1) + "%"));
        row++;
    }
}
