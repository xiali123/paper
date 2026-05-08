#include "analysis/AbstractSummaryWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QApplication>
#include <QRegularExpression>

AbstractSummaryWidget::AbstractSummaryWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void AbstractSummaryWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel("Output:"));
    formatCombo_ = new QComboBox();
    formatCombo_->addItems({"Structured", "Bullet Points", "One-Liner", "Key-Value"});
    connect(formatCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AbstractSummaryWidget::onFormatChanged);
    toolbar->addWidget(formatCombo_);

    extractBtn_ = new QPushButton("Extract");
    extractBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 6px 16px; "
        "border-radius: 6px; font-weight: bold; }"
    );
    connect(extractBtn_, &QPushButton::clicked, this, &AbstractSummaryWidget::onExtract);
    toolbar->addWidget(extractBtn_);

    batchBtn_ = new QPushButton("Batch (all papers)");
    connect(batchBtn_, &QPushButton::clicked, this, &AbstractSummaryWidget::onBatch);
    toolbar->addWidget(batchBtn_);

    copyBtn_ = new QPushButton("Copy");
    connect(copyBtn_, &QPushButton::clicked, this, &AbstractSummaryWidget::onCopy);
    toolbar->addWidget(copyBtn_);

    clearBtn_ = new QPushButton("Clear");
    connect(clearBtn_, &QPushButton::clicked, this, &AbstractSummaryWidget::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    auto* splitter = new QSplitter(Qt::Horizontal);

    // Input
    auto* leftPanel = new QVBoxLayout();
    leftPanel->addWidget(new QLabel("Abstract Input:"));
    inputEdit_ = new QTextEdit();
    inputEdit_->setPlaceholderText("Paste paper abstract here...\n\nThe extract function will identify:\n- Objective/Goal\n- Method/Approach\n- Key Results\n- Conclusion\n- Keywords");
    inputEdit_->setStyleSheet("QTextEdit { font-size: 13px; padding: 8px; }");
    leftPanel->addWidget(inputEdit_, 1);

    leftPanel->addWidget(new QLabel("Extracted Keywords:"));
    keywordList_ = new QListWidget();
    keywordList_->setMaximumHeight(120);
    keywordList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 3px; }"
    );
    leftPanel->addWidget(keywordList_);

    auto* leftWidget = new QWidget();
    leftWidget->setLayout(leftPanel);
    splitter->addWidget(leftWidget);

    // Output
    auto* rightPanel = new QVBoxLayout();
    rightPanel->addWidget(new QLabel("Structured Summary:"));
    outputEdit_ = new QTextEdit();
    outputEdit_->setReadOnly(true);
    outputEdit_->setStyleSheet("QTextEdit { font-size: 13px; padding: 8px; background: #f8fafc; }");
    rightPanel->addWidget(outputEdit_, 1);

    auto* rightWidget = new QWidget();
    rightWidget->setLayout(rightPanel);
    splitter->addWidget(rightWidget);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    layout->addWidget(splitter, 1);

    statsLabel_ = new QLabel("Ready");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);
}

void AbstractSummaryWidget::setAbstract(const QString& text) {
    inputEdit_->setPlainText(text);
}

