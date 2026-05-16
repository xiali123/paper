#include "visualization/PaperRadarSpinner.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperRadarSpinner::PaperRadarSpinner(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "RadarSpinner")
{
    setupUI();
    loadSettings();
}

void PaperRadarSpinner::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperRadarSpinner::onGenerate);
    toolbar->addWidget(generateBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Quality", "Impact", "Novelty"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperRadarSpinner::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter radar dataset...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Generate radar chart");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(600, 500);
}

void PaperRadarSpinner::addEntry(const RadarEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit radarGenerated(entry.id, entry.value);
    update();
}

QList<RadarEntry> PaperRadarSpinner::entries() const { return entries_; }

int PaperRadarSpinner::aboveCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.above) c++;
    return c;
}

qreal PaperRadarSpinner::avgValue() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.value;
    return sum / entries_.size();
}

QMap<QString, int> PaperRadarSpinner::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperRadarSpinner::onGenerate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"quality", "impact", "novelty"};
    QStringList labels = {"Rigor", "Clarity", "Originality", "Significance", "Reproducibility", "Breadth"};
    int cIdx = categoryCombo_->currentIndex();
    entries_.clear();
    int count = labels.size();
    for (int i = 0; i < count; ++i) {
        RadarEntry e;
        e.id = entries_.size() + 1;
        e.label = labels[i];
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.value = 30 + QRandomGenerator::global()->bounded(70);
        e.target = 60 + QRandomGenerator::global()->bounded(30);
        e.maxVal = 100;
        e.axis = i;
        e.above = e.value >= e.target;
        QColor catColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11)};
        int ci = categories.indexOf(e.category);
        e.color = catColors[ci >= 0 ? ci : 0];
        entries_.append(e);
    }
    saveSettings();
    updateInfo();
    emit radarGenerated(entries_.size(), avgValue());
    update();
    inputField_->clear();
}

void PaperRadarSpinner::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Generate radar chart");
    update();
}

void PaperRadarSpinner::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Generate radar chart");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Radar Chart");
    int w = width(), h = height();
    drawRadarView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperRadarSpinner::drawRadarView(QPainter& p, const QRect& rect) {
    int n = entries_.size();
    if (n < 3) return;
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + rect.height() / 2;
    int radius = qMin(rect.width(), rect.height()) / 2 - 20;
    // Draw axis lines and labels
    for (int i = 0; i < n; ++i) {
        qreal angle = (2 * M_PI * i / n) - M_PI / 2;
        int ax = cx + static_cast<int>(radius * qCos(angle));
        int ay = cy + static_cast<int>(radius * qSin(angle));
        p.setPen(QColor(203, 213, 225));
        p.drawLine(cx, cy, ax, ay);
        int lx = cx + static_cast<int>((radius + 15) * qCos(angle));
        int ly = cy + static_cast<int>((radius + 15) * qSin(angle));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 6));
        p.drawText(lx - 20, ly - 6, 40, 12, Qt::AlignCenter, entries_[i].label.left(8));
    }
    // Draw target polygon
    QPolygonF targetPoly;
    for (int i = 0; i < n; ++i) {
        qreal angle = (2 * M_PI * i / n) - M_PI / 2;
        qreal r = (entries_[i].target / entries_[i].maxVal) * radius;
        targetPoly << QPointF(cx + r * qCos(angle), cy + r * qSin(angle));
    }
    p.setPen(QPen(QColor(245,158,11), 1, Qt::DashLine));
    p.setBrush(Qt::NoBrush);
    p.drawPolygon(targetPoly);
    // Draw value polygon
    QPolygonF valuePoly;
    for (int i = 0; i < n; ++i) {
        qreal angle = (2 * M_PI * i / n) - M_PI / 2;
        qreal r = (entries_[i].value / entries_[i].maxVal) * radius;
        valuePoly << QPointF(cx + r * qCos(angle), cy + r * qSin(angle));
    }
    p.setPen(QPen(QColor(59,130,246), 2));
    p.setBrush(QColor(59,130,246, 40));
    p.drawPolygon(valuePoly);
}

void PaperRadarSpinner::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"quality", "impact", "novelty"};
    QString labels[] = {"Quality", "Impact", "Novelty"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11)};
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
                   Qt::AlignVCenter | Qt::AlignRight, QString::number(count) + " axes");
    }
}

void PaperRadarSpinner::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Axes", QString::number(entries_.size()), QColor(59,130,246)},
        {"Above", QString::number(aboveCount()), QColor(16,185,129)},
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

void PaperRadarSpinner::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Generate radar chart"); return; }
    infoLabel_->setText(QString("%1 axes | %2 above | %3 avg")
        .arg(entries_.size()).arg(aboveCount()).arg(avgValue(), 0, 'f', 0));
}

void PaperRadarSpinner::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        RadarEntry e;
        e.id = settings_.value("id").toInt();
        e.label = settings_.value("label").toString();
        e.category = settings_.value("category").toString();
        e.value = settings_.value("value").toDouble();
        e.target = settings_.value("target").toDouble();
        e.maxVal = settings_.value("maxVal").toDouble();
        e.axis = settings_.value("axis").toInt();
        e.above = settings_.value("above").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperRadarSpinner::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("label", entries_[i].label);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("value", entries_[i].value);
        settings_.setValue("target", entries_[i].target);
        settings_.setValue("maxVal", entries_[i].maxVal);
        settings_.setValue("axis", entries_[i].axis);
        settings_.setValue("above", entries_[i].above);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
