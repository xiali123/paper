#include "analysis/PaperTopicSentiment2.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>

PaperTopicSentiment2::PaperTopicSentiment2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "TopicSentiment2")
{
    setupUI();
    loadSettings();
}

void PaperTopicSentiment2::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Research", "Review", "Survey", "Case Study", "Opinion"});
    categoryCombo_->setStyleSheet("QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter topic...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    analyzeBtn_ = new QPushButton("Analyze");
    analyzeBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(analyzeBtn_, &QPushButton::clicked, this, &PaperTopicSentiment2::onAnalyze);
    toolbar->addWidget(analyzeBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperTopicSentiment2::onClear);
    toolbar->addWidget(clearBtn_);

    mainLayout->addLayout(toolbar);

    infoLabel_ = new QLabel("Topics: 0 | Positive: 0 | Avg Intensity: 0.00");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    mainLayout->addWidget(infoLabel_);

    mainLayout->addStretch(1);
    setMinimumSize(640, 520);
}

void PaperTopicSentiment2::addEntry(const Sentiment2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit sentimentMapped(entry.id, entry.intensity);
    update();
}

QList<Sentiment2Entry> PaperTopicSentiment2::entries() const { return entries_; }

int PaperTopicSentiment2::positiveCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.positive) c++;
    return c;
}

qreal PaperTopicSentiment2::avgIntensity() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.intensity;
    return sum / entries_.size();
}

QMap<QString, int> PaperTopicSentiment2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperTopicSentiment2::onAnalyze() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    static const QStringList categories = {"research", "review", "survey", "case_study", "opinion"};
    static const QStringList emotions = {"joy", "trust", "anticipation", "surprise", "fear", "sadness", "disgust", "anger"};
    static const QColor palette[] = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };

    int cIdx = categoryCombo_->currentIndex();
    QString cat = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())]
                            : categories[cIdx - 1];

    qreal intensity = QRandomGenerator::global()->bounded(1000) / 1000.0;
    int mentions = 1 + QRandomGenerator::global()->bounded(500);
    bool positive = intensity > 0.5;
    QString emotion = emotions[QRandomGenerator::global()->bounded(emotions.size())];
    QColor color = positive ? QColor("#16a34a") : QColor("#dc2626");

    Sentiment2Entry e;
    e.id = entries_.size() + 1;
    e.topic = text;
    e.category = cat;
    e.emotion = emotion;
    e.intensity = intensity;
    e.mentions = mentions;
    e.positive = positive;
    e.color = color;

    addEntry(e);
    inputField_->clear();
}

void PaperTopicSentiment2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperTopicSentiment2::paintEvent(QPaintEvent*) {
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

    drawSentimentMap(p, QRect(20, 10, colW, h - 20));
    drawCategoryChart(p, QRect(30 + colW, 10, colW, h - 20));
    drawStats(p, QRect(40 + 2 * colW, 10, colW, h - 20));
}

void PaperTopicSentiment2::drawSentimentMap(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 20, "Sentiment Map");

    int show = qMin(12, entries_.size());
    int chartTop = rect.y() + 35;
    int chartH = rect.height() - 45;
    if (show == 0) return;

    qreal step = static_cast<qreal>(chartH) / show;
    int midX = rect.x() + rect.width() / 2;
    int maxBarW = rect.width() / 2 - 40;

    p.setPen(QColor(203, 213, 225));
    p.drawLine(midX, chartTop, midX, chartTop + static_cast<int>(show * step));

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        qreal cy = chartTop + i * step + step / 2;
        int barW = static_cast<int>(e.intensity * maxBarW);

        QColor barColor = e.positive ? QColor("#16a34a") : QColor("#dc2626");
        barColor.setAlpha(180);
        p.setPen(Qt::NoPen);
        p.setBrush(barColor);

        if (e.positive) {
            p.drawRoundedRect(midX, static_cast<int>(cy - step / 2 + 4), barW, static_cast<int>(step - 8), 3, 3);
        } else {
            p.drawRoundedRect(midX - barW, static_cast<int>(cy - step / 2 + 4), barW, static_cast<int>(step - 8), 3, 3);
        }

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        QString label = e.topic.length() > 12 ? e.topic.left(12) + ".." : e.topic;
        p.drawText(rect.x(), static_cast<int>(cy - step / 2 + 4), midX - rect.x() - 8, static_cast<int>(step - 8),
                   Qt::AlignRight | Qt::AlignVCenter, label);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(midX + maxBarW + 4, static_cast<int>(cy - step / 2 + 4), 40, static_cast<int>(step - 8),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QString::number(e.intensity, 'f', 2) + " | " + e.emotion.left(4));
    }
}

void PaperTopicSentiment2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 20, "Categories");

    auto counts = categoryCounts();
    if (counts.isEmpty()) return;

    int maxVal = 1;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        maxVal = qMax(maxVal, it.value());

    static const QColor palette[] = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };

    int idx = 0;
    int barCount = counts.size();
    int areaTop = rect.y() + 35;
    int barH = qMin(28, (rect.height() - 40) / qMax(barCount, 1));
    int labelW = 70;

    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it, ++idx) {
        int y = areaTop + idx * (barH + 4);
        int barW = static_cast<int>((static_cast<qreal>(it.value()) / maxVal) * (rect.width() - labelW - 40));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x(), y, labelW, barH, Qt::AlignRight | Qt::AlignVCenter, it.key());

        p.setPen(Qt::NoPen);
        p.setBrush(palette[idx % 5]);
        p.drawRoundedRect(rect.x() + labelW + 5, y + 3, barW, barH - 6, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + labelW + 10 + barW, y + barH - 6, QString::number(it.value()));
    }
}

void PaperTopicSentiment2::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 20, "Statistics");

    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total", QString::number(entries_.size()), QColor("#3b82f6")},
        {"Positive", QString::number(positiveCount()), QColor("#16a34a")},
        {"Avg Intensity", QString::number(avgIntensity(), 'f', 2), QColor("#d97706")}
    };

    int boxH = qMin(52, (rect.height() - 45) / qMax(stats.size(), 1));

    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + 35 + i * (boxH + 6);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 16, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() - 20, 26, Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 30, rect.width() - 20, 16, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperTopicSentiment2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Topics: 0 | Positive: 0 | Avg Intensity: 0.00");
        return;
    }
    infoLabel_->setText(QString("Topics: %1 | Positive: %2 | Avg Intensity: %3")
        .arg(entries_.size())
        .arg(positiveCount())
        .arg(avgIntensity(), 0, 'f', 2));
}

void PaperTopicSentiment2::loadSettings() {
    settings_.beginGroup("TopicSentiment2");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        Sentiment2Entry e;
        e.id = settings_.value("id").toInt();
        e.topic = settings_.value("topic").toString();
        e.category = settings_.value("category").toString();
        e.emotion = settings_.value("emotion").toString();
        e.intensity = settings_.value("intensity").toDouble();
        e.mentions = settings_.value("mentions").toInt();
        e.positive = settings_.value("positive").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperTopicSentiment2::saveSettings() {
    settings_.beginGroup("TopicSentiment2");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("topic", entries_[i].topic);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("emotion", entries_[i].emotion);
        settings_.setValue("intensity", entries_[i].intensity);
        settings_.setValue("mentions", entries_[i].mentions);
        settings_.setValue("positive", entries_[i].positive);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