ExtractedInfo AbstractSummaryWidget::extract(const QString& text) const {
    ExtractedInfo info;
    if (text.trimmed().isEmpty()) return info;

    // Split into sentences
    QStringList sentences = text.split(QRegularExpression("[.!?]"), Qt::SkipEmptyParts);
    if (sentences.isEmpty()) return info;

    // Objective: first 1-2 sentences (often contain "propose", "present", "investigate", "aim")
    QStringList objectiveSentences;
    for (int i = 0; i < qMin(2, sentences.size()); ++i) {
        objectiveSentences << sentences[i].trimmed();
    }
    info.objective = objectiveSentences.join(". ");

    // Method: sentences containing method keywords
    QStringList methodKeywords = {"method", "approach", "propose", "algorithm", "model", "framework",
                                   "technique", "architecture", "pipeline", "using", "based on", "employ"};
    QStringList methodSentences;
    for (const auto& s : sentences) {
        QString lower = s.toLower();
        for (const auto& kw : methodKeywords) {
            if (lower.contains(kw)) { methodSentences << s.trimmed(); break; }
        }
    }
    info.method = methodSentences.isEmpty() ? sentences.value(qMin(2, sentences.size() - 1)).trimmed()
                                             : methodSentences.join(". ");

    // Results: sentences with result keywords
    QStringList resultKeywords = {"result", "achieve", "accuracy", "performance", "improve",
                                   "outperform", "show", "demonstrate", "experiment", "evaluation"};
    QStringList resultSentences;
    for (const auto& s : sentences) {
        QString lower = s.toLower();
        for (const auto& kw : resultKeywords) {
            if (lower.contains(kw)) { resultSentences << s.trimmed(); break; }
        }
    }
    info.result = resultSentences.isEmpty() ? "" : resultSentences.join(". ");

    // Conclusion: last sentence(s)
    info.conclusion = sentences.isEmpty() ? "" : sentences.last().trimmed();
    if (sentences.size() >= 2 && !info.result.contains(sentences[sentences.size() - 2].trimmed()))
        info.conclusion = sentences[sentences.size() - 2].trimmed() + ". " + info.conclusion;

    // Keywords extraction: most frequent meaningful words
    QMap<QString, int> wordFreq;
    QStringList stopWords = {"the", "a", "an", "is", "are", "was", "were", "be", "been", "being",
        "have", "has", "had", "do", "does", "did", "will", "would", "could", "should", "may", "might",
        "shall", "can", "need", "dare", "ought", "used", "to", "of", "in", "for", "on", "with", "at",
        "by", "from", "as", "into", "through", "during", "before", "after", "above", "below", "between",
        "out", "off", "over", "under", "again", "further", "then", "once", "and", "but", "or", "nor",
        "not", "so", "yet", "both", "either", "neither", "each", "every", "all", "any", "few", "more",
        "most", "other", "some", "such", "no", "only", "own", "same", "than", "too", "very", "just",
        "because", "this", "that", "these", "those", "it", "its", "we", "our", "they", "their", "which",
        "what", "when", "where", "who", "how", "if", "then", "also", "about", "up", "there", "here"};

    QStringList words = text.toLower().split(QRegularExpression("[^a-z]+"), Qt::SkipEmptyParts);
    for (const auto& w : words) {
        if (w.length() < 3 || stopWords.contains(w)) continue;
        wordFreq[w]++;
    }

    QList<QPair<QString, int>> sorted;
    for (auto it = wordFreq.begin(); it != wordFreq.end(); ++it)
        sorted.append({it.key(), it.value()});
    std::sort(sorted.begin(), sorted.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });

    for (int i = 0; i < qMin(10, sorted.size()); ++i)
        info.keywords.append(sorted[i].first);

    // Domain detection
    QMap<QString, QStringList> domainKeywords = {
        {"Machine Learning", {"learning", "neural", "deep", "training", "classification", "regression"}},
        {"NLP", {"language", "text", "sentiment", "translation", "nlp", "word"}},
        {"Computer Vision", {"image", "visual", "detection", "recognition", "segmentation"}},
        {"Security", {"security", "privacy", "encryption", "attack", "vulnerability"}},
        {"Robotics", {"robot", "manipulation", "navigation", "sensor", "autonomous"}},
        {"Bioinformatics", {"gene", "protein", "biological", "genomics", "cell"}},
    };
    for (auto it = domainKeywords.begin(); it != domainKeywords.end(); ++it) {
        int score = 0;
        for (const auto& kw : it.value()) {
            if (text.toLower().contains(kw)) score++;
        }
        if (score >= 2) { info.domain = it.key(); break; }
    }
    if (info.domain.isEmpty()) info.domain = "General";

    return info;
}

