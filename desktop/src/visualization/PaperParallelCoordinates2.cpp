#include "visualization/PaperParallelCoordinates2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>

PaperParallelCoordinates2::PaperParallelCoordinates2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ParallelCoordinates2")
{
    setupUI();
    loadSettings();
}

void PaperParallelCoordinates2::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperParallelCoordinates2::onRender);
    toolbar->addWidget(renderBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Citation", "Impact", "Quality", "Novelty", "Relevance"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperParallelCoordinates2::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter sample label...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Render parallel coordinates");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(700, 520);
}

void PaperParallelCoordinates2::addEntry(const ParallelCoordinates2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit sampleSelected(entry.id, entry.value);
    update();
}

QList<ParallelCoordinates2Entry> PaperParallelCoordinates2::entries() const {
    return entries_;
}

int PaperParallelCoordinates2::selectedCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.selected) c++;
    return c;
}

qreal PaperParallelCoordinates2::avgValue() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_)
        sum += e.value;
    return sum / entries_.size();
}

QMap<QString, int> PaperParallelCoordinates2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperParallelCoordinates2::onRender() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    static const QStringList categories = {"Citation", "Impact", "Quality", "Novelty", "Relevance"};
    static const QList<QColor> palette = {
        QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06), QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };
    int cIdx = categoryCombo_->currentIndex();
    int seedCount = 8;
    for (int i = 0; i < seedCount; ++i) {
        ParallelCoordinates2Entry e;
        e.id = entries_.size() + 1;
        e.sample = text.left(6) + "_s" + QString::number(i);
        e.category = cIdx == 0 ? categories[i % categories.size()] : categories[cIdx - 1];
        e.dimension = categories[i % categories.size()];
        e.value = QRandomGenerator::global()->bounded(100) / 100.0;
        e.axes = 4 + QRandomGenerator::global()->bounded(3);
        e.selected = (i % 3 == 0);
        int ci = categories.indexOf(e.category);
        e.color = palette[ci >= 0 ? ci : 0];
        entries_.append(e);
    }
    saveSettings();
    updateInfo();
    if (!entries_.isEmpty())
        emit sampleSelected(entries_.last().id, entries_.last().value);
    update();
    inputField_->clear();
}

void PaperParallelCoordinates2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Render parallel coordinates");
    update();
}

