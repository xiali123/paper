#include "visualization/PaperHeatmapGrid.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperHeatmapGrid::PaperHeatmapGrid(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "HeatmapGrid")
{
    setupUI();
    loadSettings();
}

void PaperHeatmapGrid::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperHeatmapGrid::onGenerate);
    toolbar->addWidget(generateBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Citations", "Topics", "Authors", "Journals"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperHeatmapGrid::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter dataset name for heatmap...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Generate heatmap grid");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperHeatmapGrid::addEntry(const HeatmapCell& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit heatmapGenerated(entry.id, entry.value);
    update();
}

QList<HeatmapCell> PaperHeatmapGrid::entries() const { return entries_; }

qreal PaperHeatmapGrid::totalValue() const {
    qreal t = 0;
    for (const auto& e : entries_) t += e.value;
    return t;
}

int PaperHeatmapGrid::highlightedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.highlighted) c++;
    return c;
}

QMap<QString, int> PaperHeatmapGrid::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperHeatmapGrid::onGenerate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList rows = {"ML", "NLP", "CV", "Robotics", "Theory"};
    QStringList cols = {"2023", "2024", "2025", "2026"};
    QStringList categories = {"citations", "topics", "authors", "journals"};
    QColor catColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};

    entries_.clear();
    int cIdx = categoryCombo_->currentIndex();
    for (int r = 0; r < rows.size(); ++r) {
        for (int c = 0; c < cols.size(); ++c) {
            HeatmapCell e;
            e.id = entries_.size() + 1;
            e.rowLabel = rows[r];
            e.colLabel = cols[c];
            e.value = QRandomGenerator::global()->bounded(100);
            e.cellX = c;
            e.cellY = r;
            int catIdx = cIdx == 0 ? QRandomGenerator::global()->bounded(categories.size()) : cIdx - 1;
            e.category = categories[catIdx];
            e.count = 1 + QRandomGenerator::global()->bounded(50);
            e.intensity = e.value / 100.0;
            e.highlighted = e.value >= 80;
            e.color = catColors[catIdx];
            entries_.append(e);
        }
    }
    saveSettings();
    updateInfo();
    emit heatmapGenerated(entries_.size(), totalValue());
    update();
    inputField_->clear();
}

void PaperHeatmapGrid::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Generate heatmap grid");
    update();
}

void PaperHeatmapGrid::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Generate heatmap grid");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Heatmap Grid");

    int w = width(), h = height();
    drawHeatmapView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperHeatmapGrid::drawHeatmapView(QPainter& p, const QRect& rect) {
    QStringList rows = {"ML", "NLP", "CV", "Robot", "Theory"};
    QStringList cols = {"2023", "2024", "2025", "2026"};

    int labelW = 50;
    int labelH = 18;
    int cellW = (rect.width() - labelW) / cols.size();
    int cellH = (rect.height() - labelH) / rows.size();

    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 7));
    for (int c = 0; c < cols.size(); ++c) {
        p.drawText(rect.x() + labelW + c * cellW, rect.y(), cellW, labelH,
                   Qt::AlignCenter, cols[c]);
    }
    for (int r = 0; r < rows.size(); ++r) {
        p.drawText(rect.x(), rect.y() + labelH + r * cellH, labelW, cellH,
                   Qt::AlignVCenter | Qt::AlignRight, rows[r]);
    }

    for (const auto& e : entries_) {
        int x = rect.x() + labelW + e.cellX * cellW;
        int y = rect.y() + labelH + e.cellY * cellH;

        int alpha = 40 + static_cast<int>(e.intensity * 215);
        QColor fill = QColor(e.color.red(), e.color.green(), e.color.blue(), alpha);
        p.setPen(QPen(Qt::white, 1));
        p.setBrush(fill);
        p.drawRect(x + 1, y + 1, cellW - 2, cellH - 2);

        if (cellW > 30 && cellH > 16) {
            p.setPen(alpha > 150 ? Qt::white : QColor(15, 23, 42));
            p.setFont(QFont("Arial", qMin(8, cellW / 8)));
            p.drawText(x + 1, y + 1, cellW - 2, cellH - 2, Qt::AlignCenter,
                       QString::number(e.value));
        }
    }
}

void PaperHeatmapGrid::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Legend");

    QStringList cats = {"citations", "topics", "authors", "journals"};
    QString labels[] = {"Citations", "Topics", "Authors", "Journals"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};

    auto counts = categoryCounts();
    int itemH = qMin(28, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (itemH + 4);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x(), y + 3, 14, 14, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 18, y + 1, rect.width() / 2 - 18, 18, Qt::AlignVCenter, labels[i]);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y + 1, rect.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(counts.contains(cats[i]) ? counts[cats[i]] : 0) + " cells");
    }
}

void PaperHeatmapGrid::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Cells", QString::number(entries_.size()), QColor(59,130,246)},
        {"Highlighted", QString::number(highlightedCount()), QColor(16,185,129)},
        {"Total Value", QString::number(static_cast<int>(totalValue())), QColor(245,158,11)},
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

void PaperHeatmapGrid::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Generate heatmap grid"); return; }
    infoLabel_->setText(QString("%1 cells | %2 hot | %3 total")
        .arg(entries_.size()).arg(highlightedCount()).arg(static_cast<int>(totalValue())));
}

void PaperHeatmapGrid::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        HeatmapCell e;
        e.id = settings_.value("id").toInt();
        e.rowLabel = settings_.value("rowLabel").toString();
        e.colLabel = settings_.value("colLabel").toString();
        e.value = settings_.value("value").toDouble();
        e.cellX = settings_.value("cellX").toInt();
        e.cellY = settings_.value("cellY").toInt();
        e.category = settings_.value("category").toString();
        e.count = settings_.value("count").toInt();
        e.intensity = settings_.value("intensity").toDouble();
        e.highlighted = settings_.value("highlighted").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperHeatmapGrid::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("rowLabel", entries_[i].rowLabel);
        settings_.setValue("colLabel", entries_[i].colLabel);
        settings_.setValue("value", entries_[i].value);
        settings_.setValue("cellX", entries_[i].cellX);
        settings_.setValue("cellY", entries_[i].cellY);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("count", entries_[i].count);
        settings_.setValue("intensity", entries_[i].intensity);
        settings_.setValue("highlighted", entries_[i].highlighted);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
