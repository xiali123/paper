#include "analysis/PaperSentimentExplorer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperSentimentExplorer::PaperSentimentExplorer(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "SentimentExplorer")
{
    setupUI();
    loadSettings();
}

void PaperSentimentExplorer::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    exploreBtn_ = new QPushButton("Explore");
    exploreBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(exploreBtn_, &QPushButton::clicked, this, &PaperSentimentExplorer::onExplore);
    toolbar->addWidget(exploreBtn_);
    toolbar->addWidget(new QLabel("Sentiment:"));
    sentimentCombo_ = new QComboBox();
    sentimentCombo_->addItems({"All", "Positive", "Neutral", "Negative"});
    toolbar->addWidget(sentimentCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperSentimentExplorer::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter text for sentiment analysis...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Explore sentiment patterns");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperSentimentExplorer::addEntry(const SentimentEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit sentimentExplored(entry.id, entry.score);
    update();
}

QList<SentimentEntry> PaperSentimentExplorer::entries() const { return entries_; }

qreal PaperSentimentExplorer::avgScore() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.score;
    return sum / entries_.size();
}

int PaperSentimentExplorer::positiveCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.positive) c++;
    return c;
}

QMap<QString, int> PaperSentimentExplorer::sentimentCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.sentiment]++;
    return counts;
}

void PaperSentimentExplorer::onExplore() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList sentiments = {"positive", "neutral", "negative"};
    QStringList emotions = {"joy", "trust", "anticipation", "fear", "sadness", "surprise"};
    QStringList sections = {"abstract", "intro", "method", "results", "conclusion"};
    int sIdx = sentimentCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(5);
    for (int i = 0; i < count; ++i) {
        SentimentEntry e;
        e.id = entries_.size() + 1;
        e.text = text.left(8) + " seg" + QString::number(i);
        e.sentiment = sIdx == 0 ? sentiments[QRandomGenerator::global()->bounded(sentiments.size())] : sentiments[sIdx - 1];
        e.score = -1 + QRandomGenerator::global()->bounded(200) / 100.0;
        e.emotion = emotions[QRandomGenerator::global()->bounded(emotions.size())];
        e.section = sections[QRandomGenerator::global()->bounded(sections.size())];
        e.magnitude = 0.1 + QRandomGenerator::global()->bounded(90) / 100.0;
        e.positive = e.score > 0.2;
        e.color = e.positive ? QColor(16,185,129) : (e.score < -0.2 ? QColor(239,68,68) : QColor(156,163,175));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperSentimentExplorer::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Explore sentiment patterns");
    update();
}

void PaperSentimentExplorer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Explore sentiment patterns");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Sentiment Explorer");
    int w = width(), h = height();
    drawSentimentList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawSentimentChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperSentimentExplorer::drawSentimentList(QPainter& p, const QRect& rect) {
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
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.text.left(14) + " [" + e.emotion.left(4) + "]");
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.section + " | mag " + QString::number(e.magnitude, 'f', 2));
        int barW = static_cast<int>(((e.score + 1) / 2.0) * (rect.width() / 2 - 20));
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x() + rect.width() / 2, y + 10, barW, 14, 3, 3);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(rect.x() + rect.width() / 2 + barW + 4, y + 22,
                   QString::number(e.score, 'f', 2));
    }
}

void PaperSentimentExplorer::drawSentimentChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Sentiment");
    auto counts = sentimentCounts();
    QStringList sentiments = {"positive", "neutral", "negative"};
    QString labels[] = {"Positive", "Neutral", "Negative"};
    QColor colors[] = {QColor(16,185,129), QColor(156,163,175), QColor(239,68,68)};
    int total = entries_.size();
    int pieW = qMin(rect.width(), rect.height() - 50);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 25 + pieW / 2;
    qreal startAngle = 0;
    for (int i = 0; i < 3; ++i) {
        int count = counts.contains(sentiments[i]) ? counts[sentiments[i]] : 0;
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

void PaperSentimentExplorer::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Segments", QString::number(entries_.size()), QColor(59,130,246)},
        {"Positive", QString::number(positiveCount()), QColor(16,185,129)},
        {"Avg Score", QString::number(avgScore(), 'f', 2), QColor(245,158,11)},
        {"Sentiments", QString::number(sentimentCounts().size()), QColor(139,92,246)}
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

void PaperSentimentExplorer::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Explore sentiment patterns"); return; }
    infoLabel_->setText(QString("%1 segments | %2 positive | %3 avg")
        .arg(entries_.size()).arg(positiveCount()).arg(avgScore(), 0, 'f', 2));
}

void PaperSentimentExplorer::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        SentimentEntry e;
        e.id = settings_.value("id").toInt();
        e.text = settings_.value("text").toString();
        e.sentiment = settings_.value("sentiment").toString();
        e.score = settings_.value("score").toDouble();
        e.emotion = settings_.value("emotion").toString();
        e.section = settings_.value("section").toString();
        e.magnitude = settings_.value("magnitude").toDouble();
        e.positive = settings_.value("positive").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperSentimentExplorer::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("text", entries_[i].text);
        settings_.setValue("sentiment", entries_[i].sentiment);
        settings_.setValue("score", entries_[i].score);
        settings_.setValue("emotion", entries_[i].emotion);
        settings_.setValue("section", entries_[i].section);
        settings_.setValue("magnitude", entries_[i].magnitude);
        settings_.setValue("positive", entries_[i].positive);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
