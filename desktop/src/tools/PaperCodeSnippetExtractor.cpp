#include "tools/PaperCodeSnippetExtractor.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperCodeSnippetExtractor::PaperCodeSnippetExtractor(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CodeSnippetExtractor")
{
    setupUI();
    loadSettings();
}

void PaperCodeSnippetExtractor::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    extractBtn_ = new QPushButton("Extract");
    extractBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(extractBtn_, &QPushButton::clicked, this, &PaperCodeSnippetExtractor::onExtract);
    toolbar->addWidget(extractBtn_);

    toolbar->addWidget(new QLabel("Language:"));
    langCombo_ = new QComboBox();
    langCombo_->addItems({"Python", "MATLAB", "R", "C++", "Java"});
    toolbar->addWidget(langCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCodeSnippetExtractor::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter paper title to extract code snippets...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Extract code snippets from papers");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperCodeSnippetExtractor::addEntry(const SnippetEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit snippetExtracted(entry.id, entry.language);
    update();
}

QList<SnippetEntry> PaperCodeSnippetExtractor::entries() const { return entries_; }

int PaperCodeSnippetExtractor::runnableCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.runnable) c++;
    return c;
}

qreal PaperCodeSnippetExtractor::avgConfidence() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.confidence;
    return sum / entries_.size();
}

QMap<QString, int> PaperCodeSnippetExtractor::languageCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.language]++;
    return counts;
}

void PaperCodeSnippetExtractor::onExtract() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList languages = {"python", "matlab", "r", "cpp", "java"};
    QStringList categories = {"algorithm", "preprocessing", "model", "evaluation", "visualization"};
    QStringList algorithms = {"CNN", "LSTM", "Transformer", "Random Forest", "SVM"};
    QStringList snippets = {"def train_model(data):", "function [out] = process(in)", "model <- lm(y~x)",
                            "class Model { public:", "public static void main("};
    QColor langColors[] = {QColor(59,130,246), QColor(245,158,11), QColor(16,185,129), QColor(239,68,68), QColor(139,92,246)};

    int lIdx = langCombo_->currentIndex();
    int count = 2 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        SnippetEntry e;
        e.id = entries_.size() + 1;
        e.paperTitle = text.left(15);
        e.language = languages[lIdx];
        e.snippet = snippets[lIdx];
        e.lineCount = 5 + QRandomGenerator::global()->bounded(50);
        e.confidence = 0.4 + QRandomGenerator::global()->bounded(60) / 100.0;
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.algorithm = algorithms[QRandomGenerator::global()->bounded(algorithms.size())];
        e.runnable = e.confidence >= 0.7 && QRandomGenerator::global()->bounded(2) == 0;
        e.figureNum = 1 + QRandomGenerator::global()->bounded(10);
        e.color = langColors[lIdx];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperCodeSnippetExtractor::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Extract code snippets from papers");
    update();
}

void PaperCodeSnippetExtractor::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Extract code snippets from papers");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Code Snippet Extractor");

    int w = width(), h = height();
    drawSnippetList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawLanguageChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperCodeSnippetExtractor::drawSnippetList(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(34, (rect.height() - 10) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Courier", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.snippet.left(20));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + e.algorithm + (e.runnable ? " | run" : ""));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.language + " | " + QString::number(e.lineCount) + " lines");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.confidence * 100, 'f', 0) + "% conf | Fig " + QString::number(e.figureNum));
    }
}

void PaperCodeSnippetExtractor::drawLanguageChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Languages");

    auto counts = languageCounts();
    QStringList langs = {"python", "matlab", "r", "cpp", "java"};
    QString labels[] = {"Python", "MATLAB", "R", "C++", "Java"};
    QColor colors[] = {QColor(59,130,246), QColor(245,158,11), QColor(16,185,129), QColor(239,68,68), QColor(139,92,246)};

    int total = entries_.size();
    int pieW = qMin(rect.width(), rect.height() - 50);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 25 + pieW / 2;

    qreal startAngle = 0;
    for (int i = 0; i < 5; ++i) {
        int count = counts.contains(langs[i]) ? counts[langs[i]] : 0;
        qreal span = (static_cast<qreal>(count) / qMax(total, 1)) * 360;
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawPie(cx - pieW / 2, cy - pieW / 2, pieW, pieW,
                  static_cast<int>(startAngle * 16), static_cast<int>(span * 16));
        startAngle += span;
    }

    p.setBrush(Qt::white);
    p.drawEllipse(cx - pieW / 4, cy - pieW / 4, pieW / 2, pieW / 2);

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(cx - 10, cy + 5, QString::number(total));
}

void PaperCodeSnippetExtractor::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Snippets", QString::number(entries_.size()), QColor(59,130,246)},
        {"Runnable", QString::number(runnableCount()), QColor(16,185,129)},
        {"Avg Conf", QString::number(avgConfidence() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Languages", QString::number(languageCounts().size()), QColor(139,92,246)}
    };

    int boxH = qMin(42, (rect.height() - 10) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 22, Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 26, rect.width() - 20, 14, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperCodeSnippetExtractor::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Extract code snippets from papers"); return; }
    infoLabel_->setText(QString("%1 snippets | %2 runnable | %3% conf")
        .arg(entries_.size()).arg(runnableCount()).arg(avgConfidence() * 100, 0, 'f', 0));
}

void PaperCodeSnippetExtractor::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        SnippetEntry e;
        e.id = settings_.value("id").toInt();
        e.paperTitle = settings_.value("paperTitle").toString();
        e.language = settings_.value("language").toString();
        e.snippet = settings_.value("snippet").toString();
        e.lineCount = settings_.value("lineCount").toInt();
        e.confidence = settings_.value("confidence").toDouble();
        e.category = settings_.value("category").toString();
        e.algorithm = settings_.value("algorithm").toString();
        e.runnable = settings_.value("runnable").toBool();
        e.figureNum = settings_.value("figureNum").toInt();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperCodeSnippetExtractor::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("paperTitle", entries_[i].paperTitle);
        settings_.setValue("language", entries_[i].language);
        settings_.setValue("snippet", entries_[i].snippet);
        settings_.setValue("lineCount", entries_[i].lineCount);
        settings_.setValue("confidence", entries_[i].confidence);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("algorithm", entries_[i].algorithm);
        settings_.setValue("runnable", entries_[i].runnable);
        settings_.setValue("figureNum", entries_[i].figureNum);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}

