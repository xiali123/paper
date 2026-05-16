#include "visualization/PaperRadarChart.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <cmath>

PaperRadarChart::PaperRadarChart(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "RadarChart")
{
    setupUI();
    loadSettings();
}

void PaperRadarChart::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperRadarChart::onGenerate);
    toolbar->addWidget(generateBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Research Quality", "Impact", "Methodology", "Novelty"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperRadarChart::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter paper title for radar chart...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Generate radar chart");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperRadarChart::addEntry(const RadarEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit radarGenerated(entry.id, entry.value);
    update();
}

QList<RadarEntry> PaperRadarChart::entries() const { return entries_; }

qreal PaperRadarChart::totalValue() const {
    qreal t = 0;
    for (const auto& e : entries_) t += e.value;
    return t;
}

int PaperRadarChart::highlightedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.highlighted) c++;
    return c;
}

QMap<QString, int> PaperRadarChart::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperRadarChart::onGenerate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList dimensions = {"Originality", "Rigor", "Significance", "Clarity", "Reproducibility",
                              "Impact", "Novelty", "Soundness"};
    QStringList categories = {"quality", "impact", "methodology", "novelty"};
    QColor catColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};

    entries_.clear();
    int cIdx = categoryCombo_->currentIndex();
    int count = 6 + QRandomGenerator::global()->bounded(3);
    for (int i = 0; i < count; ++i) {
        RadarEntry e;
        e.id = entries_.size() + 1;
        e.dimension = dimensions[i % dimensions.size()];
        e.maxValue = 10.0;
        e.value = 2 + QRandomGenerator::global()->bounded(80) / 10.0;
        e.category = cIdx == 0 ? categories[i % categories.size()] : categories[cIdx - 1];
        e.rank = i + 1;
        e.normalized = e.value / e.maxValue;
        e.highlighted = e.value >= 8.0;
        int catIdx = 0;
        for (int j = 0; j < categories.size(); ++j) if (categories[j] == e.category) catIdx = j;
        e.color = catColors[catIdx];
        entries_.append(e);
    }
    saveSettings();
    updateInfo();
    emit radarGenerated(entries_.size(), totalValue());
    update();
    inputField_->clear();
}

void PaperRadarChart::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Generate radar chart");
    update();
}

void PaperRadarChart::paintEvent(QPaintEvent*) {
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
    drawDimensionLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperRadarChart::drawRadarView(QPainter& p, const QRect& rect) {
    int n = entries_.size();
    if (n < 3) return;

    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + rect.height() / 2;
    int radius = qMin(rect.width(), rect.height()) / 2 - 30;

    for (int ring = 1; ring <= 4; ++ring) {
        int r = radius * ring / 4;
        p.setPen(QPen(QColor(203, 213, 225), 1));
        p.setBrush(Qt::NoBrush);
        p.drawPolygon([&]() -> QPolygon {
            QPolygon poly;
            for (int i = 0; i < n; ++i) {
                qreal angle = (2 * M_PI * i / n) - M_PI / 2;
                poly << QPoint(cx + static_cast<int>(r * qCos(angle)),
                               cy + static_cast<int>(r * qSin(angle)));
            }
            return poly;
        }());
    }

    for (int i = 0; i < n; ++i) {
        qreal angle = (2 * M_PI * i / n) - M_PI / 2;
        p.setPen(QPen(QColor(203, 213, 225), 1));
        p.drawLine(cx, cy,
                   cx + static_cast<int>(radius * qCos(angle)),
                   cy + static_cast<int>(radius * qSin(angle)));

        int lx = cx + static_cast<int>((radius + 15) * qCos(angle));
        int ly = cy + static_cast<int>((radius + 15) * qSin(angle));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(lx - 25, ly - 6, 50, 12, Qt::AlignCenter,
                   entries_[i].dimension.left(8));
    }

    QPolygon dataPoly;
    for (int i = 0; i < n; ++i) {
        qreal angle = (2 * M_PI * i / n) - M_PI / 2;
        int r = static_cast<int>(entries_[i].normalized * radius);
        dataPoly << QPoint(cx + static_cast<int>(r * qCos(angle)),
                           cy + static_cast<int>(r * qSin(angle)));
    }
    QColor fillColor = QColor(59, 130, 246, 60);
    p.setPen(QPen(QColor(59, 130, 246), 2));
    p.setBrush(fillColor);
    p.drawPolygon(dataPoly);

    for (int i = 0; i < n; ++i) {
        qreal angle = (2 * M_PI * i / n) - M_PI / 2;
        int r = static_cast<int>(entries_[i].normalized * radius);
        int px = cx + static_cast<int>(r * qCos(angle));
        int py = cy + static_cast<int>(r * qSin(angle));
        p.setPen(Qt::NoPen);
        p.setBrush(entries_[i].highlighted ? QColor(239,68,68) : QColor(59,130,246));
        p.drawEllipse(px - 3, py - 3, 6, 6);
    }
}

void PaperRadarChart::drawDimensionLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Dimensions");

    int show = qMin(static_cast<int>(entries_.size()), 8);
    int itemH = qMin(24, (rect.height() - 30) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + 22 + i * (itemH + 2);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y + 2, 10, 10, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 14, y, rect.width() / 2 - 14, 14, Qt::AlignVCenter,
                   e.dimension.left(12));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + rect.width() / 2, y, rect.width() / 2 - 5, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.value, 'f', 1) + "/" + QString::number(e.maxValue, 'f', 0));
    }
}

void PaperRadarChart::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Dimensions", QString::number(entries_.size()), QColor(59,130,246)},
        {"Highlighted", QString::number(highlightedCount()), QColor(16,185,129)},
        {"Total Value", QString::number(totalValue(), 'f', 1), QColor(245,158,11)},
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

void PaperRadarChart::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Generate radar chart"); return; }
    infoLabel_->setText(QString("%1 dims | %2 highlighted | %3 total")
        .arg(entries_.size()).arg(highlightedCount()).arg(totalValue(), 0, 'f', 1));
}

void PaperRadarChart::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        RadarEntry e;
        e.id = settings_.value("id").toInt();
        e.dimension = settings_.value("dimension").toString();
        e.value = settings_.value("value").toDouble();
        e.maxValue = settings_.value("maxValue").toDouble();
        e.category = settings_.value("category").toString();
        e.rank = settings_.value("rank").toInt();
        e.normalized = settings_.value("normalized").toDouble();
        e.highlighted = settings_.value("highlighted").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperRadarChart::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("dimension", entries_[i].dimension);
        settings_.setValue("value", entries_[i].value);
        settings_.setValue("maxValue", entries_[i].maxValue);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("rank", entries_[i].rank);
        settings_.setValue("normalized", entries_[i].normalized);
        settings_.setValue("highlighted", entries_[i].highlighted);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
