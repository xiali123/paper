#include "analysis/PaperTrendDetector.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperTrendDetector::PaperTrendDetector(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "TrendDetector")
{
    setupUI();
    loadSettings();
}

void PaperTrendDetector::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    detectBtn_ = new QPushButton("Detect");
    detectBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(detectBtn_, &QPushButton::clicked, this, &PaperTrendDetector::onDetect);
    toolbar->addWidget(detectBtn_);

    toolbar->addWidget(new QLabel("Category:"));

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "AI/ML", "NLP", "Computer Vision", "Robotics", "Bioinformatics", "Materials", "Climate"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperTrendDetector::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter research topic to detect trends...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Detect research trends");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperTrendDetector::addEntry(const TrendEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<TrendEntry> PaperTrendDetector::entries() const {
    return entries_;
}

int PaperTrendDetector::emergingCount() const {
    int count = 0;
    for (const auto& e : entries_) {
        if (e.emerging) ++count;
    }
    return count;
}

qreal PaperTrendDetector::avgStrength() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) {
        sum += e.strength;
    }
    return sum / entries_.size();
}

QMap<QString, int> PaperTrendDetector::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) {
        counts[e.category]++;
    }
    return counts;
}

void PaperTrendDetector::onDetect() {
    QString topic = inputField_->text().trimmed();
    if (topic.isEmpty()) return;

    static const QStringList categories = {
        "AI/ML", "NLP", "Computer Vision", "Robotics", "Bioinformatics", "Materials", "Climate"
    };
    static const QStringList directions = {"rising", "stable", "declining", "surging"};
    static const QList<QColor> palette = {
        QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)
    };

    int count = 3 + QRandomGenerator::global()->bounded(5);
    for (int i = 0; i < count; ++i) {
        TrendEntry entry;
        entry.id = entries_.size() + 1;
        entry.topic = (i == 0) ? topic : topic + " " + QString::number(i + 1);
        entry.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        entry.direction = directions[QRandomGenerator::global()->bounded(directions.size())];
        entry.strength = QRandomGenerator::global()->bounded(100) / 100.0;
        entry.velocity = QRandomGenerator::global()->bounded(100) / 100.0;
        entry.confidence = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
        entry.emerging = entry.strength > 0.7 && entry.velocity > 0.5;
        entry.color = palette[entry.id % palette.size()];
        addEntry(entry);
        emit trendDetected(entry.id, entry.strength);
    }

    inputField_->clear();
}

void PaperTrendDetector::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Detect research trends");
    update();
}

void PaperTrendDetector::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Detect research trends");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Trend Detector");

    int w = width(), h = height();
    drawTrendList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperTrendDetector::drawTrendList(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(34, (rect.height() - 10) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 3);

        // Background row
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        // Left accent bar
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        // Topic and direction label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        QString dirTag = e.emerging ? " [EMERGING]" : QString(" [%1]").arg(e.direction.left(4));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.topic.left(14) + dirTag);

        // Strength and velocity detail
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | Str " + QString::number(e.strength, 'f', 2) +
                   " | Vel " + QString::number(e.velocity, 'f', 2));

        // Strength bar
        int barW = static_cast<int>(e.strength * (rect.width() / 2 - 20));
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x() + rect.width() / 2, y + 10, barW, 14, 3, 3);

        // Confidence text
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(rect.x() + rect.width() / 2 + barW + 4, y + 22,
                   "Conf " + QString::number(e.confidence, 'f', 2));
    }
}

void PaperTrendDetector::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    static const QList<QColor> colors = {
        QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)
    };

    int maxVal = 1;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        maxVal = qMax(maxVal, it.value());
    }

    int ci = 0;
    int barH = qMin(24, (rect.height() - 30) / qMax(counts.size(), 1));
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        int y = rect.y() + 22 + ci * (barH + 3);
        int barW = static_cast<int>((static_cast<qreal>(it.value()) / maxVal) * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 80, barH, Qt::AlignRight | Qt::AlignVCenter, it.key());

        p.setPen(Qt::NoPen);
        p.setBrush(colors[ci % colors.size()]);
        p.drawRoundedRect(rect.x() + 85, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 88 + barW, y + barH - 2, QString::number(it.value()));
        ++ci;
    }
}

void PaperTrendDetector::drawStats(QPainter& p, const QRect& rect) {
    struct Stat {
        QString label;
        QString value;
        QColor color;
    };

    QList<Stat> stats = {
        {"Total Trends", QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Emerging", QString::number(emergingCount()), QColor(22, 163, 74)},
        {"Avg Strength", QString::number(avgStrength(), 'f', 2), QColor(217, 119, 6)},
        {"Categories", QString::number(categoryCounts().size()), QColor(124, 58, 237)}
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

void PaperTrendDetector::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Detect research trends");
        return;
    }
    infoLabel_->setText(QString("%1 trends | %2 emerging | avg strength %3")
        .arg(entries_.size())
        .arg(emergingCount())
        .arg(avgStrength(), 0, 'f', 2));
}

void PaperTrendDetector::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        TrendEntry e;
        e.id = settings_.value("id").toInt();
        e.topic = settings_.value("topic").toString();
        e.category = settings_.value("category").toString();
        e.direction = settings_.value("direction").toString();
        e.strength = settings_.value("strength").toDouble();
        e.velocity = settings_.value("velocity").toDouble();
        e.confidence = settings_.value("confidence").toDouble();
        e.emerging = settings_.value("emerging").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperTrendDetector::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("topic", entries_[i].topic);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("direction", entries_[i].direction);
        settings_.setValue("strength", entries_[i].strength);
        settings_.setValue("velocity", entries_[i].velocity);
        settings_.setValue("confidence", entries_[i].confidence);
        settings_.setValue("emerging", entries_[i].emerging);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
