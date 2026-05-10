#include "visualization/PaperFunnelChart.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperFunnelChart::PaperFunnelChart(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "FunnelChart")
{
    setupUI();
    loadSettings();
}

void PaperFunnelChart::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperFunnelChart::onGenerate);
    toolbar->addWidget(generateBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Search", "View", "Download", "Cite"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperFunnelChart::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter funnel dataset...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Generate funnel chart");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(600, 500);
}

void PaperFunnelChart::addEntry(const FunnelEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit funnelGenerated(entry.id, entry.count);
    update();
}

QList<FunnelEntry> PaperFunnelChart::entries() const { return entries_; }

int PaperFunnelChart::totalCount() const {
    int t = 0;
    for (const auto& e : entries_) t += e.count;
    return t;
}

int PaperFunnelChart::bottleneckCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.bottleneck) c++;
    return c;
}

QMap<QString, int> PaperFunnelChart::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperFunnelChart::onGenerate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList stages = {"Search Results", "Page Views", "Abstract Read", "PDF Download", "Citation"};
    QStringList categories = {"search", "view", "download", "cite"};
    int cIdx = categoryCombo_->currentIndex();
    entries_.clear();
    int baseCount = 500 + QRandomGenerator::global()->bounded(500);
    for (int i = 0; i < 5; ++i) {
        FunnelEntry e;
        e.id = entries_.size() + 1;
        e.stage = stages[i];
        int drop = 10 + QRandomGenerator::global()->bounded(30);
        e.count = qMax(10, baseCount * (100 - drop * i) / 100);
        e.percentage = static_cast<qreal>(e.count) / baseCount;
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.dropoff = (i > 0) ? (entries_[i - 1].count - e.count) : 0;
        e.conversionRate = (i > 0 && entries_[i - 1].count > 0) ? static_cast<qreal>(e.count) / entries_[i - 1].count : 1.0;
        e.color_label = stages[i];
        e.order = i;
        e.bottleneck = e.conversionRate < 0.5 && i > 0;
        e.color = e.bottleneck ? QColor(239,68,68) : (e.conversionRate >= 0.7 ? QColor(16,185,129) : QColor(245,158,11));
        entries_.append(e);
    }
    saveSettings();
    updateInfo();
    emit funnelGenerated(entries_.size(), totalCount());
    update();
    inputField_->clear();
}

void PaperFunnelChart::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Generate funnel chart");
    update();
}

void PaperFunnelChart::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Generate funnel chart");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Funnel Chart");
    int w = width(), h = height();
    drawFunnelView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperFunnelChart::drawFunnelView(QPainter& p, const QRect& rect) {
    int n = entries_.size();
    if (n == 0) return;
    int maxCount = entries_[0].count;
    int funnelH = rect.height() - 20;
    int stageH = funnelH / qMax(n, 1);
    for (int i = 0; i < n; ++i) {
        const auto& e = entries_[i];
        int barW = static_cast<int>((static_cast<qreal>(e.count) / maxCount) * (rect.width() - 40));
        int x = rect.x() + (rect.width() - barW) / 2;
        int y = rect.y() + 10 + i * stageH;
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(150));
        p.drawRoundedRect(x, y, barW, stageH - 6, 4, 4);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(x + 10, y + 2, barW - 20, 16, Qt::AlignVCenter, e.stage);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(x + 10, y + 16, barW - 20, 14, Qt::AlignVCenter,
                   QString::number(e.count) + " | " + QString::number(e.conversionRate * 100, 'f', 0) + "% conv");
    }
}

void PaperFunnelChart::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"search", "view", "download", "cite"};
    QString labels[] = {"Search", "View", "Download", "Cite"};
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
                   QString::number(count) + " stages");
    }
}

void PaperFunnelChart::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Stages", QString::number(entries_.size()), QColor(59,130,246)},
        {"Bottlenecks", QString::number(bottleneckCount()), QColor(239,68,68)},
        {"Total Count", QString::number(totalCount()), QColor(245,158,11)},
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

void PaperFunnelChart::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Generate funnel chart"); return; }
    infoLabel_->setText(QString("%1 stages | %2 bottlenecks | %3 total")
        .arg(entries_.size()).arg(bottleneckCount()).arg(totalCount()));
}

void PaperFunnelChart::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        FunnelEntry e;
        e.id = settings_.value("id").toInt();
        e.stage = settings_.value("stage").toString();
        e.count = settings_.value("count").toInt();
        e.percentage = settings_.value("percentage").toDouble();
        e.category = settings_.value("category").toString();
        e.dropoff = settings_.value("dropoff").toInt();
        e.conversionRate = settings_.value("conversionRate").toDouble();
        e.color_label = settings_.value("color_label").toString();
        e.order = settings_.value("order").toInt();
        e.bottleneck = settings_.value("bottleneck").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperFunnelChart::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("stage", entries_[i].stage);
        settings_.setValue("count", entries_[i].count);
        settings_.setValue("percentage", entries_[i].percentage);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("dropoff", entries_[i].dropoff);
        settings_.setValue("conversionRate", entries_[i].conversionRate);
        settings_.setValue("color_label", entries_[i].color_label);
        settings_.setValue("order", entries_[i].order);
        settings_.setValue("bottleneck", entries_[i].bottleneck);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
