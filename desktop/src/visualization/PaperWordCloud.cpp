#include "visualization/PaperWordCloud.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperWordCloud::PaperWordCloud(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "WordCloud")
{
    setupUI();
    loadSettings();
}

void PaperWordCloud::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperWordCloud::onGenerate);
    toolbar->addWidget(generateBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Keywords", "Topics", "Entities"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperWordCloud::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter text for word cloud...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Generate word cloud");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(600, 500);
}

void PaperWordCloud::addEntry(const WordCloudEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit wordCloudGenerated(entry.id, entry.frequency);
    update();
}

QList<WordCloudEntry> PaperWordCloud::entries() const { return entries_; }

int PaperWordCloud::totalFrequency() const {
    int t = 0;
    for (const auto& e : entries_) t += e.frequency;
    return t;
}

int PaperWordCloud::highlightedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.highlighted) c++;
    return c;
}

QMap<QString, int> PaperWordCloud::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperWordCloud::onGenerate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"keywords", "topics", "entities"};
    QStringList words = {"neural", "transformer", "attention", "gradient", "embedding", "convolution",
                         "recurrent", "generative", "discriminative", "reinforcement", "optimization",
                         "regularization", "normalization", "activation", "backpropagation"};
    int cIdx = categoryCombo_->currentIndex();
    entries_.clear();
    int count = 8 + QRandomGenerator::global()->bounded(8);
    for (int i = 0; i < count; ++i) {
        WordCloudEntry e;
        e.id = entries_.size() + 1;
        e.word = words[i % words.size()];
        e.frequency = 5 + QRandomGenerator::global()->bounded(100);
        e.weight = static_cast<qreal>(e.frequency) / 100.0;
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.rank = i + 1;
        e.tfidf = 0.1 + QRandomGenerator::global()->bounded(50) / 10.0;
        e.highlighted = e.frequency >= 60;
        e.color = e.highlighted ? QColor(239,68,68) : (e.frequency >= 30 ? QColor(59,130,246) : QColor(156,163,175));
        entries_.append(e);
    }
    saveSettings();
    updateInfo();
    emit wordCloudGenerated(entries_.size(), totalFrequency());
    update();
    inputField_->clear();
}

void PaperWordCloud::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Generate word cloud");
    update();
}

void PaperWordCloud::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Generate word cloud");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Word Cloud");
    int w = width(), h = height();
    drawCloudView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperWordCloud::drawCloudView(QPainter& p, const QRect& rect) {
    int n = entries_.size();
    int maxFreq = 1;
    for (const auto& e : entries_) maxFreq = qMax(maxFreq, e.frequency);
    int cols = qMax(1, static_cast<int>(qSqrt(n)));
    int rows = (n + cols - 1) / cols;
    int cellW = (rect.width() - 10) / cols;
    int cellH = (rect.height() - 10) / rows;
    for (int i = 0; i < n; ++i) {
        const auto& e = entries_[i];
        int col = i % cols;
        int row = i / cols;
        qreal ratio = static_cast<qreal>(e.frequency) / maxFreq;
        int fontSize = qMax(8, qMin(20, static_cast<int>(ratio * 20)));
        int x = rect.x() + 5 + col * cellW + static_cast<int>((1 - ratio) * cellW * 0.15);
        int y = rect.y() + 5 + row * cellH + static_cast<int>((1 - ratio) * cellH * 0.15);
        p.setPen(e.color);
        p.setFont(QFont("Arial", fontSize, e.highlighted ? QFont::Bold : QFont::Normal));
        p.drawText(x, y, cellW, cellH, Qt::AlignCenter, e.word);
    }
}

void PaperWordCloud::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"keywords", "topics", "entities"};
    QString labels[] = {"Keywords", "Topics", "Entities"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11)};
    int itemH = qMin(28, (rect.height() - 30) / 3);
    for (int i = 0; i < 3; ++i) {
        int y = rect.y() + 22 + i * (itemH + 4);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 5, y + 4, 14, 14, 3, 3);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18, Qt::AlignVCenter, labels[i]);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(count) + " words");
    }
}

void PaperWordCloud::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Words", QString::number(entries_.size()), QColor(59,130,246)},
        {"Highlighted", QString::number(highlightedCount()), QColor(239,68,68)},
        {"Total Freq", QString::number(totalFrequency()), QColor(245,158,11)},
        {"Categories", QString::number(categoryCounts().size()), QColor(139,92,246)}
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

void PaperWordCloud::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Generate word cloud"); return; }
    infoLabel_->setText(QString("%1 words | %2 highlighted | %3 freq")
        .arg(entries_.size()).arg(highlightedCount()).arg(totalFrequency()));
}

void PaperWordCloud::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        WordCloudEntry e;
        e.id = settings_.value("id").toInt();
        e.word = settings_.value("word").toString();
        e.frequency = settings_.value("frequency").toInt();
        e.weight = settings_.value("weight").toDouble();
        e.category = settings_.value("category").toString();
        e.rank = settings_.value("rank").toInt();
        e.tfidf = settings_.value("tfidf").toDouble();
        e.highlighted = settings_.value("highlighted").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperWordCloud::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("word", entries_[i].word);
        settings_.setValue("frequency", entries_[i].frequency);
        settings_.setValue("weight", entries_[i].weight);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("rank", entries_[i].rank);
        settings_.setValue("tfidf", entries_[i].tfidf);
        settings_.setValue("highlighted", entries_[i].highlighted);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
