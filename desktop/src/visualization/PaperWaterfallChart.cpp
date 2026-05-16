#include "visualization/PaperWaterfallChart.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperWaterfallChart::PaperWaterfallChart(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "WaterfallChart")
{
    setupUI();
    loadSettings();
}

void PaperWaterfallChart::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperWaterfallChart::onGenerate);
    toolbar->addWidget(generateBtn_);
    toolbar->addWidget(new QLabel("Period:"));
    periodCombo_ = new QComboBox();
    periodCombo_->addItems({"Q1", "Q2", "Q3", "Q4"});
    toolbar->addWidget(periodCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperWaterfallChart::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter dataset for waterfall chart...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Generate waterfall chart");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(600, 500);
}

void PaperWaterfallChart::addEntry(const WaterfallEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit waterfallGenerated(entry.id, entry.value);
    update();
}

QList<WaterfallEntry> PaperWaterfallChart::entries() const { return entries_; }

qreal PaperWaterfallChart::totalValue() const {
    qreal t = 0;
    for (const auto& e : entries_) t += e.value;
    return t;
}

int PaperWaterfallChart::positiveCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.positive) c++;
    return c;
}

QMap<QString, int> PaperWaterfallChart::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperWaterfallChart::onGenerate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"citations", "downloads", "views", "shares"};
    QStringList labels = {"Start", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "End"};
    entries_.clear();
    qreal cumulative = 0;
    int count = 6 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        WaterfallEntry e;
        e.id = entries_.size() + 1;
        e.label = (i == 0) ? "Start" : ((i == count - 1) ? "End" : labels[1 + QRandomGenerator::global()->bounded(6)]);
        e.value = (i == 0) ? 100.0 : (i == count - 1) ? 0 : (-30 + QRandomGenerator::global()->bounded(80));
        cumulative += e.value;
        e.cumulative = cumulative;
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.positive = e.value >= 0;
        e.order = i;
        e.period = periodCombo_->currentText();
        e.percentage = (i == 0) ? 100.0 : qAbs(e.value);
        e.color = e.positive ? QColor(16,185,129) : QColor(239,68,68);
        entries_.append(e);
    }
    saveSettings();
    updateInfo();
    emit waterfallGenerated(entries_.size(), totalValue());
    update();
    inputField_->clear();
}

void PaperWaterfallChart::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Generate waterfall chart");
    update();
}

void PaperWaterfallChart::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Generate waterfall chart");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Waterfall Chart");
    int w = width(), h = height();
    drawWaterfallView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperWaterfallChart::drawWaterfallView(QPainter& p, const QRect& rect) {
    int margin = 20;
    int plotW = rect.width() - margin * 2;
    int plotH = rect.height() - margin * 2;
    qreal maxVal = 1;
    for (const auto& e : entries_) maxVal = qMax(maxVal, qAbs(e.cumulative));
    int n = entries_.size();
    int barW = qMax(8, (plotW - 10) / qMax(n, 1) - 4);
    p.setPen(QPen(QColor(203, 213, 225), 1));
    p.drawLine(rect.x() + margin, rect.y() + margin + plotH / 2,
               rect.x() + margin + plotW, rect.y() + margin + plotH / 2);
    for (int i = 0; i < n; ++i) {
        const auto& e = entries_[i];
        int x = rect.x() + margin + i * ((plotW - 10) / qMax(n, 1));
        int barH = static_cast<int>((e.value / maxVal) * (plotH / 2 - 10));
        int y;
        if (e.positive) {
            y = rect.y() + margin + plotH / 2 - barH;
        } else {
            y = rect.y() + margin + plotH / 2;
        }
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(x, y, barW, qAbs(barH), 2, 2);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 6));
        p.drawText(x - 2, rect.y() + margin + plotH + 2, barW + 4, 12, Qt::AlignCenter, e.label.left(4));
    }
}

void PaperWaterfallChart::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"citations", "downloads", "views", "shares"};
    QString labels[] = {"Citations", "Downloads", "Views", "Shares"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int itemH = qMin(28, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
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
                   QString::number(count) + " items");
    }
}

void PaperWaterfallChart::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Items", QString::number(entries_.size()), QColor(59,130,246)},
        {"Positive", QString::number(positiveCount()), QColor(16,185,129)},
        {"Total", QString::number(totalValue(), 'f', 0), QColor(245,158,11)},
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

void PaperWaterfallChart::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Generate waterfall chart"); return; }
    infoLabel_->setText(QString("%1 items | %2 pos | total %3")
        .arg(entries_.size()).arg(positiveCount()).arg(totalValue(), 0, 'f', 0));
}

void PaperWaterfallChart::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        WaterfallEntry e;
        e.id = settings_.value("id").toInt();
        e.label = settings_.value("label").toString();
        e.value = settings_.value("value").toDouble();
        e.cumulative = settings_.value("cumulative").toDouble();
        e.category = settings_.value("category").toString();
        e.positive = settings_.value("positive").toBool();
        e.order = settings_.value("order").toInt();
        e.period = settings_.value("period").toString();
        e.percentage = settings_.value("percentage").toDouble();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperWaterfallChart::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("label", entries_[i].label);
        settings_.setValue("value", entries_[i].value);
        settings_.setValue("cumulative", entries_[i].cumulative);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("positive", entries_[i].positive);
        settings_.setValue("order", entries_[i].order);
        settings_.setValue("period", entries_[i].period);
        settings_.setValue("percentage", entries_[i].percentage);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
