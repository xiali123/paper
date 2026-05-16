#include "reading/PaperReadingZoneTracker.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QDateTime>
#include <QFontMetrics>
#include <algorithm>
#include <cmath>

PaperReadingZoneTracker::PaperReadingZoneTracker(QWidget* parent)
    : QWidget(parent)
    , settings_(QSettings::IniFormat, QSettings::UserScope, "PaperCrawler", "PaperReadingZoneTracker")
{
    setupUI();
    loadSettings();
}

void PaperReadingZoneTracker::setupUI()
{
    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(16);

    // Left panel
    auto* leftPanel = new QVBoxLayout();
    leftPanel->setSpacing(8);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItem("General");
    categoryCombo_->addItem("Machine Learning");
    categoryCombo_->addItem("NLP");
    categoryCombo_->addItem("Computer Vision");
    categoryCombo_->addItem("Systems");
    categoryCombo_->addItem("Theory");
    categoryCombo_->addItem("Security");
    categoryCombo_->addItem("HCI");
    leftPanel->addWidget(categoryCombo_);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Paper title...");
    leftPanel->addWidget(inputField_);

    trackBtn_ = new QPushButton("Track", this);
    leftPanel->addWidget(trackBtn_);

    clearBtn_ = new QPushButton("Clear", this);
    leftPanel->addWidget(clearBtn_);

    infoLabel_ = new QLabel("Zones: 0 | Deep: 0 | Avg Focus: 0.00", this);
    infoLabel_->setWordWrap(true);
    leftPanel->addWidget(infoLabel_);

    leftPanel->addStretch();

    mainLayout->addLayout(leftPanel, 0);
    mainLayout->addStretch(1);

    connect(trackBtn_, &QPushButton::clicked, this, &PaperReadingZoneTracker::onTrack);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingZoneTracker::onClear);
}

void PaperReadingZoneTracker::onTrack()
{
    QString paper = inputField_->text().trimmed();
    if (paper.isEmpty()) {
        return;
    }

    static const QColor colors[] = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };

    qreal focus = QRandomGenerator::global()->generateDouble();
    int duration = QRandomGenerator::global()->bounded(5, 121); // 5-120 minutes
    bool deep = focus > 0.7;

    ReadingZoneEntry entry;
    entry.id = static_cast<int>(QDateTime::currentMSecsSinceEpoch() % 100000);
    entry.zone = deep ? "deep" : "shallow";
    entry.category = categoryCombo_->currentText();
    entry.paper = paper;
    entry.focus = focus;
    entry.duration = duration;
    entry.deep = deep;
    entry.color = colors[entries_.size() % 5];

    addEntry(entry);

    inputField_->clear();
}

void PaperReadingZoneTracker::onClear()
{
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperReadingZoneTracker::addEntry(const ReadingZoneEntry& entry)
{
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
    emit zoneEntered(entry.id, entry.focus);
}

QList<ReadingZoneEntry> PaperReadingZoneTracker::entries() const
{
    return entries_;
}

int PaperReadingZoneTracker::deepCount() const
{
    int count = 0;
    for (const auto& e : entries_) {
        if (e.deep) {
            ++count;
        }
    }
    return count;
}

qreal PaperReadingZoneTracker::avgFocus() const
{
    if (entries_.isEmpty()) {
        return 0.0;
    }
    qreal sum = 0.0;
    for (const auto& e : entries_) {
        sum += e.focus;
    }
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingZoneTracker::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& e : entries_) {
        counts[e.category]++;
    }
    return counts;
}

void PaperReadingZoneTracker::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    int margin = 16;
    int colW = (w - 4 * margin) / 3;

    QRect zoneRect(margin, margin, colW, h - 2 * margin);
    QRect chartRect(2 * margin + colW, margin, colW, h - 2 * margin);
    QRect statsRect(3 * margin + 2 * colW, margin, colW, h - 2 * margin);

    drawZoneMap(p, zoneRect);
    drawCategoryChart(p, chartRect);
    drawStats(p, statsRect);
}

