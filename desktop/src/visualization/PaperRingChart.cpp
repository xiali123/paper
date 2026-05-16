#include "visualization/PaperRingChart.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperRingChart::PaperRingChart(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "RingChart")
{
    setupUI();
    loadSettings();
}

void PaperRingChart::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperRingChart::onGenerate);
    toolbar->addWidget(generateBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Progress", "Budget", "Quality"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperRingChart::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter ring dataset...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Generate ring chart");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(600, 500);
}

void PaperRingChart::addEntry(const RingSegment& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit ringGenerated(entry.id, entry.value);
    update();
}

QList<RingSegment> PaperRingChart::entries() const { return entries_; }

int PaperRingChart::completeCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.complete) c++;
    return c;
}

qreal PaperRingChart::totalValue() const {
    qreal t = 0;
    for (const auto& e : entries_) t += e.value;
    return t;
}

QMap<QString, int> PaperRingChart::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperRingChart::onGenerate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"progress", "budget", "quality"};
    QStringList labels = {"Research", "Writing", "Review", "Data", "Analysis"};
    int cIdx = categoryCombo_->currentIndex();
    entries_.clear();
    int count = 4 + QRandomGenerator::global()->bounded(2);
    for (int i = 0; i < count; ++i) {
        RingSegment e;
        e.id = entries_.size() + 1;
        e.label = labels[i < labels.size() ? i : labels.size() - 1];
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.target = 100;
        e.value = 30 + QRandomGenerator::global()->bounded(70);
        e.angle = 360.0 / count;
        e.complete = e.value >= e.target * 0.9;
        QColor catColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11)};
        int ci = categories.indexOf(e.category);
        e.color = catColors[ci >= 0 ? ci : 0];
        entries_.append(e);
    }
    saveSettings();
    updateInfo();
    emit ringGenerated(entries_.size(), totalValue());
    update();
    inputField_->clear();
}

void PaperRingChart::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Generate ring chart");
    update();
}

void PaperRingChart::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Generate ring chart");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Ring Chart");
    int w = width(), h = height();
    drawRingView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperRingChart::drawRingView(QPainter& p, const QRect& rect) {
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + rect.height() / 2;
    int outerR = qMin(rect.width(), rect.height()) / 2 - 15;
    int innerR = outerR * 0.6;
    qreal startAngle = 0;
    for (const auto& e : entries_) {
        qreal span = (e.value / totalValue()) * 360.0 * 16;
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawPie(cx - outerR, cy - outerR, outerR * 2, outerR * 2,
                  static_cast<int>(startAngle), static_cast<int>(span));
        startAngle += span;
    }
    // inner circle (donut hole)
    p.setBrush(Qt::white);
    p.drawEllipse(cx - innerR, cy - innerR, innerR * 2, innerR * 2);
    // center text
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 16, QFont::Bold));
    p.drawText(QRect(cx - innerR, cy - 15, innerR * 2, 30), Qt::AlignCenter,
               QString::number(totalValue(), 'f', 0));
}

void PaperRingChart::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Segments");
    int show = qMin(5, entries_.size());
    int itemH = qMin(28, (rect.height() - 30) / qMax(show, 1));
    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + 22 + i * (itemH + 4);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x() + 5, y + 4, 14, 14, 3, 3);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18, Qt::AlignVCenter, e.label);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.value, 'f', 0) + (e.complete ? " [OK]" : ""));
    }
}

void PaperRingChart::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Segments", QString::number(entries_.size()), QColor(59,130,246)},
        {"Complete", QString::number(completeCount()), QColor(16,185,129)},
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

void PaperRingChart::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Generate ring chart"); return; }
    infoLabel_->setText(QString("%1 segments | %2 complete | %3 total")
        .arg(entries_.size()).arg(completeCount()).arg(totalValue(), 0, 'f', 0));
}

void PaperRingChart::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        RingSegment e;
        e.id = settings_.value("id").toInt();
        e.label = settings_.value("label").toString();
        e.category = settings_.value("category").toString();
        e.value = settings_.value("value").toDouble();
        e.target = settings_.value("target").toDouble();
        e.angle = settings_.value("angle").toDouble();
        e.complete = settings_.value("complete").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperRingChart::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("label", entries_[i].label);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("value", entries_[i].value);
        settings_.setValue("target", entries_[i].target);
        settings_.setValue("angle", entries_[i].angle);
        settings_.setValue("complete", entries_[i].complete);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
