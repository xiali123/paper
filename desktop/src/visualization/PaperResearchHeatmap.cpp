#include "visualization/PaperResearchHeatmap.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperResearchHeatmap::PaperResearchHeatmap(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ResearchHeatmap")
{
    setupUI();
    loadSettings();
}

void PaperResearchHeatmap::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperResearchHeatmap::onGenerate);
    toolbar->addWidget(generateBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "ML/AI", "NLP", "Vision", "Security"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperResearchHeatmap::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter research topic for heatmap...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Research heatmap");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperResearchHeatmap::addEntry(const HeatmapEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit heatmapGenerated(entry.id, entry.intensity);
    update();
}

QList<HeatmapEntry> PaperResearchHeatmap::entries() const { return entries_; }

qreal PaperResearchHeatmap::avgActivity() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.activityScore;
    return sum / entries_.size();
}

int PaperResearchHeatmap::peakIntensity() const {
    int peak = 0;
    for (const auto& e : entries_) peak = qMax(peak, e.intensity);
    return peak;
}

QMap<QString, int> PaperResearchHeatmap::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperResearchHeatmap::onGenerate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"ML/AI", "NLP", "Vision", "Security", "Theory"};
    QStringList timeSlots = {"Mon AM", "Mon PM", "Tue AM", "Tue PM", "Wed AM", "Wed PM",
                             "Thu AM", "Thu PM", "Fri AM", "Fri PM"};
    QStringList trends = {"rising", "stable", "declining"};
    QColor catColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68), QColor(139,92,246)};

    int catIdx = categoryCombo_->currentIndex();
    if (catIdx == 0) catIdx = QRandomGenerator::global()->bounded(categories.size());

    int count = 6 + QRandomGenerator::global()->bounded(5);
    for (int i = 0; i < count; ++i) {
        HeatmapEntry e;
        e.id = entries_.size() + 1;
        e.topic = text.left(10) + " [" + QString::number(i) + "]";
        e.intensity = QRandomGenerator::global()->bounded(100);
        e.timeSlot = timeSlots[i % timeSlots.size()];
        e.activityScore = e.intensity / 100.0;
        e.category = categories[qMin(catIdx - 1, categories.size() - 1)];
        e.paperCount = 1 + QRandomGenerator::global()->bounded(15);
        e.trend = trends[QRandomGenerator::global()->bounded(trends.size())];
        e.color = catColors[qMin(catIdx - 1, 4)];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperResearchHeatmap::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Research heatmap");
    update();
}

void PaperResearchHeatmap::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Research heatmap");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Research Heatmap");

    int w = width(), h = height();
    drawHeatmapGrid(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperResearchHeatmap::drawHeatmapGrid(QPainter& p, const QRect& rect) {
    int cols = 5;
    int rows = qMin(10, entries_.size());
    if (rows == 0) return;

    int cellW = (rect.width() - 10) / cols;
    int cellH = qMin(30, (rect.height() - 10) / rows);

    for (int i = 0; i < rows; ++i) {
        const auto& e = entries_[i];
        int row = i / cols;
        int col = i % cols;
        int x = rect.x() + col * cellW;
        int y = rect.y() + row * cellH;

        int alpha = 60 + static_cast<int>(e.activityScore * 195);
        QColor cellColor = QColor(e.color.red(), e.color.green(), e.color.blue(), alpha);
        p.setPen(Qt::NoPen);
        p.setBrush(cellColor);
        p.drawRoundedRect(x + 1, y + 1, cellW - 2, cellH - 2, 3, 3);

        p.setPen(alpha > 150 ? Qt::white : QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(x + 2, y + 2, cellW - 4, cellH / 2 - 2, Qt::AlignCenter, QString::number(e.intensity));
        p.setFont(QFont("Arial", 5));
        p.drawText(x + 2, y + cellH / 2, cellW - 4, cellH / 2 - 2, Qt::AlignCenter, e.trend.left(4));
    }
}

void PaperResearchHeatmap::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"ML/AI", "NLP", "Vision", "Security", "Theory"};
    QString labels[] = {"ML/AI", "NLP", "Vision", "Security", "Theory"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68), QColor(139,92,246)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 2);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 100));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 55, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 60, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 63 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperResearchHeatmap::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Cells", QString::number(entries_.size()), QColor(59,130,246)},
        {"Peak", QString::number(peakIntensity()), QColor(239,68,68)},
        {"Avg Activity", QString::number(avgActivity() * 100, 'f', 0) + "%", QColor(16,185,129)},
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

void PaperResearchHeatmap::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Research heatmap"); return; }
    infoLabel_->setText(QString("%1 cells | %2 peak | %3% avg")
        .arg(entries_.size()).arg(peakIntensity()).arg(avgActivity() * 100, 0, 'f', 0));
}

void PaperResearchHeatmap::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        HeatmapEntry e;
        e.id = settings_.value("id").toInt();
        e.topic = settings_.value("topic").toString();
        e.intensity = settings_.value("intensity").toInt();
        e.timeSlot = settings_.value("timeSlot").toString();
        e.activityScore = settings_.value("activityScore").toDouble();
        e.category = settings_.value("category").toString();
        e.paperCount = settings_.value("paperCount").toInt();
        e.trend = settings_.value("trend").toString();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperResearchHeatmap::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("topic", entries_[i].topic);
        settings_.setValue("intensity", entries_[i].intensity);
        settings_.setValue("timeSlot", entries_[i].timeSlot);
        settings_.setValue("activityScore", entries_[i].activityScore);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("paperCount", entries_[i].paperCount);
        settings_.setValue("trend", entries_[i].trend);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