void PaperReadingZoneTracker::drawZoneMap(QPainter& p, const QRect& rect)
{
    // Background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(248, 250, 252));
    p.drawRoundedRect(rect, 8, 8);

    // Border
    p.setPen(QPen(QColor(226, 232, 240), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(rect, 8, 8);

    // Title
    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.setPen(QColor(30, 41, 59));
    p.drawText(QRect(rect.x() + 8, rect.y() + 6, rect.width() - 16, 24),
               Qt::AlignLeft | Qt::AlignVCenter, "Reading Zones");

    if (entries_.isEmpty()) {
        QFont hintFont = p.font();
        hintFont.setBold(false);
        hintFont.setPointSize(9);
        p.setFont(hintFont);
        p.setPen(QColor(148, 163, 184));
        p.drawText(rect.adjusted(8, 36, -8, -8), Qt::AlignCenter, "No zones tracked yet");
        return;
    }

    // Grid layout for zones
    int cols = 4;
    int rows = std::max(1, static_cast<int>(std::ceil(static_cast<double>(entries_.size()) / cols)));
    int gridTop = rect.y() + 36;
    int gridW = rect.width() - 16;
    int gridH = rect.height() - 48;
    int cellW = gridW / cols;
    int cellH = std::min(gridH / rows, 60);

    QFont cellFont = p.font();
    cellFont.setBold(false);
    cellFont.setPointSize(8);
    p.setFont(cellFont);

    for (int i = 0; i < entries_.size(); ++i) {
        const auto& entry = entries_[i];
        int row = i / cols;
        int col = i % cols;

        int x = rect.x() + 8 + col * cellW;
        int y = gridTop + row * cellH;

        QRect cellRect(x + 2, y + 2, cellW - 4, cellH - 4);

        // Color intensity based on focus level
        QColor fillColor = entry.color;
        if (entry.deep) {
            fillColor.setAlphaF(0.6 + entry.focus * 0.4); // bright for deep zones
        } else {
            fillColor.setAlphaF(0.15 + entry.focus * 0.25); // dim for shallow zones
        }

        p.setPen(Qt::NoPen);
        p.setBrush(fillColor);
        p.drawRoundedRect(cellRect, 4, 4);

        // Zone label
        p.setPen(entry.deep ? QColor(255, 255, 255) : QColor(30, 41, 59));
        QString label = QString("%1\n%2").arg(entry.zone).arg(QString::number(entry.focus, 'f', 2));
        p.drawText(cellRect, Qt::AlignCenter, label);
    }
}

void PaperReadingZoneTracker::drawCategoryChart(QPainter& p, const QRect& rect)
{
    // Background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(248, 250, 252));
    p.drawRoundedRect(rect, 8, 8);

    // Border
    p.setPen(QPen(QColor(226, 232, 240), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(rect, 8, 8);

    // Title
    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.setPen(QColor(30, 41, 59));
    p.drawText(QRect(rect.x() + 8, rect.y() + 6, rect.width() - 16, 24),
               Qt::AlignLeft | Qt::AlignVCenter, "Categories");

    QMap<QString, int> counts = categoryCounts();
    if (counts.isEmpty()) {
        QFont hintFont = p.font();
        hintFont.setBold(false);
        hintFont.setPointSize(9);
        p.setFont(hintFont);
        p.setPen(QColor(148, 163, 184));
        p.drawText(rect.adjusted(8, 36, -8, -8), Qt::AlignCenter, "No categories yet");
        return;
    }

    int maxCount = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        maxCount = std::max(maxCount, it.value());
    }

    static const QColor barColors[] = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };

    QFont barFont = p.font();
    barFont.setBold(false);
    barFont.setPointSize(9);
    p.setFont(barFont);

    int barTop = rect.y() + 40;
    int barHeight = 24;
    int barSpacing = 8;
    int labelWidth = 100;
    int maxBarWidth = rect.width() - labelWidth - 40;

    int idx = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        int y = barTop + idx * (barHeight + barSpacing);
        if (y + barHeight > rect.bottom() - 8) break;

        // Category label
        p.setPen(QColor(30, 41, 59));
        p.drawText(QRect(rect.x() + 8, y, labelWidth, barHeight),
                   Qt::AlignRight | Qt::AlignVCenter, it.key());

        // Bar
        int barW = maxCount > 0 ? static_cast<int>(static_cast<double>(it.value()) / maxCount * maxBarWidth) : 0;
        QColor barColor = barColors[idx % 5];
        barColor.setAlphaF(0.75);
        p.setPen(Qt::NoPen);
        p.setBrush(barColor);
        p.drawRoundedRect(rect.x() + labelWidth + 14, y + 2, barW, barHeight - 4, 3, 3);

        // Count label
        p.setPen(QColor(71, 85, 105));
        p.drawText(QRect(rect.x() + labelWidth + 18 + barW, y, 30, barHeight),
                   Qt::AlignLeft | Qt::AlignVCenter, QString::number(it.value()));

        ++idx;
    }
}

void PaperReadingZoneTracker::drawStats(QPainter& p, const QRect& rect)
{
    // Background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(248, 250, 252));
    p.drawRoundedRect(rect, 8, 8);

    // Border
    p.setPen(QPen(QColor(226, 232, 240), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(rect, 8, 8);

    // Title
    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.setPen(QColor(30, 41, 59));
    p.drawText(QRect(rect.x() + 8, rect.y() + 6, rect.width() - 16, 24),
               Qt::AlignLeft | Qt::AlignVCenter, "Statistics");

    QFont statFont = p.font();
    statFont.setBold(false);
    statFont.setPointSize(10);
    p.setFont(statFont);

    int y = rect.y() + 48;
    int lineH = 36;

    // Total zones
    p.setPen(QColor(100, 116, 139));
    p.drawText(QRect(rect.x() + 12, y, rect.width() - 24, lineH / 2),
               Qt::AlignLeft | Qt::AlignVCenter, "Total Zones");
    p.setPen(QColor(30, 41, 59));
    QFont numFont = statFont;
    numFont.setBold(true);
    p.setFont(numFont);
    p.drawText(QRect(rect.x() + 12, y + lineH / 2, rect.width() - 24, lineH / 2),
               Qt::AlignLeft | Qt::AlignVCenter, QString::number(entries_.size()));
    p.setFont(statFont);
    y += lineH;

    // Deep count
    p.setPen(QColor(100, 116, 139));
    p.drawText(QRect(rect.x() + 12, y, rect.width() - 24, lineH / 2),
               Qt::AlignLeft | Qt::AlignVCenter, "Deep Zones");
    p.setPen(QColor("#16a34a"));
    p.setFont(numFont);
    p.drawText(QRect(rect.x() + 12, y + lineH / 2, rect.width() - 24, lineH / 2),
               Qt::AlignLeft | Qt::AlignVCenter, QString::number(deepCount()));
    p.setFont(statFont);
    y += lineH;

    // Average focus
    p.setPen(QColor(100, 116, 139));
    p.drawText(QRect(rect.x() + 12, y, rect.width() - 24, lineH / 2),
               Qt::AlignLeft | Qt::AlignVCenter, "Avg Focus");
    p.setPen(QColor("#3b82f6"));
    p.setFont(numFont);
    p.drawText(QRect(rect.x() + 12, y + lineH / 2, rect.width() - 24, lineH / 2),
               Qt::AlignLeft | Qt::AlignVCenter, QString::number(avgFocus(), 'f', 2));
    p.setFont(statFont);
}

void PaperReadingZoneTracker::updateInfo()
{
    infoLabel_->setText(QString("Zones: %1 | Deep: %2 | Avg Focus: %3")
                            .arg(entries_.size())
                            .arg(deepCount())
                            .arg(QString::number(avgFocus(), 'f', 2)));
}

void PaperReadingZoneTracker::loadSettings()
{
    settings_.beginGroup("ReadingZoneTracker");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ReadingZoneEntry entry;
        entry.id = settings_.value("id").toInt();
        entry.zone = settings_.value("zone").toString();
        entry.category = settings_.value("category").toString();
        entry.paper = settings_.value("paper").toString();
        entry.focus = settings_.value("focus").toReal();
        entry.duration = settings_.value("duration").toInt();
        entry.deep = settings_.value("deep").toBool();
        entry.color = QColor(settings_.value("color").toString());
        entries_.append(entry);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
    update();
}

void PaperReadingZoneTracker::saveSettings()
{
    settings_.beginGroup("ReadingZoneTracker");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& entry = entries_[i];
        settings_.setValue("id", entry.id);
        settings_.setValue("zone", entry.zone);
        settings_.setValue("category", entry.category);
        settings_.setValue("paper", entry.paper);
        settings_.setValue("focus", entry.focus);
        settings_.setValue("duration", entry.duration);
        settings_.setValue("deep", entry.deep);
        settings_.setValue("color", entry.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
    settings_.sync();
}
