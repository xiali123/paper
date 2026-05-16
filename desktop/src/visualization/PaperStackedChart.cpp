#include "visualization/PaperStackedChart.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperStackedChart::PaperStackedChart(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "StackedChart")
{
    setupUI();
    loadSettings();
}

void PaperStackedChart::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperStackedChart::onGenerate);
    toolbar->addWidget(generateBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Type-A", "Type-B", "Type-C"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperStackedChart::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter dataset name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Generate stacked chart");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(600, 500);
}

void PaperStackedChart::addEntry(const StackEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit chartGenerated(entry.id, entry.total);
    update();
}

QList<StackEntry> PaperStackedChart::entries() const { return entries_; }

int PaperStackedChart::highlightedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.highlighted) c++;
    return c;
}

qreal PaperStackedChart::maxTotal() const {
    qreal m = 0;
    for (const auto& e : entries_) m = qMax(m, e.total);
    return m;
}

QMap<QString, int> PaperStackedChart::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperStackedChart::onGenerate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"type-a", "type-b", "type-c"};
    QStringList labels = {"Q1", "Q2", "Q3", "Q4", "H1", "H2"};
    int cIdx = categoryCombo_->currentIndex();
    entries_.clear();
    int count = 4 + QRandomGenerator::global()->bounded(3);
    for (int i = 0; i < count; ++i) {
        StackEntry e;
        e.id = entries_.size() + 1;
        e.label = labels[i < labels.size() ? i : labels.size() - 1];
        e.group = text.left(6) + " g" + QString::number(i);
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.value1 = 10 + QRandomGenerator::global()->bounded(40);
        e.value2 = 10 + QRandomGenerator::global()->bounded(30);
        e.value3 = 5 + QRandomGenerator::global()->bounded(20);
        e.total = e.value1 + e.value2 + e.value3;
        e.highlighted = e.total >= 70;
        e.color = e.highlighted ? QColor(245,158,11) : QColor(59,130,246);
        entries_.append(e);
    }
    saveSettings();
    updateInfo();
    emit chartGenerated(entries_.size(), maxTotal());
    update();
    inputField_->clear();
}

void PaperStackedChart::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Generate stacked chart");
    update();
}

void PaperStackedChart::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Generate stacked chart");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Stacked Chart");
    int w = width(), h = height();
    drawStackedView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperStackedChart::drawStackedView(QPainter& p, const QRect& rect) {
    qreal mt = maxTotal();
    if (mt <= 0) return;
    int margin = 25;
    int plotW = rect.width() - margin - 10;
    int plotH = rect.height() - 20;
    int barW = qMin(40, (plotW - 10) / qMax(entries_.size(), 1) - 5);
    QColor segColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11)};
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        int bx = rect.x() + margin + i * (barW + 5);
        qreal vals[] = {e.value1, e.value2, e.value3};
        int cy = rect.y() + plotH;
        for (int s = 0; s < 3; ++s) {
            int segH = static_cast<int>((vals[s] / mt) * plotH);
            p.setPen(Qt::NoPen);
            p.setBrush(segColors[s]);
            p.drawRoundedRect(bx, cy - segH, barW, segH, 2, 2);
            cy -= segH;
        }
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(bx, rect.y() + plotH + 4, barW, 14, Qt::AlignCenter, e.label);
    }
}

void PaperStackedChart::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Segments");
    QString labels[] = {"Value 1", "Value 2", "Value 3"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11)};
    int itemH = qMin(28, (rect.height() - 60) / 4);
    for (int i = 0; i < 3; ++i) {
        int y = rect.y() + 22 + i * (itemH + 4);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 5, y + 4, 14, 14, 3, 3);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18, Qt::AlignVCenter, labels[i]);
    }
    // highlight
    int y = rect.y() + 22 + 3 * (itemH + 4);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(245,158,11));
    p.drawRoundedRect(rect.x() + 5, y + 4, 14, 14, 3, 3);
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 9, QFont::Bold));
    p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18, Qt::AlignVCenter, "Highlighted");
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 8));
    p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
               Qt::AlignVCenter | Qt::AlignRight, QString::number(highlightedCount()));
}

void PaperStackedChart::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Bars", QString::number(entries_.size()), QColor(59,130,246)},
        {"Highlighted", QString::number(highlightedCount()), QColor(245,158,11)},
        {"Max Total", QString::number(maxTotal(), 'f', 0), QColor(16,185,129)},
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

void PaperStackedChart::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Generate stacked chart"); return; }
    infoLabel_->setText(QString("%1 bars | %2 highlighted | %3 max")
        .arg(entries_.size()).arg(highlightedCount()).arg(maxTotal(), 0, 'f', 0));
}

void PaperStackedChart::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        StackEntry e;
        e.id = settings_.value("id").toInt();
        e.label = settings_.value("label").toString();
        e.group = settings_.value("group").toString();
        e.category = settings_.value("category").toString();
        e.value1 = settings_.value("value1").toDouble();
        e.value2 = settings_.value("value2").toDouble();
        e.value3 = settings_.value("value3").toDouble();
        e.total = settings_.value("total").toDouble();
        e.highlighted = settings_.value("highlighted").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperStackedChart::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("label", entries_[i].label);
        settings_.setValue("group", entries_[i].group);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("value1", entries_[i].value1);
        settings_.setValue("value2", entries_[i].value2);
        settings_.setValue("value3", entries_[i].value3);
        settings_.setValue("total", entries_[i].total);
        settings_.setValue("highlighted", entries_[i].highlighted);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
