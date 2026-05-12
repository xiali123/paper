#include "analysis/PaperTopicSentiment.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>

PaperTopicSentiment::PaperTopicSentiment(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "TopicSentiment")
{
    setupUI();
    loadSettings();
}

void PaperTopicSentiment::setupUI() {
    auto* layout = new QHBoxLayout(this);
    auto* left = new QWidget();
    auto* leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Science", "Technology", "Health", "Politics", "Business"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    leftLayout->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter topic...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    leftLayout->addWidget(inputField_);

    auto* btnRow = new QHBoxLayout();
    analyzeBtn_ = new QPushButton("Analyze");
    analyzeBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(analyzeBtn_, &QPushButton::clicked, this, &PaperTopicSentiment::onAnalyze);
    btnRow->addWidget(analyzeBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperTopicSentiment::onClear);
    btnRow->addWidget(clearBtn_);
    leftLayout->addLayout(btnRow);

    infoLabel_ = new QLabel("Topics: 0 | Positive: 0 | Avg: 0.00");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    leftLayout->addWidget(infoLabel_);

    layout->addWidget(left, 0);
    layout->addStretch(1);
    setMinimumSize(640, 520);
}

void PaperTopicSentiment::addEntry(const TopicSentimentEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit sentimentAnalyzed(entry.id, entry.score);
    update();
}

QList<TopicSentimentEntry> PaperTopicSentiment::entries() const { return entries_; }

int PaperTopicSentiment::positiveCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.positive) c++;
    return c;
}

qreal PaperTopicSentiment::avgScore() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.score;
    return sum / entries_.size();
}

QMap<QString, int> PaperTopicSentiment::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperTopicSentiment::onAnalyze() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"science", "technology", "health", "politics", "business"};
    int cIdx = categoryCombo_->currentIndex();
    QString cat = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())]
                             : categories[cIdx - 1];

    qreal score = QRandomGenerator::global()->bounded(2000) / 1000.0 - 1.0;
    int mentions = 1 + QRandomGenerator::global()->bounded(500);
    bool positive = score > 0;
    QString sentiment = score > 0 ? "positive" : (score < 0 ? "negative" : "neutral");

    QColor palette[] = {QColor(59, 130, 246), QColor(22, 163, 74), QColor(217, 119, 6),
                        QColor(220, 38, 38), QColor(124, 58, 237)};
    QColor color = positive ? QColor(22, 163, 74)
                            : (score < 0 ? QColor(220, 38, 38)
                                         : QColor(217, 119, 6));

    TopicSentimentEntry e;
    e.id = entries_.size() + 1;
    e.topic = text;
    e.category = cat;
    e.sentiment = sentiment;
    e.score = score;
    e.mentions = mentions;
    e.positive = positive;
    e.color = color;
    addEntry(e);
    inputField_->clear();
}

void PaperTopicSentiment::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperTopicSentiment::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Analyze topic sentiment");
        return;
    }

    int w = width(), h = height();
    int colW = (w - 60) / 3;
    drawSentimentChart(p, QRect(20, 20, colW, h - 40));
    drawCategoryChart(p, QRect(30 + colW, 20, colW, h - 40));
    drawStats(p, QRect(40 + 2 * colW, 20, colW, h - 40));
}

void PaperTopicSentiment::drawSentimentChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 18, "Topic Sentiment");

    int show = qMin(12, entries_.size());
    int chartTop = rect.y() + 35;
    int chartH = rect.height() - 55;
    int barH = qMin(28, (chartH - 10) / qMax(show, 1));
    int midX = rect.x() + rect.width() / 2;
    int maxBarW = rect.width() / 2 - 40;

    // Center line
    p.setPen(QColor(203, 213, 225));
    p.drawLine(midX, chartTop, midX, chartTop + show * (barH + 4));

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = chartTop + i * (barH + 4);
        int barW = static_cast<int>(qAbs(e.score) * maxBarW);

        if (e.positive) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(22, 163, 74));
            p.drawRoundedRect(midX, y + 2, barW, barH - 4, 3, 3);
        } else {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(220, 38, 38));
            p.drawRoundedRect(midX - barW, y + 2, barW, barH - 4, 3, 3);
        }

        // Topic label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        QString label = e.topic.length() > 14 ? e.topic.left(14) + ".." : e.topic;
        p.drawText(rect.x(), y + 2, midX - rect.x() - 8, barH - 4,
                   Qt::AlignRight | Qt::AlignVCenter, label);

        // Score label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(midX + maxBarW + 4, y + 2, 36, barH - 4,
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QString::number(e.score, 'f', 2));
    }
}

void PaperTopicSentiment::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 18, "Categories");

    auto counts = categoryCounts();
    QStringList categories = {"science", "technology", "health", "politics", "business"};
    QString labels[] = {"Science", "Tech", "Health", "Politics", "Business"};
    QColor colors[] = {QColor(59, 130, 246), QColor(22, 163, 74), QColor(217, 119, 6),
                       QColor(220, 38, 38), QColor(124, 58, 237)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int chartTop = rect.y() + 35;
    int barH = qMin(26, (rect.height() - 50) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = chartTop + i * (barH + 6);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 100));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + 2, 55, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 60, y + 2, barW, barH - 4, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 63 + barW, y + 2, 40, barH - 4,
                   Qt::AlignLeft | Qt::AlignVCenter, QString::number(count));
    }
}

void PaperTopicSentiment::drawStats(QPainter& p, const QRect& rect) {
    struct Stat {
        QString label;
        QString value;
        QColor color;
    };
    QList<Stat> stats = {
        {"Total", QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Positive", QString::number(positiveCount()), QColor(22, 163, 74)},
        {"Avg Score", QString::number(avgScore(), 'f', 2), QColor(217, 119, 6)},
    };

    int boxH = qMin(48, (rect.height() - 20) / 3);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + 10 + i * (boxH + 8);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 22,
                   Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 28, rect.width() - 20, 14,
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperTopicSentiment::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Topics: 0 | Positive: 0 | Avg: 0.00");
        return;
    }
    infoLabel_->setText(QString("Topics: %1 | Positive: %2 | Avg: %3")
                            .arg(entries_.size())
                            .arg(positiveCount())
                            .arg(avgScore(), 0, 'f', 2));
}

void PaperTopicSentiment::loadSettings() {
    settings_.beginGroup("TopicSentiment");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        TopicSentimentEntry e;
        e.id = settings_.value("id").toInt();
        e.topic = settings_.value("topic").toString();
        e.category = settings_.value("category").toString();
        e.sentiment = settings_.value("sentiment").toString();
        e.score = settings_.value("score").toDouble();
        e.mentions = settings_.value("mentions").toInt();
        e.positive = settings_.value("positive").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperTopicSentiment::saveSettings() {
    settings_.beginGroup("TopicSentiment");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("topic", entries_[i].topic);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("sentiment", entries_[i].sentiment);
        settings_.setValue("score", entries_[i].score);
        settings_.setValue("mentions", entries_[i].mentions);
        settings_.setValue("positive", entries_[i].positive);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