QList<ExtractedInfo> AbstractSummaryWidget::batchExtract(const QList<QString>& abstracts) const {
    QList<ExtractedInfo> results;
    for (const auto& text : abstracts) results.append(extract(text));
    return results;
}

QString AbstractSummaryWidget::generateStructuredSummary(const ExtractedInfo& info) const {
    int format = formatCombo_->currentIndex();
    switch (format) {
        case 0: // Structured
            return QString("OBJECTIVE: %1\n\nMETHOD: %2\n\nRESULTS: %3\n\nCONCLUSION: %4\n\nDOMAIN: %5\n\nKEYWORDS: %6")
                .arg(info.objective).arg(info.method).arg(info.result)
                .arg(info.conclusion).arg(info.domain).arg(info.keywords.join(", "));

        case 1: // Bullet Points
            return QString("- Objective: %1\n- Method: %2\n- Results: %3\n- Conclusion: %4\n- Domain: %5\n- Keywords: %6")
                .arg(info.objective).arg(info.method).arg(info.result)
                .arg(info.conclusion).arg(info.domain).arg(info.keywords.join(", "));

        case 2: // One-Liner
            return QString("[%1] %2 — %3")
                .arg(info.domain, info.objective.left(80), info.result.left(60));

        case 3: // Key-Value
            return QString("objective=%1\nmethod=%2\nresult=%3\nconclusion=%4\ndomain=%5\nkeywords=%6")
                .arg(info.objective).arg(info.method).arg(info.result)
                .arg(info.conclusion).arg(info.domain).arg(info.keywords.join(";"));
    }
    return "";
}

void AbstractSummaryWidget::onExtract() {
    QString text = inputEdit_->toPlainText();
    if (text.trimmed().isEmpty()) {
        statsLabel_->setText("Enter an abstract first");
        return;
    }

    lastResult_ = extract(text);
    displayResult(lastResult_);
    statsLabel_->setText(QString("Extracted: %1 keywords, domain: %2")
        .arg(lastResult_.keywords.size()).arg(lastResult_.domain));
    emit extractionCompleted(lastResult_);
}

void AbstractSummaryWidget::onBatch() {
    QStringList paragraphs = inputEdit_->toPlainText().split(QRegularExpression("\n\n+"), Qt::SkipEmptyParts);
    if (paragraphs.size() < 2) {
        statsLabel_->setText("Enter multiple abstracts separated by blank lines");
        return;
    }
    auto results = batchExtract(paragraphs);
    QStringList summaries;
    for (int i = 0; i < results.size(); ++i) {
        summaries << QString("--- Paper %1 ---\n%2").arg(i + 1).arg(generateStructuredSummary(results[i]));
    }
    outputEdit_->setPlainText(summaries.join("\n\n"));
    statsLabel_->setText(QString("Batch: %1 abstracts processed").arg(results.size()));
    emit batchCompleted(results.size());
}

void AbstractSummaryWidget::onCopy() {
    QString text = outputEdit_->toPlainText();
    if (!text.isEmpty()) QApplication::clipboard()->setText(text);
}

void AbstractSummaryWidget::onClear() {
    inputEdit_->clear();
    outputEdit_->clear();
    keywordList_->clear();
    statsLabel_->setText("Cleared");
}

void AbstractSummaryWidget::onFormatChanged(int) {
    if (!lastResult_.objective.isEmpty()) displayResult(lastResult_);
}

void AbstractSummaryWidget::displayResult(const ExtractedInfo& info) {
    outputEdit_->setPlainText(generateStructuredSummary(info));

    keywordList_->clear();
    for (const auto& kw : info.keywords) {
        auto* item = new QListWidgetItem(kw);
        item->setForeground(QColor(59, 130, 246));
        keywordList_->addItem(item);
    }
}
