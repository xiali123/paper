#include "visualization/PaperGaugeChart.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperGaugeChart::PaperGaugeChart(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "GaugeChart")
{
    setupUI();
    loadSettings();
}

void PaperGaugeChart::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperGaugeChart::onGenerate);
    toolbar->addWidget(generateBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Performance", "Quality", "Coverage"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperGaugeChart::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter metric for gauge...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Generate gauge chart");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(600, 500);
}

void PaperGaugeChart::addEntry(const GaugeEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit gaugeGenerated(entry.id, entry.value);
    update();
}

QList<GaugeEntry> PaperGaugeChart::entries() const { return entries_; }

qreal PaperGaugeChart::avgValue() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.value;
    return sum / entries_.size();
}

int PaperGaugeChart::onTargetCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.onTarget) c++;
    return c;
}

QMap<QString, int> PaperGaugeChart::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperGaugeChart::onGenerate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList metrics = {"citations", "downloads", "coverage", "quality", "speed", "accuracy"};
    QStringList units = {"%", "count", "score", "ms", "ratio", "index"};
    QStringList categories = {"performance", "quality", "coverage"};
    QStringList statuses = {"excellent", "good", "warning", "critical"};
    QColor statusColors[] = {QColor(16,185,129), QColor(59,130,246), QColor(245,158,11), QColor(239,68,68)};
    int cIdx = categoryCombo_->currentIndex();
    entries_.clear();
    int count = 4 + QRandomGenerator::global()->bounded(3);
    for (int i = 0; i < count; ++i) {
        GaugeEntry e;
        e.id = entries_.size() + 1;
        e.metric = metrics[QRandomGenerator::global()->bounded(metrics.size())];
        e.maxVal = 100.0;
        e.target = 60 + QRandomGenerator::global()->bounded(30);
        e.value = 20 + QRandomGenerator::global()->bounded(80);
        e.unit = units[QRandomGenerator::global()->bounded(units.size())];
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        int sIdx = e.value >= 80 ? 0 : (e.value >= 60 ? 1 : (e.value >= 40 ? 2 : 3));
        e.status = statuses[sIdx];
        e.rank = i + 1;
        e.onTarget = e.value >= e.target;
        e.color = statusColors[sIdx];
        entries_.append(e);
    }
    saveSettings();
    updateInfo();
    emit gaugeGenerated(entries_.size(), avgValue());
    update();
    inputField_->clear();
}

void PaperGaugeChart::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Generate gauge chart");
    update();
}

void PaperGaugeChart::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Generate gauge chart");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Gauge Chart");
    int w = width(), h = height();
    drawGaugeView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperGaugeChart::drawGaugeView(QPainter& p, const QRect& rect) {
    int n = entries_.size();
    int cols = qMin(3, n);
    int rows = (n + cols - 1) / cols;
    int gaugeW = (rect.width() - 20) / cols;
    int gaugeH = (rect.height() - 20) / rows;
    for (int i = 0; i < n; ++i) {
        const auto& e = entries_[i];
        int col = i % cols;
        int row = i / cols;
        int cx = rect.x() + 10 + col * gaugeW + gaugeW / 2;
        int cy = rect.y() + 10 + row * gaugeH + gaugeH / 2;
        int r = qMin(gaugeW, gaugeH) / 2 - 15;
        qreal ratio = e.value / e.maxVal;
        int spanAngle = static_cast<int>(ratio * 270 * 16);
        int startAngle = 225 * 16;
        p.setPen(QPen(QColor(226, 232, 240), 6));
        p.drawArc(cx - r, cy - r, r * 2, r * 2, startAngle, -270 * 16);
        p.setPen(QPen(e.color, 6));
        p.drawArc(cx - r, cy - r, r * 2, r * 2, startAngle, -spanAngle);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 10, QFont::Bold));
        p.drawText(cx - 20, cy - 2, 40, 16, Qt::AlignCenter, QString::number(e.value, 'f', 0));
        p.setFont(QFont("Arial", 7));
        p.drawText(cx - gaugeW / 2 + 5, cy + r + 2, gaugeW - 10, 14, Qt::AlignCenter, e.metric);
    }
}

void PaperGaugeChart::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"performance", "quality", "coverage"};
    QString labels[] = {"Performance", "Quality", "Coverage"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11)};
    int itemH = qMin(28, (rect.height() - 30) / 3);
    for (int i = 0; i < 3; ++i) {
        int y = rect.y() + 22 + i * (itemH + 4);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
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
                   QString::number(count) + " metrics");
    }
}

void PaperGaugeChart::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Metrics", QString::number(entries_.size()), QColor(59,130,246)},
        {"On Target", QString::number(onTargetCount()), QColor(16,185,129)},
        {"Avg Value", QString::number(avgValue(), 'f', 0), QColor(245,158,11)},
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

void PaperGaugeChart::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Generate gauge chart"); return; }
    infoLabel_->setText(QString("%1 metrics | %2 on target | %3 avg")
        .arg(entries_.size()).arg(onTargetCount()).arg(avgValue(), 0, 'f', 0));
}

void PaperGaugeChart::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        GaugeEntry e;
        e.id = settings_.value("id").toInt();
        e.metric = settings_.value("metric").toString();
        e.value = settings_.value("value").toDouble();
        e.target = settings_.value("target").toDouble();
        e.maxVal = settings_.value("maxVal").toDouble();
        e.unit = settings_.value("unit").toString();
        e.category = settings_.value("category").toString();
        e.status = settings_.value("status").toString();
        e.rank = settings_.value("rank").toInt();
        e.onTarget = settings_.value("onTarget").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperGaugeChart::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("metric", entries_[i].metric);
        settings_.setValue("value", entries_[i].value);
        settings_.setValue("target", entries_[i].target);
        settings_.setValue("maxVal", entries_[i].maxVal);
        settings_.setValue("unit", entries_[i].unit);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("status", entries_[i].status);
        settings_.setValue("rank", entries_[i].rank);
        settings_.setValue("onTarget", entries_[i].onTarget);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
