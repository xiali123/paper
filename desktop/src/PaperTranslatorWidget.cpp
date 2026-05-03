#include "PaperTranslatorWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QApplication>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>

PaperTranslatorWidget::PaperTranslatorWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadHistory();
}

void PaperTranslatorWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // Language selectors
    auto* langRow = new QHBoxLayout();

    srcLangCombo_ = new QComboBox();
    srcLangCombo_->addItems({"Auto Detect", "English", "Chinese", "Japanese", "Korean", "French", "German", "Spanish", "Portuguese", "Russian", "Arabic"});
    connect(srcLangCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperTranslatorWidget::onSourceLangChanged);
    langRow->addWidget(new QLabel("From:"));
    langRow->addWidget(srcLangCombo_, 1);

    swapBtn_ = new QPushButton("⇄");
    swapBtn_->setFixedSize(36, 36);
    swapBtn_->setStyleSheet(
        "QPushButton { font-size: 18px; border: 1px solid #d1d5db; border-radius: 18px; }"
        "QPushButton:hover { background: #e5e7eb; }"
    );
    connect(swapBtn_, &QPushButton::clicked, this, &PaperTranslatorWidget::onSwap);
    langRow->addWidget(swapBtn_);

    tgtLangCombo_ = new QComboBox();
    tgtLangCombo_->addItems({"English", "Chinese", "Japanese", "Korean", "French", "German", "Spanish", "Portuguese", "Russian", "Arabic"});
    tgtLangCombo_->setCurrentIndex(1);
    connect(tgtLangCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperTranslatorWidget::onTargetLangChanged);
    langRow->addWidget(new QLabel("To:"));
    langRow->addWidget(tgtLangCombo_, 1);

    layout->addLayout(langRow);

    // Source + result
    auto* splitter = new QSplitter(Qt::Horizontal);

    auto* leftPanel = new QVBoxLayout();
    leftPanel->addWidget(new QLabel("Source Text:"));
    sourceEdit_ = new QTextEdit();
    sourceEdit_->setPlaceholderText("Enter or paste paper title/abstract to translate...");
    sourceEdit_->setStyleSheet("QTextEdit { font-size: 13px; padding: 8px; }");
    leftPanel->addWidget(sourceEdit_, 1);

    charCountLabel_ = new QLabel("0 chars");
    charCountLabel_->setStyleSheet("font-size: 10px; color: #94a3b8;");
    leftPanel->addWidget(charCountLabel_);

    auto* leftWidget = new QWidget();
    leftWidget->setLayout(leftPanel);
    splitter->addWidget(leftWidget);

    auto* rightPanel = new QVBoxLayout();
    rightPanel->addWidget(new QLabel("Translation:"));
    resultEdit_ = new QTextEdit();
    resultEdit_->setReadOnly(true);
    resultEdit_->setPlaceholderText("Translation will appear here...");
    resultEdit_->setStyleSheet("QTextEdit { font-size: 13px; padding: 8px; background: #f8fafc; }");
    rightPanel->addWidget(resultEdit_, 1);

    auto* rightWidget = new QWidget();
    rightWidget->setLayout(rightPanel);
    splitter->addWidget(rightWidget);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    layout->addWidget(splitter, 1);

    // Buttons
    auto* btnRow = new QHBoxLayout();

    translateBtn_ = new QPushButton("Translate");
    translateBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 8px 24px; "
        "border-radius: 6px; font-weight: bold; font-size: 13px; }"
    );
    connect(translateBtn_, &QPushButton::clicked, this, &PaperTranslatorWidget::onTranslate);
    btnRow->addWidget(translateBtn_);

    copyBtn_ = new QPushButton("Copy Result");
    connect(copyBtn_, &QPushButton::clicked, this, &PaperTranslatorWidget::onCopy);
    btnRow->addWidget(copyBtn_);

    clearBtn_ = new QPushButton("Clear");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperTranslatorWidget::onClear);
    btnRow->addWidget(clearBtn_);

    btnRow->addStretch();
    layout->addLayout(btnRow);

    statusLabel_ = new QLabel("Ready");
    statusLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statusLabel_);

    // History
    layout->addWidget(new QLabel("Translation History:"));
    historyList_ = new QListWidget();
    historyList_->setMaximumHeight(120);
    historyList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 3px; }"
    );
    connect(historyList_, &QListWidget::itemClicked, this, &PaperTranslatorWidget::onHistoryClicked);
    layout->addWidget(historyList_);

    connect(sourceEdit_, &QTextEdit::textChanged, this, [this]() {
        charCountLabel_->setText(QString("%1 chars").arg(sourceEdit_->toPlainText().length()));
    });
}

void PaperTranslatorWidget::setSourceText(const QString& text) {
    sourceEdit_->setPlainText(text);
}

void PaperTranslatorWidget::setLanguages(const QString& source, const QString& target) {
    int si = srcLangCombo_->findText(source);
    if (si >= 0) srcLangCombo_->setCurrentIndex(si);
    int ti = tgtLangCombo_->findText(target);
    if (ti >= 0) tgtLangCombo_->setCurrentIndex(ti);
}

QList<TranslationEntry> PaperTranslatorWidget::history() const { return history_; }

