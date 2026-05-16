#include "visualization/PaperBubbleMatrix2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainterPath>
#include <QtMath>

namespace {
const QColor kCorrelationColor(0x3b, 0x82, 0xf6);
const QColor kDistributionColor(0x16, 0xa3, 0x4a);
const QColor kFrequencyColor(0x7c, 0x3a, 0xed);
const QColor kImpactColor(0xd9, 0x77, 0x06);

QColor categoryColor(const QString& category) {
    QString lower = category.toLower();
    if (lower == "correlation")   return kCorrelationColor;
    if (lower == "distribution")  return kDistributionColor;
    if (lower == "frequency")     return kFrequencyColor;
    if (lower == "impact")        return kImpactColor;
    return QColor(0x94, 0xa3, 0xb8);
}

struct CategoryInfo {
    QString key;
    QString label;
    QColor color;
};

const CategoryInfo kCategories[] = {
    {"correlation",  "Correlation",  kCorrelationColor},
    {"distribution", "Distribution", kDistributionColor},
    {"frequency",    "Frequency",    kFrequencyColor},
    {"impact",       "Impact",       kImpactColor},
};
constexpr int kCategoryCount = 4;
}

PaperBubbleMatrix2::PaperBubbleMatrix2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "BubbleMatrix2")
{
    setupUI();
    loadSettings();
}

void PaperBubbleMatrix2::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Correlation", "Distribution", "Frequency", "Impact"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Row,Column:value,size");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 2);

    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperBubbleMatrix2::onRender);
    toolbar->addWidget(renderBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperBubbleMatrix2::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Render a bubble matrix");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperBubbleMatrix2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Render a bubble matrix");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Bubble Matrix 2");

    int w = width(), h = height();
    drawBubbleMatrix(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperBubbleMatrix2::drawBubbleMatrix(QPainter& p, const QRect& rect) {
    QStringList uniqueRows;
    QStringList uniqueCols;
    for (const auto& e : entries_) {
        if (!uniqueRows.contains(e.row))   uniqueRows.append(e.row);
        if (!uniqueCols.contains(e.column)) uniqueCols.append(e.column);
    }

    int nRows = qMax(uniqueRows.size(), 1);
    int nCols = qMax(uniqueCols.size(), 1);
    int labelW = 50;
    int labelH = 22;
    int cellW = (rect.width() - labelW) / nCols;
    int cellH = (rect.height() - labelH) / nRows;

    // Column headers
    p.setFont(QFont("Arial", 8));
    for (int c = 0; c < uniqueCols.size(); ++c) {
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + labelW + c * cellW, rect.y(),
                   cellW, labelH, Qt::AlignCenter, uniqueCols[c]);
    }

    // Row headers
    for (int r = 0; r < uniqueRows.size(); ++r) {
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x(), rect.y() + labelH + r * cellH,
                   labelW - 4, cellH, Qt::AlignVCenter | Qt::AlignRight, uniqueRows[r]);
    }

    // Grid lines
    p.setPen(QPen(QColor(226, 232, 240), 1));
    for (int r = 0; r <= nRows; ++r) {
        int y = rect.y() + labelH + r * cellH;
        p.drawLine(rect.x() + labelW, y, rect.x() + rect.width(), y);
    }
    for (int c = 0; c <= nCols; ++c) {
        int x = rect.x() + labelW + c * cellW;
        p.drawLine(x, rect.y() + labelH, x, rect.y() + rect.height());
    }

    // Filter by category combo
    int comboIdx = categoryCombo_->currentIndex();
    QString filterCategory;
    if (comboIdx >= 1 && comboIdx <= kCategoryCount) {
        filterCategory = kCategories[comboIdx - 1].key;
    }

    // Draw bubbles using QPainterPath circles
    for (const auto& e : entries_) {
        if (!filterCategory.isEmpty() && e.category.toLower() != filterCategory) continue;

        int ri = uniqueRows.indexOf(e.row);
        int ci = uniqueCols.indexOf(e.column);
        if (ri < 0 || ci < 0) continue;

        qreal cx = rect.x() + labelW + ci * cellW + cellW / 2.0;
        qreal cy = rect.y() + labelH + ri * cellH + cellH / 2.0;
        int maxRadius = qMin(cellW, cellH) / 2 - 4;
        qreal radius = qBound(4.0, e.size * maxRadius / 50.0, static_cast<qreal>(maxRadius));

        QColor fillColor = categoryColor(e.category);
        fillColor.setAlpha(160);

        // Highlight glow border
        if (e.highlight) {
            QPainterPath glowPath;
            glowPath.addEllipse(QPointF(cx, cy), radius + 4, radius + 4);
            p.setPen(Qt::NoPen);
            QColor glowColor = fillColor;
            glowColor.setAlpha(70);
            p.setBrush(glowColor);
            p.drawPath(glowPath);
        }

        // Main bubble circle
        QPainterPath bubblePath;
        bubblePath.addEllipse(QPointF(cx, cy), radius, radius);
        p.setPen(e.highlight ? QPen(QColor(15, 23, 42), 2) : QPen(fillColor.darker(120), 1));
        p.setBrush(fillColor);
        p.drawPath(bubblePath);

        // Value label inside bubble
        p.setPen(QColor(255, 255, 255));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        QRect textRect(static_cast<int>(cx - radius), static_cast<int>(cy - radius),
                       static_cast<int>(radius * 2), static_cast<int>(radius * 2));
        p.drawText(textRect, Qt::AlignCenter, QString::number(static_cast<int>(e.value)));
    }
}

