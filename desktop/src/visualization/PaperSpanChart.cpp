#include "visualization/PaperSpanChart.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperSpanChart::PaperSpanChart(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "SpanChart")
{
    setupUI();
    loadSettings();
}

void PaperSpanChart::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperSpanChart::onRender);
    toolbar->addWidget(renderBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Research", "Writing", "Review", "Editing", "Submission"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperSpanChart::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter span label...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Render span chart");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(620, 520);
}

void PaperSpanChart::addEntry(const SpanChartEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit spanSelected(entry.id, entry.duration);
    update();
}

QList<SpanChartEntry> PaperSpanChart::entries() const { return entries_; }

int PaperSpanChart::criticalCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.critical) c++;
    return c;
}

qreal PaperSpanChart::totalDuration() const {
    qreal sum = 0;
    for (const auto& e : entries_)
        sum += e.duration;
    return sum;
}

QMap<QString, int> PaperSpanChart::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperSpanChart::onRender() {
    QString text = inputField_->text().trimmed();

    QStringList categories = {"Research", "Writing", "Review", "Editing", "Submission"};
    QStringList phases = {"Planning", "Execution", "Finalization"};
    QColor palette[] = {
        QColor(0x3b, 0x82, 0xf6),  // #3b82f6
        QColor(0x16, 0xa3, 0x4a),  // #16a34a
        QColor(0xd9, 0x77, 0x06),  // #d97706
        QColor(0xdc, 0x26, 0x26),  // #dc2626
        QColor(0x7c, 0x3a, 0xed)   // #7c3aed
    };

    int filterIdx = categoryCombo_->currentIndex();

    entries_.clear();

    struct Seed { QString task; QString category; QString phase; qreal duration; int segments; bool critical; };
    Seed seeds[] = {
        {"Literature Review",   "Research",   "Planning",      12.0, 3, true},
        {"Data Collection",     "Research",   "Execution",     18.0, 4, true},
        {"Draft Writing",       "Writing",    "Execution",     24.0, 5, true},
        {"Internal Review",     "Review",     "Finalization",   7.0, 2, false},
        {"Revision Pass",       "Editing",    "Finalization",  10.0, 3, false},
        {"Co-author Feedback",  "Review",     "Execution",      8.0, 2, false},
        {"Final Proofreading",  "Editing",    "Finalization",   5.0, 1, false},
        {"Journal Submission",  "Submission", "Finalization",   3.0, 1, true}
    };

    for (int i = 0; i < 8; ++i) {
        const auto& s = seeds[i];
        if (filterIdx > 0 && s.category != categories[filterIdx - 1])
            continue;

        SpanChartEntry e;
        e.id = i + 1;
        e.task = text.isEmpty() ? s.task : text + " " + QString::number(i + 1);
        e.category = s.category;
        e.phase = s.phase;
        e.duration = s.duration;
        e.segments = s.segments;
        e.critical = s.critical;

        int catIdx = categories.indexOf(e.category);
        e.color = palette[qMax(0, catIdx)];

        entries_.append(e);
    }

    saveSettings();
    updateInfo();
    update();
}

void PaperSpanChart::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Render span chart");
    update();
}

