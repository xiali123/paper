#include "visualization/PaperContourPlot.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperContourPlot::PaperContourPlot(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ContourPlot")
{
    setupUI();
    loadSettings();
}

void PaperContourPlot::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperContourPlot::onGenerate);
    toolbar->addWidget(generateBtn_);
    toolbar->addWidget(new QLabel("Region:"));
    regionCombo_ = new QComboBox();
    regionCombo_->addItems({"All", "Core", "Periphery", "Frontier"});
    toolbar->addWidget(regionCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperContourPlot::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter dataset for contour plot...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Generate contour plot");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(600, 500);
}

void PaperContourPlot::addEntry(const ContourEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit contourGenerated(entry.id, entry.intensity);
    update();
}

QList<ContourEntry> PaperContourPlot::entries() const { return entries_; }

qreal PaperContourPlot::avgIntensity() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.intensity;
    return sum / entries_.size();
}

int PaperContourPlot::peakCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.peak) c++;
    return c;
}

QMap<QString, int> PaperContourPlot::regionCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.region]++;
    return counts;
}

void PaperContourPlot::onGenerate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList regions = {"core", "periphery", "frontier"};
    QStringList categories = {"density", "impact", "growth", "diversity"};
    QColor regionColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11)};
    entries_.clear();
    int rIdx = regionCombo_->currentIndex();
    int count = 20 + QRandomGenerator::global()->bounded(30);
    for (int i = 0; i < count; ++i) {
        ContourEntry e;
        e.id = entries_.size() + 1;
        e.label = "C" + QString::number(i);
        e.xValue = QRandomGenerator::global()->bounded(100);
        e.yValue = QRandomGenerator::global()->bounded(100);
        e.zValue = QRandomGenerator::global()->bounded(100) / 10.0;
        e.gridIndex = i;
        int regIdx = rIdx == 0 ? QRandomGenerator::global()->bounded(regions.size()) : rIdx - 1;
        e.region = regions[regIdx];
        e.intensity = e.zValue;
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.peak = e.intensity > 8.0;
        e.color = regionColors[regIdx];
        entries_.append(e);
    }
    saveSettings();
    updateInfo();
    emit contourGenerated(entries_.size(), avgIntensity());
    update();
    inputField_->clear();
}

void PaperContourPlot::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Generate contour plot");
    update();
}

void PaperContourPlot::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Generate contour plot");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Contour Plot");
    int w = width(), h = height();
    drawContourView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawRegionLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperContourPlot::drawContourView(QPainter& p, const QRect& rect) {
    int margin = 20;
    int plotW = rect.width() - margin * 2;
    int plotH = rect.height() - margin * 2;
    p.setPen(QPen(QColor(203, 213, 225), 1));
    for (int i = 0; i <= 5; ++i) {
        int x = rect.x() + margin + plotW * i / 5;
        p.drawLine(x, rect.y() + margin, x, rect.y() + margin + plotH);
        int y = rect.y() + margin + plotH * i / 5;
        p.drawLine(rect.x() + margin, y, rect.x() + margin + plotW, y);
    }
    for (const auto& e : entries_) {
        int px = rect.x() + margin + static_cast<int>(e.xValue / 100.0 * plotW);
        int py = rect.y() + margin + plotH - static_cast<int>(e.yValue / 100.0 * plotH);
        int sz = 2 + static_cast<int>(e.intensity);
        int alpha = 60 + static_cast<int>(e.intensity * 20);
        QColor fill = QColor(e.color.red(), e.color.green(), e.color.blue(), qMin(255, alpha));
        p.setPen(e.peak ? QPen(Qt::red, 1) : Qt::NoPen);
        p.setBrush(fill);
        p.drawEllipse(px - sz / 2, py - sz / 2, sz, sz);
    }
}

void PaperContourPlot::drawRegionLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Regions");
    auto counts = regionCounts();
    QStringList regions = {"core", "periphery", "frontier"};
    QString labels[] = {"Core", "Periphery", "Frontier"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11)};
    int itemH = qMin(28, (rect.height() - 30) / 3);
    for (int i = 0; i < 3; ++i) {
        int y = rect.y() + 22 + i * (itemH + 4);
        int count = counts.contains(regions[i]) ? counts[regions[i]] : 0;
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawEllipse(rect.x() + 5, y + 4, 14, 14);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18, Qt::AlignVCenter, labels[i]);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(count) + " points");
    }
}

void PaperContourPlot::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Points", QString::number(entries_.size()), QColor(59,130,246)},
        {"Peaks", QString::number(peakCount()), QColor(239,68,68)},
        {"Avg Intensity", QString::number(avgIntensity(), 'f', 1), QColor(245,158,11)},
        {"Regions", QString::number(regionCounts().size()), QColor(139,92,246)}
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

void PaperContourPlot::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Generate contour plot"); return; }
    infoLabel_->setText(QString("%1 points | %2 peaks | %3 regions")
        .arg(entries_.size()).arg(peakCount()).arg(regionCounts().size()));
}

void PaperContourPlot::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ContourEntry e;
        e.id = settings_.value("id").toInt();
        e.label = settings_.value("label").toString();
        e.xValue = settings_.value("xValue").toDouble();
        e.yValue = settings_.value("yValue").toDouble();
        e.zValue = settings_.value("zValue").toDouble();
        e.gridIndex = settings_.value("gridIndex").toInt();
        e.region = settings_.value("region").toString();
        e.intensity = settings_.value("intensity").toDouble();
        e.category = settings_.value("category").toString();
        e.peak = settings_.value("peak").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperContourPlot::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("label", entries_[i].label);
        settings_.setValue("xValue", entries_[i].xValue);
        settings_.setValue("yValue", entries_[i].yValue);
        settings_.setValue("zValue", entries_[i].zValue);
        settings_.setValue("gridIndex", entries_[i].gridIndex);
        settings_.setValue("region", entries_[i].region);
        settings_.setValue("intensity", entries_[i].intensity);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("peak", entries_[i].peak);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