void PaperParallelCoordinates2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Render parallel coordinates");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Parallel Coordinates 2");
    int w = width(), h = height();
    drawParallelCoords(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperParallelCoordinates2::drawParallelCoords(QPainter& p, const QRect& rect) {
    static const QStringList axisLabels = {"Citation", "Impact", "Quality", "Novelty", "Relevance"};
    int axisCount = axisLabels.size();
    int axisSpacing = rect.width() / (axisCount + 1);

    // Group entries by sample to draw polylines
    QMap<QString, QList<int>> sampleIndices;
    for (int i = 0; i < entries_.size(); ++i)
        sampleIndices[entries_[i].sample].append(i);

    // Draw axes
    for (int a = 0; a < axisCount; ++a) {
        int x = rect.x() + (a + 1) * axisSpacing;
        p.setPen(QPen(QColor(203, 213, 225), 1));
        p.drawLine(x, rect.y() + 20, x, rect.y() + rect.height() - 10);
        // Axis label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(x - 25, rect.y() + 4, 50, 14, Qt::AlignCenter, axisLabels[a]);
        // Tick marks
        for (int t = 0; t <= 4; ++t) {
            qreal frac = t / 4.0;
            int y = rect.y() + rect.height() - 10 - static_cast<int>(frac * (rect.height() - 35));
            p.setPen(QPen(QColor(226, 232, 240), 1));
            p.drawLine(x - 3, y, x + 3, y);
            p.setPen(QColor(148, 163, 184));
            p.setFont(QFont("Arial", 6));
            p.drawText(x + 5, y - 4, 24, 10, Qt::AlignVCenter, QString::number(frac, 'f', 1));
        }
    }

    // Draw connecting polylines per sample
    for (auto it = sampleIndices.constBegin(); it != sampleIndices.constEnd(); ++it) {
        const auto& indices = it.value();
        if (indices.isEmpty()) continue;
        const auto& first = entries_[indices.first()];
        QColor lineColor = first.color;

        // Build dimension -> value map for this sample
        QMap<QString, qreal> dimValues;
        for (int idx : indices)
            dimValues[entries_[idx].dimension] = entries_[idx].value;
        // Fill missing dims with the entry's value as fallback
        for (const auto& dim : axisLabels) {
            if (!dimValues.contains(dim))
                dimValues[dim] = first.value;
        }

        QPainterPath path;
        bool started = false;
        for (int a = 0; a < axisCount; ++a) {
            int x = rect.x() + (a + 1) * axisSpacing;
            qreal val = dimValues.value(axisLabels[a], first.value);
            int y = rect.y() + rect.height() - 10 - static_cast<int>(val * (rect.height() - 35));
            if (!started) {
                path.moveTo(x, y);
                started = true;
            } else {
                path.lineTo(x, y);
            }
        }
        p.setPen(QPen(lineColor, first.selected ? 2.5 : 1.2,
                       first.selected ? Qt::SolidLine : Qt::DashLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(Qt::NoBrush);
        p.drawPath(path);

        // Draw dots at each axis intersection
        for (int a = 0; a < axisCount; ++a) {
            int x = rect.x() + (a + 1) * axisSpacing;
            qreal val = dimValues.value(axisLabels[a], first.value);
            int y = rect.y() + rect.height() - 10 - static_cast<int>(val * (rect.height() - 35));
            p.setPen(Qt::NoPen);
            p.setBrush(lineColor);
            p.drawEllipse(QPoint(x, y), first.selected ? 4 : 3, first.selected ? 4 : 3);
        }
    }
}

void PaperParallelCoordinates2::drawCategoryLegend(QPainter& p, const QRect& rect) {
    static const QStringList categories = {"Citation", "Impact", "Quality", "Novelty", "Relevance"};
    static const QList<QColor> palette = {
        QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06), QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };
    auto counts = categoryCounts();
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    int itemH = qMin(26, (rect.height() - 30) / categories.size());
    for (int i = 0; i < categories.size(); ++i) {
        int y = rect.y() + 22 + i * (itemH + 4);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        p.setPen(Qt::NoPen);
        p.setBrush(palette[i]);
        p.drawRoundedRect(rect.x() + 5, y + 4, 14, 14, 3, 3);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18, Qt::AlignVCenter, categories[i]);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(count) + " entries");
    }
}

void PaperParallelCoordinates2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries",   QString::number(entries_.size()),       QColor(0x3b, 0x82, 0xf6)},
        {"Selected",  QString::number(selectedCount()),       QColor(0x16, 0xa3, 0x4a)},
        {"Avg Value", QString::number(avgValue(), 'f', 2),    QColor(0xd9, 0x77, 0x06)},
        {"Categories",QString::number(categoryCounts().size()),QColor(0x7c, 0x3a, 0xed)}
    };
    int boxH = qMin(42, (rect.height() - 10) / stats.size());
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

void PaperParallelCoordinates2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Render parallel coordinates");
        return;
    }
    infoLabel_->setText(QString("%1 entries | %2 selected | avg %3")
        .arg(entries_.size())
        .arg(selectedCount())
        .arg(avgValue(), 0, 'f', 2));
}

void PaperParallelCoordinates2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ParallelCoordinates2Entry e;
        e.id       = settings_.value("id").toInt();
        e.sample   = settings_.value("sample").toString();
        e.category = settings_.value("category").toString();
        e.dimension= settings_.value("dimension").toString();
        e.value    = settings_.value("value").toDouble();
        e.axes     = settings_.value("axes").toInt();
        e.selected = settings_.value("selected").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperParallelCoordinates2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& e = entries_[i];
        settings_.setValue("id",       e.id);
        settings_.setValue("sample",   e.sample);
        settings_.setValue("category", e.category);
        settings_.setValue("dimension",e.dimension);
        settings_.setValue("value",    e.value);
        settings_.setValue("axes",     e.axes);
        settings_.setValue("selected", e.selected);
        settings_.setValue("color",    e.color.name());
    }
    settings_.endArray();
}
