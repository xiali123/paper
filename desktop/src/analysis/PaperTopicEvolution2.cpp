#include "analysis/PaperTopicEvolution2.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>

PaperTopicEvolution2::PaperTopicEvolution2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "TopicEvolution2")
{
    setupUI();
    loadSettings();
}

void PaperTopicEvolution2::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "AI/ML", "NLP", "Vision", "Security", "Systems", "Theory"});
    categoryCombo_->setStyleSheet("QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Topic name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    trackBtn_ = new QPushButton("Track");
    trackBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(trackBtn_, &QPushButton::clicked, this, &PaperTopicEvolution2::onTrack);
    toolbar->addWidget(trackBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperTopicEvolution2::onClear);
    toolbar->addWidget(clearBtn_);

    mainLayout->addLayout(toolbar);

    infoLabel_ = new QLabel("Track topic evolution");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    mainLayout->addWidget(infoLabel_);

    mainLayout->addStretch(1);
    setMinimumSize(640, 520);
}

void PaperTopicEvolution2::addEntry(const TopicEvoEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit evolutionTracked(entry.id, entry.popularity);
    update();
}

QList<TopicEvoEntry> PaperTopicEvolution2::entries() const { return entries_; }

int PaperTopicEvolution2::trendingCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.trending) c++;
    return c;
}

qreal PaperTopicEvolution2::avgPopularity() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.popularity;
    return sum / entries_.size();
}

QMap<QString, int> PaperTopicEvolution2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperTopicEvolution2::onTrack() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    static const QStringList eras = {"2010s", "2015s", "2020s", "2025s"};
    static const QStringList categories = {"AI/ML", "NLP", "Vision", "Security", "Systems", "Theory"};
    static const QColor palette[] = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };

    TopicEvoEntry e;
    e.id = entries_.size() + 1;
    e.topic = text;
    e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
    e.popularity = QRandomGenerator::global()->bounded(101);
    e.papers = 1 + QRandomGenerator::global()->bounded(200);
    e.trending = e.popularity > 70;
    e.era = eras[QRandomGenerator::global()->bounded(eras.size())];
    e.color = palette[QRandomGenerator::global()->bounded(5)];

    addEntry(e);
    inputField_->clear();
}

void PaperTopicEvolution2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperTopicEvolution2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Track topic evolution");
        return;
    }

    int w = width(), h = height();
    int colW = (w - 60) / 3;

    drawEvolutionChart(p, QRect(20, 10, colW, h - 20));
    drawCategoryChart(p, QRect(30 + colW, 10, colW, h - 20));
    drawStats(p, QRect(40 + 2 * colW, 10, colW, h - 20));
}

void PaperTopicEvolution2::drawEvolutionChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 20, "Topic Evolution");

    int show = qMin(12, entries_.size());
    int areaTop = rect.y() + 35;
    int areaH = rect.height() - 45;
    if (show == 0) return;

    qreal step = static_cast<qreal>(areaH) / show;

    // Draw era-colored timeline band in background
    static const QMap<QString, QColor> eraColors = {
        {"2010s", QColor("#3b82f6")},
        {"2015s", QColor("#16a34a")},
        {"2020s", QColor("#d97706")},
        {"2025s", QColor("#dc2626")}
    };

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        qreal cy = areaTop + i * step + step / 2;

        // Era background stripe
        QColor eraBg = eraColors.contains(e.era) ? eraColors[e.era] : QColor("#7c3aed");
        eraBg.setAlpha(30);
        p.setPen(Qt::NoPen);
        p.setBrush(eraBg);
        p.drawRoundedRect(QRect(rect.x(), static_cast<int>(cy - step / 2 + 1), rect.width(), static_cast<int>(step - 2)), 4, 4);

        // Popularity bubble -- trending topics get larger bubbles
        int bubbleR = e.trending ? 14 : 8;
        int bubbleX = rect.x() + 20 + static_cast<int>((e.popularity / 100.0) * (rect.width() - 50));

        QColor bubbleColor = e.color;
        bubbleColor.setAlpha(180);
        p.setBrush(bubbleColor);
        p.setPen(e.color);
        p.drawEllipse(QPoint(bubbleX, static_cast<int>(cy)), bubbleR, bubbleR);

        // Topic label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 2, static_cast<int>(cy - 8), rect.width() - 4, 14, Qt::AlignLeft | Qt::AlignVCenter,
                   e.topic.left(14));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 2, static_cast<int>(cy + 3), rect.width() - 4, 12, Qt::AlignLeft | Qt::AlignVCenter,
                   e.era + " | pop:" + QString::number(static_cast<int>(e.popularity)));
    }
}

void PaperTopicEvolution2::drawCategoryChart(QPainter& p, const QRect& rect) {
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

void PaperTopicEvolution2::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 20, "Statistics");

    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Topics", QString::number(entries_.size()), QColor("#3b82f6")},
        {"Trending",     QString::number(trendingCount()), QColor("#16a34a")},
        {"Avg Popularity", QString::number(avgPopularity(), 'f', 1), QColor("#d97706")}
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

void PaperTopicEvolution2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Track topic evolution");
        return;
    }
    infoLabel_->setText(QString("Topics: %1 | Trending: %2 | Avg Pop: %3")
        .arg(entries_.size())
        .arg(trendingCount())
        .arg(avgPopularity(), 0, 'f', 1));
}

void PaperTopicEvolution2::loadSettings() {
    settings_.beginGroup("TopicEvolution2");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        TopicEvoEntry e;
        e.id = settings_.value("id").toInt();
        e.topic = settings_.value("topic").toString();
        e.category = settings_.value("category").toString();
        e.era = settings_.value("era").toString();
        e.popularity = settings_.value("popularity").toDouble();
        e.papers = settings_.value("papers").toInt();
        e.trending = settings_.value("trending").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperTopicEvolution2::saveSettings() {
    settings_.beginGroup("TopicEvolution2");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("topic", entries_[i].topic);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("era", entries_[i].era);
        settings_.setValue("popularity", entries_[i].popularity);
        settings_.setValue("papers", entries_[i].papers);
        settings_.setValue("trending", entries_[i].trending);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