void PaperSpanChart::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Render span chart");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Span Chart");

    int w = width(), h = height();
    drawSpanChart(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperSpanChart::drawSpanChart(QPainter& p, const QRect& rect) {
    int n = entries_.size();
    if (n == 0) return;

    // Find max cumulative end for the time axis
    qreal cumOffset = 0;
    QList<qreal> starts;
    QList<qreal> ends;
    qreal maxEnd = 0;
    for (int i = 0; i < n; ++i) {
        starts.append(cumOffset);
        ends.append(cumOffset + entries_[i].duration);
        if (ends[i] > maxEnd) maxEnd = ends[i];
        cumOffset += entries_[i].duration * 0.15; // slight overlap for visual grouping
    }
    if (maxEnd < 1.0) maxEnd = 1.0;

    int labelW = 90;
    int barAreaW = rect.width() - labelW - 30;
    int laneH = qMin(28, (rect.height() - 20) / qMax(n, 1));

    for (int i = 0; i < n; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + 5 + i * (laneH + 6);
        int barX = rect.x() + labelW;
        int barCenterY = y + laneH / 2;

        // Task label on left
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(rect.x(), y, labelW - 5, laneH, Qt::AlignRight | Qt::AlignVCenter,
                   e.task.left(14));

        // Compute span bar geometry
        qreal startX = static_cast<qreal>(starts[i]) / maxEnd;
        qreal endX = static_cast<qreal>(ends[i]) / maxEnd;
        int spanX = barX + static_cast<int>(startX * barAreaW);
        int spanW = static_cast<int>((endX - startX) * barAreaW);
        if (spanW < 6) spanW = 6;

        // Background track
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(241, 245, 249));
        p.drawRoundedRect(barX, y + 4, barAreaW, laneH - 8, 4, 4);

        // Segment divisions within the span bar
        QColor barColor = e.color;
        if (e.critical) {
            // Critical path: slightly brighter
            barColor = barColor.lighter(115);
        }

        int segW = spanW / qMax(e.segments, 1);
        for (int s = 0; s < e.segments; ++s) {
            int segStart = spanX + s * segW;
            int segEnd = (s == e.segments - 1) ? spanX + spanW : spanX + (s + 1) * segW;
            int sw = segEnd - segStart - 1;
            if (sw < 2) sw = 2;

            // Alternate segment opacity for visual texture
            QColor segColor = barColor;
            if (s % 2 == 1) segColor = segColor.darker(110);

            p.setBrush(segColor);
            p.drawRoundedRect(segStart, y + 4, sw, laneH - 8, 3, 3);
        }

        // Start marker (small triangle pointing right)
        p.setPen(Qt::NoPen);
        p.setBrush(Qt::white);
        QPolygon startMarker;
        startMarker << QPoint(spanX, barCenterY - 5)
                     << QPoint(spanX + 6, barCenterY)
                     << QPoint(spanX, barCenterY + 5);
        p.drawPolygon(startMarker);

        // End marker (small triangle pointing left)
        int endPosX = spanX + spanW;
        QPolygon endMarker;
        endMarker << QPoint(endPosX, barCenterY - 5)
                  << QPoint(endPosX - 6, barCenterY)
                  << QPoint(endPosX, barCenterY + 5);
        p.drawPolygon(endMarker);

        // Duration label above bar
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 7));
        p.drawText(spanX, y - 2, spanW, 14, Qt::AlignCenter,
                   QString::number(e.duration, 'f', 0) + "d");

        // Critical path highlight: red border
        if (e.critical) {
            p.setPen(QPen(QColor(220, 38, 38), 1.5, Qt::DashLine));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(spanX - 2, y + 2, spanW + 4, laneH - 4, 5, 5);

            // Small critical badge
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(220, 38, 38));
            p.drawEllipse(endPosX + 4, barCenterY - 5, 10, 10);
            p.setPen(Qt::white);
            p.setFont(QFont("Arial", 6, QFont::Bold));
            p.drawText(endPosX + 4, barCenterY - 5, 10, 10, Qt::AlignCenter, "!");
        }
    }
}

void PaperSpanChart::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList categories = {"Research", "Writing", "Review", "Editing", "Submission"};
    QColor palette[] = {
        QColor(0x3b, 0x82, 0xf6),
        QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06),
        QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int itemH = qMin(26, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (itemH + 4);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 130));

        // Color swatch
        p.setPen(Qt::NoPen);
        p.setBrush(palette[i]);
        p.drawRoundedRect(rect.x() + 5, y + 3, 14, 14, 3, 3);

        // Category label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y + 1, rect.width() / 2 - 24, 18, Qt::AlignVCenter,
                   categories[i]);

        // Count bar
        p.setPen(Qt::NoPen);
        p.setBrush(palette[i]);
        p.drawRoundedRect(rect.x() + rect.width() / 2, y + 2, barW, itemH - 4, 3, 3);

        // Count text
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2 + barW + 5, y + 2, 40, itemH - 4,
                   Qt::AlignVCenter | Qt::AlignLeft, QString::number(count));
    }
}

void PaperSpanChart::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries",    QString::number(entries_.size()),                   QColor(0x3b, 0x82, 0xf6)},
        {"Critical",   QString::number(criticalCount()),                  QColor(0xdc, 0x26, 0x26)},
        {"Total Dur",  QString::number(totalDuration(), 'f', 0) + "d",   QColor(0x16, 0xa3, 0x4a)},
        {"Categories", QString::number(categoryCounts().size()),          QColor(0x7c, 0x3a, 0xed)}
    };

    int boxH = qMin(42, (rect.height() - 10) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 22, Qt::AlignVCenter,
                   stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 26, rect.width() - 20, 14, Qt::AlignVCenter,
                   stats[i].label);
    }
}

void PaperSpanChart::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Render span chart");
        return;
    }
    infoLabel_->setText(QString("%1 entries | %2 critical | %3d total")
        .arg(entries_.size())
        .arg(criticalCount())
        .arg(totalDuration(), 0, 'f', 0));
}

void PaperSpanChart::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        SpanChartEntry e;
        e.id       = settings_.value("id").toInt();
        e.task     = settings_.value("task").toString();
        e.category = settings_.value("category").toString();
        e.phase    = settings_.value("phase").toString();
        e.duration = settings_.value("duration").toDouble();
        e.segments = settings_.value("segments").toInt();
        e.critical = settings_.value("critical").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperSpanChart::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",       entries_[i].id);
        settings_.setValue("task",     entries_[i].task);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("phase",    entries_[i].phase);
        settings_.setValue("duration", entries_[i].duration);
        settings_.setValue("segments", entries_[i].segments);
        settings_.setValue("critical", entries_[i].critical);
        settings_.setValue("color",    entries_[i].color.name());
    }
    settings_.endArray();
}
