#include "reading/PaperReadingHeatmapGrid.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReadingHeatmapGrid::PaperReadingHeatmapGrid(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingHeatmapGrid")
{
    setupUI();
    loadSettings();
}

void PaperReadingHeatmapGrid::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    updateBtn_ = new QPushButton("Update");
    updateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(updateBtn_, &QPushButton::clicked, this, &PaperReadingHeatmapGrid::onUpdate);
    toolbar->addWidget(updateBtn_);
    toolbar->addWidget(new QLabel("Period:"));
    periodCombo_ = new QComboBox();
    periodCombo_->addItems({"This Week", "Last Week", "This Month"});
    toolbar->addWidget(periodCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingHeatmapGrid::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter reader name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Reading heatmap grid");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperReadingHeatmapGrid::addEntry(const HeatGridEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit heatmapUpdated(entry.id, entry.intensity);
    update();
}

QList<HeatGridEntry> PaperReadingHeatmapGrid::entries() const { return entries_; }

qreal PaperReadingHeatmapGrid::avgIntensity() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.intensity;
    return sum / entries_.size();
}

int PaperReadingHeatmapGrid::peakCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.peak) c++;
    return c;
}

QMap<QString, int> PaperReadingHeatmapGrid::dayCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.day]++;
    return counts;
}

void PaperReadingHeatmapGrid::onUpdate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList days = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    QStringList periods = {"this week", "last week", "this month"};
    QStringList categories = {"ML", "NLP", "CV", "Theory"};
    entries_.clear();
    int pIdx = periodCombo_->currentIndex();
    for (int d = 0; d < 7; ++d) {
        int hourSlots = 12 + QRandomGenerator::global()->bounded(6);
        for (int h = 0; h < hourSlots; ++h) {
            HeatGridEntry e;
            e.id = entries_.size() + 1;
            e.day = days[d];
            e.hour = QString::number(6 + h);
            e.papersRead = QRandomGenerator::global()->bounded(10);
            e.intensity = static_cast<qreal>(e.papersRead) / 10.0;
            e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
            e.period = periods[pIdx];
            e.totalMinutes = e.papersRead * (10 + QRandomGenerator::global()->bounded(30));
            e.peak = e.papersRead >= 7;
            e.color = e.peak ? QColor(239,68,68) : QColor(16,185, static_cast<int>(185 - e.intensity * 120), static_cast<int>(129 - e.intensity * 80));
            entries_.append(e);
        }
    }
    saveSettings();
    updateInfo();
    emit heatmapUpdated(entries_.size(), avgIntensity());
    update();
    inputField_->clear();
}

void PaperReadingHeatmapGrid::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Reading heatmap grid");
    update();
}

void PaperReadingHeatmapGrid::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Reading heatmap grid");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Heatmap Grid");
    int w = width(), h = height();
    drawHeatGrid(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawDayLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReadingHeatmapGrid::drawHeatGrid(QPainter& p, const QRect& rect) {
    auto counts = dayCounts();
    QStringList days = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    int cellW = (rect.width() - 10) / 18;
    int cellH = (rect.height() - 20) / 7;
    for (int d = 0; d < 7; ++d) {
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), rect.y() + d * cellH + 2, 30, cellH, Qt::AlignVCenter, days[d]);
        int col = 0;
        for (const auto& e : entries_) {
            if (e.day == days[d] && col < 17) {
                int alpha = 60 + static_cast<int>(e.intensity * 195);
                QColor fill = QColor(16, 185, 129, qMin(255, alpha));
                p.setPen(Qt::NoPen);
                p.setBrush(fill);
                p.drawRoundedRect(rect.x() + 34 + col * (cellW + 1), rect.y() + d * cellH + 1, cellW, cellH - 2, 2, 2);
                col++;
            }
        }
    }
}

void PaperReadingHeatmapGrid::drawDayLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Days");
    auto counts = dayCounts();
    QStringList days = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(18, (rect.height() - 30) / 7);
    for (int i = 0; i < 7; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(days[i]) ? counts[days[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 80));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 30, barH, Qt::AlignRight | Qt::AlignVCenter, days[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(16, 185, 129, 150));
        p.drawRoundedRect(rect.x() + 35, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 38 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperReadingHeatmapGrid::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Cells", QString::number(entries_.size()), QColor(59,130,246)},
        {"Peaks", QString::number(peakCount()), QColor(239,68,68)},
        {"Avg Intensity", QString::number(avgIntensity(), 'f', 2), QColor(245,158,11)},
        {"Days", QString::number(dayCounts().size()), QColor(139,92,246)}
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

void PaperReadingHeatmapGrid::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Reading heatmap grid"); return; }
    infoLabel_->setText(QString("%1 cells | %2 peaks | %3 avg")
        .arg(entries_.size()).arg(peakCount()).arg(avgIntensity(), 0, 'f', 2));
}

void PaperReadingHeatmapGrid::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        HeatGridEntry e;
        e.id = settings_.value("id").toInt();
        e.day = settings_.value("day").toString();
        e.hour = settings_.value("hour").toString();
        e.papersRead = settings_.value("papersRead").toInt();
        e.intensity = settings_.value("intensity").toDouble();
        e.category = settings_.value("category").toString();
        e.period = settings_.value("period").toString();
        e.totalMinutes = settings_.value("totalMinutes").toInt();
        e.peak = settings_.value("peak").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingHeatmapGrid::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("day", entries_[i].day);
        settings_.setValue("hour", entries_[i].hour);
        settings_.setValue("papersRead", entries_[i].papersRead);
        settings_.setValue("intensity", entries_[i].intensity);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("period", entries_[i].period);
        settings_.setValue("totalMinutes", entries_[i].totalMinutes);
        settings_.setValue("peak", entries_[i].peak);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
