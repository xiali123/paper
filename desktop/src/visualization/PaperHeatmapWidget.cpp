#include "visualization/PaperHeatmapWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperHeatmapWidget::PaperHeatmapWidget(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "HeatmapWidget")
{
    setupUI();
    loadSettings();
}

void PaperHeatmapWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperHeatmapWidget::onGenerate);
    toolbar->addWidget(generateBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Activity", "Impact", "Trend"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperHeatmapWidget::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter heatmap dataset...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Generate heatmap visualization");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(600, 500);
}

void PaperHeatmapWidget::addEntry(const HeatmapCell& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit heatmapGenerated(entry.id, entry.intensity);
    update();
}

QList<HeatmapCell> PaperHeatmapWidget::entries() const { return entries_; }

qreal PaperHeatmapWidget::avgIntensity() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.intensity;
    return sum / entries_.size();
}

int PaperHeatmapWidget::hotSpotCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.hotSpot) c++;
    return c;
}

QMap<QString, int> PaperHeatmapWidget::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperHeatmapWidget::onGenerate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"activity", "impact", "trend"};
    QStringList rows = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    QStringList cols = {"W1", "W2", "W3", "W4"};
    int cIdx = categoryCombo_->currentIndex();
    entries_.clear();
    int count = 12 + QRandomGenerator::global()->bounded(16);
    for (int i = 0; i < count; ++i) {
        HeatmapCell e;
        e.id = entries_.size() + 1;
        e.label = text.left(6) + " c" + QString::number(i);
        e.row = rows[i % rows.size()];
        e.col = cols[QRandomGenerator::global()->bounded(cols.size())];
        e.value = QRandomGenerator::global()->bounded(100);
        e.intensity = e.value / 100.0;
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.hotSpot = e.intensity >= 0.8;
        int r = static_cast<int>(e.intensity * 200);
        int g = static_cast<int>((1 - e.intensity) * 200);
        e.color = QColor(qMin(255, r + 55), qMin(255, g + 55), 55);
        entries_.append(e);
    }
    saveSettings();
    updateInfo();
    emit heatmapGenerated(entries_.size(), avgIntensity());
    update();
    inputField_->clear();
}

void PaperHeatmapWidget::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Generate heatmap visualization");
    update();
}

void PaperHeatmapWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Generate heatmap visualization");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Heatmap");
    int w = width(), h = height();
    drawHeatmapGrid(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperHeatmapWidget::drawHeatmapGrid(QPainter& p, const QRect& rect) {
    int n = entries_.size();
    int cols = qMax(1, static_cast<int>(qSqrt(n)));
    int rows = (n + cols - 1) / cols;
    int cellW = (rect.width() - 10) / cols;
    int cellH = (rect.height() - 10) / rows;
    for (int i = 0; i < n; ++i) {
        const auto& e = entries_[i];
        int col = i % cols;
        int row = i / cols;
        int x = rect.x() + 5 + col * cellW;
        int y = rect.y() + 5 + row * cellH;
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(x + 1, y + 1, cellW - 2, cellH - 2, 3, 3);
        if (e.hotSpot) {
            p.setPen(QColor(239,68,68));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(x + 1, y + 1, cellW - 2, cellH - 2, 3, 3);
        }
        p.setPen(e.intensity > 0.5 ? Qt::white : QColor(15, 23, 42));
        p.setFont(QFont("Arial", qMax(6, qMin(9, cellW / 8))));
        p.drawText(x + 2, y + 2, cellW - 4, cellH - 4, Qt::AlignCenter,
                   QString::number(e.value));
    }
}

void PaperHeatmapWidget::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"activity", "impact", "trend"};
    QString labels[] = {"Activity", "Impact", "Trend"};
    QColor colors[] = {QColor(239,130,80), QColor(130,80,239), QColor(80,200,130)};
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
                   QString::number(count) + " cells");
    }
}

void PaperHeatmapWidget::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Cells", QString::number(entries_.size()), QColor(59,130,246)},
        {"Hot Spots", QString::number(hotSpotCount()), QColor(239,68,68)},
        {"Avg Intensity", QString::number(avgIntensity() * 100, 'f', 0) + "%", QColor(245,158,11)},
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

void PaperHeatmapWidget::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Generate heatmap visualization"); return; }
    infoLabel_->setText(QString("%1 cells | %2 hot | %3% intensity")
        .arg(entries_.size()).arg(hotSpotCount()).arg(avgIntensity() * 100, 0, 'f', 0));
}

void PaperHeatmapWidget::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        HeatmapCell e;
        e.id = settings_.value("id").toInt();
        e.label = settings_.value("label").toString();
        e.row = settings_.value("row").toString();
        e.col = settings_.value("col").toString();
        e.value = settings_.value("value").toDouble();
        e.intensity = settings_.value("intensity").toDouble();
        e.category = settings_.value("category").toString();
        e.hotSpot = settings_.value("hotSpot").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperHeatmapWidget::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("label", entries_[i].label);
        settings_.setValue("row", entries_[i].row);
        settings_.setValue("col", entries_[i].col);
        settings_.setValue("value", entries_[i].value);
        settings_.setValue("intensity", entries_[i].intensity);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("hotSpot", entries_[i].hotSpot);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