void PaperTranslatorWidget::onTranslate() {
    QString src = sourceEdit_->toPlainText().trimmed();
    if (src.isEmpty()) {
        statusLabel_->setText("Enter text to translate");
        return;
    }

    QString srcLang = srcLangCombo_->currentText();
    QString tgtLang = tgtLangCombo_->currentText();

    if (srcLang == "Auto Detect") {
        srcLang = detectLanguage(src);
        int idx = srcLangCombo_->findText(srcLang);
        if (idx < 0) srcLang = "English";
    }

    statusLabel_->setText("Translating...");

    // Placeholder: simulate translation
    QString result = simulateTranslation(src, srcLang, tgtLang);
    resultEdit_->setPlainText(result);

    TranslationEntry entry;
    entry.id = nextId_++;
    entry.sourceText = src;
    entry.translatedText = result;
    entry.sourceLang = srcLang;
    entry.targetLang = tgtLang;
    entry.timestamp = QDateTime::currentSecsSinceEpoch();
    history_.prepend(entry);
    saveHistory();
    refreshHistory();

    statusLabel_->setText(QString("Translated %1 → %2 (%3 chars)")
        .arg(srcLang, tgtLang).arg(result.length()));
    emit translationCompleted(src, result);
}

void PaperTranslatorWidget::onSwap() {
    int si = srcLangCombo_->currentIndex();
    int ti = tgtLangCombo_->currentIndex();
    if (si == 0) si = 1; // skip "Auto Detect"
    srcLangCombo_->setCurrentIndex(ti);
    tgtLangCombo_->setCurrentIndex(si);

    QString srcText = sourceEdit_->toPlainText();
    QString resText = resultEdit_->toPlainText();
    sourceEdit_->setPlainText(resText);
    resultEdit_->setPlainText(srcText);
}

void PaperTranslatorWidget::onCopy() {
    QString text = resultEdit_->toPlainText();
    if (!text.isEmpty()) {
        QApplication::clipboard()->setText(text);
        statusLabel_->setText("Copied to clipboard");
    }
}

void PaperTranslatorWidget::onClear() {
    sourceEdit_->clear();
    resultEdit_->clear();
    statusLabel_->setText("Cleared");
}

void PaperTranslatorWidget::onHistoryClicked(QListWidgetItem* item) {
    if (!item) return;
    int id = item->data(Qt::UserRole).toInt();
    for (const auto& e : history_) {
        if (e.id == id) {
            sourceEdit_->setPlainText(e.sourceText);
            resultEdit_->setPlainText(e.translatedText);
            statusLabel_->setText(QString("Loaded from history: %1 → %2").arg(e.sourceLang, e.targetLang));
            break;
        }
    }
}

void PaperTranslatorWidget::onSourceLangChanged(int) {
    emit languagePairChanged(srcLangCombo_->currentText(), tgtLangCombo_->currentText());
}

void PaperTranslatorWidget::onTargetLangChanged(int) {
    emit languagePairChanged(srcLangCombo_->currentText(), tgtLangCombo_->currentText());
}

QString PaperTranslatorWidget::detectLanguage(const QString& text) const {
    int cjk = 0, latin = 0;
    for (const QChar& ch : text) {
        ushort code = ch.unicode();
        if ((code >= 0x4E00 && code <= 0x9FFF) || (code >= 0x3400 && code <= 0x4DBF))
            cjk++;
        else if ((code >= 'A' && code <= 'Z') || (code >= 'a' && code <= 'z'))
            latin++;
    }
    if (cjk > latin) {
        // Check for Japanese kana
        for (const QChar& ch : text) {
            ushort code = ch.unicode();
            if ((code >= 0x3040 && code <= 0x309F) || (code >= 0x30A0 && code <= 0x30FF))
                return "Japanese";
        }
        return "Chinese";
    }
    return "English";
}

QString PaperTranslatorWidget::simulateTranslation(const QString& text, const QString& src, const QString& tgt) {
    // Placeholder simulation — in production, call real translation API
    Q_UNUSED(src);
    QString prefix = (tgt == "Chinese") ? "[译文] " : "[Translation] ";
    if (tgt == "Chinese" && src == "English") {
        return prefix + "（翻译占位）原文: " + text.left(200);
    }
    return prefix + "(Translation placeholder) Source: " + text.left(200);
}

void PaperTranslatorWidget::loadHistory() {
    QSettings settings("PaperCrawler", "TranslationHistory");
    QByteArray data = settings.value("history").toByteArray();
    if (data.isEmpty()) return;

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        TranslationEntry e;
        e.id = obj["id"].toInt();
        e.sourceText = obj["sourceText"].toString();
        e.translatedText = obj["translatedText"].toString();
        e.sourceLang = obj["sourceLang"].toString();
        e.targetLang = obj["targetLang"].toString();
        e.timestamp = obj["timestamp"].toInteger();
        history_.append(e);
        nextId_ = qMax(nextId_, e.id + 1);
    }
    refreshHistory();
}

void PaperTranslatorWidget::saveHistory() {
    QJsonArray arr;
    for (const auto& e : history_) {
        QJsonObject obj;
        obj["id"] = e.id;
        obj["sourceText"] = e.sourceText;
        obj["translatedText"] = e.translatedText;
        obj["sourceLang"] = e.sourceLang;
        obj["targetLang"] = e.targetLang;
        obj["timestamp"] = static_cast<qint64>(e.timestamp);
        arr.append(obj);
    }
    QSettings settings("PaperCrawler", "TranslationHistory");
    settings.setValue("history", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

void PaperTranslatorWidget::refreshHistory() {
    historyList_->clear();
    int limit = qMin(history_.size(), 20);
    for (int i = 0; i < limit; ++i) {
        const auto& e = history_[i];
        QDateTime dt = QDateTime::fromSecsSinceEpoch(e.timestamp);
        QString display = QString("%1 → %2 | %3 | %4")
            .arg(e.sourceLang, e.targetLang,
                 dt.toString("MM-dd HH:mm"),
                 e.sourceText.left(40).replace("\n", " "));
        auto* item = new QListWidgetItem(display);
        item->setData(Qt::UserRole, e.id);
        historyList_->addItem(item);
    }
}
