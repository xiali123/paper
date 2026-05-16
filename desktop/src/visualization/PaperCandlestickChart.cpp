#include "visualization/PaperCandlestickChart.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperCandlestickChart::PaperCandlestickChart(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CandlestickChart")
{
    setupUI();
    loadSettings();
}

void PaperCandlestickChart::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperCandlestickChart::onGenerate);
    toolbar->addWidget(generateBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Weekly", "Monthly", "Quarterly"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCandlestickChart::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter dataset...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Generate candlestick chart");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperCandlestickChart::addEntry(const CandleEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit chartGenerated(entry.id, entry.close);
    update();
}

QList<CandleEntry> PaperCandlestickChart::entries() const { return entries_; }

int PaperCandlestickChart::bullishCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.bullish) c++;
    return c;
}

qreal PaperCandlestickChart::maxPrice() const {
    qreal m = 0;
    for (const auto& e : entries_) m = qMax(m, e.high);
    return m;
}

QMap<QString, int> PaperCandlestickChart::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperCandlestickChart::onGenerate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"weekly", "monthly", "quarterly"};
    int cIdx = categoryCombo_->currentIndex();
    entries_.clear();
    int count = 6 + QRandomGenerator::global()->bounded(3);
    for (int i = 0; i < count; ++i) {
        CandleEntry e;
        e.id = entries_.size() + 1;
        e.label = "W" + QString::number(i + 1);
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.open = 50 + QRandomGenerator::global()->bounded(50);
        int range = QRandomGenerator::global()->bounded(20);
        e.high = e.open + range;
        int lowRange = QRandomGenerator::global()->bounded(20);
        e.low = e.open - lowRange;
        e.close = e.low + QRandomGenerator::global()->bounded(static_cast<int>(e.high - e.low) + 1);
        e.volume = 100 + QRandomGenerator::global()->bounded(900);
        e.bullish = e.close >= e.open;
        e.color = e.bullish ? QColor(16,185,129) : QColor(239,68,68);
        addEntry(e);
    }
    inputField_->clear();
}

void PaperCandlestickChart::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Generate candlestick chart");
    update();
}

void PaperCandlestickChart::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Generate candlestick chart");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Candlestick Chart");
    int w = width(), h = height();
    drawCandleView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperCandlestickChart::drawCandleView(QPainter& p, const QRect& rect) {
    if (entries_.isEmpty()) return;
    qreal minP = entries_[0].low, maxP = entries_[0].high;
    for (const auto& e : entries_) {
        minP = qMin(minP, e.low);
        maxP = qMax(maxP, e.high);
    }
    qreal range = qMax(maxP - minP, 1.0);
    int show = qMin(static_cast<int>(entries_.size()), 10);
    int candleW = qMax(12, (rect.width() - 20) / qMax(show, 1) - 6);

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int x = rect.x() + 10 + i * (candleW + 6);
        int chartH = rect.height() - 30;
        int midY = rect.y() + 15;

        // Wick line (high to low)
        int highY = midY + static_cast<int>((1.0 - (e.high - minP) / range) * chartH);
        int lowY = midY + static_cast<int>((1.0 - (e.low - minP) / range) * chartH);
        p.setPen(QPen(e.color.darker(130), 1));
        p.drawLine(x + candleW / 2, highY, x + candleW / 2, lowY);

        // Body rect (open to close)
        int openY = midY + static_cast<int>((1.0 - (e.open - minP) / range) * chartH);
        int closeY = midY + static_cast<int>((1.0 - (e.close - minP) / range) * chartH);
        int bodyTop = qMin(openY, closeY);
        int bodyH = qMax(qAbs(closeY - openY), 2);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(x, bodyTop, candleW, bodyH, 2, 2);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(x - 2, rect.y() + rect.height() - 5, candleW + 4, 12, Qt::AlignCenter, e.label);
    }
}

void PaperCandlestickChart::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"weekly", "monthly", "quarterly"};
    QString labels[] = {"Weekly", "Monthly", "Quarterly"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 3);
    for (int i = 0; i < 3; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 100));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 55, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 60, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 63 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperCandlestickChart::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Candles", QString::number(entries_.size()), QColor(59,130,246)},
        {"Bullish", QString::number(bullishCount()), QColor(16,185,129)},
        {"Max Price", QString::number(maxPrice(), 'f', 1), QColor(245,158,11)},
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

void PaperCandlestickChart::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Generate candlestick chart"); return; }
    infoLabel_->setText(QString("%1 candles | %2 bullish | %3 max price")
        .arg(entries_.size()).arg(bullishCount()).arg(maxPrice(), 0, 'f', 1));
}

void PaperCandlestickChart::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        CandleEntry e;
        e.id = settings_.value("id").toInt();
        e.label = settings_.value("label").toString();
        e.category = settings_.value("category").toString();
        e.open = settings_.value("open").toDouble();
        e.high = settings_.value("high").toDouble();
        e.low = settings_.value("low").toDouble();
        e.close = settings_.value("close").toDouble();
        e.volume = settings_.value("volume").toDouble();
        e.bullish = settings_.value("bullish").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperCandlestickChart::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("label", entries_[i].label);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("open", entries_[i].open);
        settings_.setValue("high", entries_[i].high);
        settings_.setValue("low", entries_[i].low);
        settings_.setValue("close", entries_[i].close);
        settings_.setValue("volume", entries_[i].volume);
        settings_.setValue("bullish", entries_[i].bullish);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