void PaperBubbleMatrix2::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    int itemH = qMin(28, (rect.height() - 30) / kCategoryCount);

    for (int i = 0; i < kCategoryCount; ++i) {
        int y = rect.y() + 22 + i * (itemH + 4);
        int count = counts.contains(kCategories[i].key) ? counts[kCategories[i].key] : 0;

        // Color box
        p.setPen(Qt::NoPen);
        p.setBrush(kCategories[i].color);
        p.drawRoundedRect(rect.x() + 5, y + 4, 14, 14, 3, 3);

        // Label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18,
                   Qt::AlignVCenter, kCategories[i].label);

        // Count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(count) + " bubbles");
    }
}

void PaperBubbleMatrix2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Bubbles",  QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Avg Value",      QString::number(avgValue(), 'f', 1), QColor(245, 158, 11)},
        {"Highlighted",    QString::number(highlightCount()), QColor(239, 68, 68)},
        {"Categories",     QString::number(categoryCounts().size()), QColor(139, 92, 246)}
    };

    int boxH = qMin(42, (rect.height() - 10) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 22,
                   Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 26, rect.width() - 20, 14,
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperBubbleMatrix2::onRender() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    int nextId = entries_.isEmpty() ? 1 : entries_.last().id + 1;
    QStringList parts = text.split(',', Qt::SkipEmptyParts);

    QString rowName;
    QString colName;
    qreal value = 50.0;
    int size = 20;

    // Parse "Row,Column:value,size" or "Row,Column:value" or "Row,Column"
    if (parts.size() >= 1) {
        QString first = parts[0].trimmed();
        int colonPos = first.indexOf(':');
        if (colonPos > 0) {
            rowName = first.left(colonPos).trimmed();
            QString rest = first.mid(colonPos + 1).trimmed();
            // rest may be "value" or "value,size"
            QStringList valParts = rest.split(',', Qt::SkipEmptyParts);
            if (!valParts.isEmpty()) value = qBound(0.0, valParts[0].trimmed().toDouble(), 100.0);
            if (valParts.size() > 1) size = qBound(1, valParts[1].trimmed().toInt(), 50);
        } else {
            rowName = first;
        }
    }
    if (parts.size() >= 2) {
        colName = parts[1].trimmed();
        int colonPos = colName.indexOf(':');
        if (colonPos > 0) {
            QString after = colName.mid(colonPos + 1).trimmed();
            colName = colName.left(colonPos).trimmed();
            QStringList valParts = after.split(',', Qt::SkipEmptyParts);
            if (!valParts.isEmpty()) value = qBound(0.0, valParts[0].trimmed().toDouble(), 100.0);
            if (valParts.size() > 1) size = qBound(1, valParts[1].trimmed().toInt(), 50);
        }
    }
    if (parts.size() >= 3) {
        bool ok = false;
        qreal v = parts[2].trimmed().toDouble(&ok);
        if (ok) value = qBound(0.0, v, 100.0);
    }
    if (parts.size() >= 4) {
        bool ok = false;
        int s = parts[3].trimmed().toInt(&ok);
        if (ok) size = qBound(1, s, 50);
    }

    if (rowName.isEmpty()) rowName = "R" + QString::number(nextId);
    if (colName.isEmpty()) colName = "C" + QString::number(nextId);

    int comboIdx = categoryCombo_->currentIndex();
    QString category;
    if (comboIdx >= 1 && comboIdx <= kCategoryCount) {
        category = kCategories[comboIdx - 1].key;
    } else {
        category = kCategories[nextId % kCategoryCount].key;
    }

    BubbleMatrix2Entry entry;
    entry.id = nextId;
    entry.row = rowName;
    entry.category = category;
    entry.column = colName;
    entry.value = value;
    entry.size = size;
    entry.highlight = (value >= 75.0);
    entry.color = categoryColor(category);
    entries_.append(entry);

    saveSettings();
    updateInfo();
    emit bubbleClicked(entry.id, entry.value);
    update();
    inputField_->clear();
}

void PaperBubbleMatrix2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperBubbleMatrix2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Render a bubble matrix");
        return;
    }
    infoLabel_->setText(QString("%1 bubbles | %2 highlighted | avg %3")
        .arg(entries_.size())
        .arg(highlightCount())
        .arg(avgValue(), 0, 'f', 1));
}

void PaperBubbleMatrix2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        BubbleMatrix2Entry e;
        e.id = settings_.value("id").toInt();
        e.row = settings_.value("row").toString();
        e.category = settings_.value("category").toString();
        e.column = settings_.value("column").toString();
        e.value = settings_.value("value").toDouble();
        e.size = settings_.value("size").toInt();
        e.highlight = settings_.value("highlight").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperBubbleMatrix2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("row", entries_[i].row);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("column", entries_[i].column);
        settings_.setValue("value", entries_[i].value);
        settings_.setValue("size", entries_[i].size);
        settings_.setValue("highlight", entries_[i].highlight);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}

QList<BubbleMatrix2Entry> PaperBubbleMatrix2::entries() const {
    return entries_;
}

int PaperBubbleMatrix2::highlightCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.highlight) ++c;
    return c;
}

qreal PaperBubbleMatrix2::avgValue() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.value;
    return sum / entries_.size();
}

QMap<QString, int> PaperBubbleMatrix2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperBubbleMatrix2::addEntry(const BubbleMatrix2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit bubbleClicked(entry.id, entry.value);
    update();
}
