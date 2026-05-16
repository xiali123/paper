#include "citation/PaperCitationBinder.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainterPath>
#include <QRandomGenerator>
#include <QtMath>

PaperCitationBinder::PaperCitationBinder(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CitationBinder")
    , bindBtn_(nullptr)
    , clearBtn_(nullptr)
    , categoryCombo_(nullptr)
    , inputField_(nullptr)
    , infoLabel_(nullptr)
{
    setupUI();
    loadSettings();
}

void PaperCitationBinder::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    // Toolbar row
    auto* toolbar = new QHBoxLayout();
    toolbar->setSpacing(6);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Direct", "Indirect", "Self", "Cross-ref"});
    categoryCombo_->setMinimumWidth(110);
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; "
        "background: white; }"
        "QComboBox::drop-down { border: none; }"
    );
    connect(categoryCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperCitationBinder::updateInfo);
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Source->Target");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }"
    );
    toolbar->addWidget(inputField_, 1);

    bindBtn_ = new QPushButton("Bind");
    bindBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 16px; "
        "border-radius: 4px; font-weight: 600; }"
        "QPushButton:hover { background: #2563eb; }"
        "QPushButton:pressed { background: #1d4ed8; }"
    );
    connect(bindBtn_, &QPushButton::clicked, this, &PaperCitationBinder::onBind);
    toolbar->addWidget(bindBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet(
        "QPushButton { background: #dc2626; color: white; padding: 4px 16px; "
        "border-radius: 4px; font-weight: 600; }"
        "QPushButton:hover { background: #b91c1c; }"
        "QPushButton:pressed { background: #991b1b; }"
    );
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCitationBinder::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    // Info label
    infoLabel_ = new QLabel("Binds: 0 | Verified: 0 | Avg Confidence: 0.0%");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(780, 560);
}

void PaperCitationBinder::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int toolbarH = 80;
    int w = width() - 16;
    int h = height() - toolbarH;

    // Bind view: top half
    int bindH = h * 55 / 100;
    drawBindView(p, QRect(8, toolbarH, w, bindH));

    // Bottom section: pie chart left, stats right
    int bottomY = toolbarH + bindH + 6;
    int bottomH = h - bindH - 6;
    int halfW = w / 2 - 3;

    drawCategoryChart(p, QRect(8, bottomY, halfW, bottomH));
    drawStats(p, QRect(8 + halfW + 6, bottomY, halfW, bottomH));
}

void PaperCitationBinder::drawBindView(QPainter& p, const QRect& rect) {
    // Background
    QPainterPath bgPath;
    bgPath.addRoundedRect(rect, 8, 8);
    p.fillPath(bgPath, QColor("#f8fafc"));
    p.strokePath(bgPath, QPen(QColor("#e2e8f0"), 1));

    // Title
    p.setPen(QColor("#1e293b"));
    QFont titleFont = p.font();
    titleFont.setPointSize(11);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(12, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Citation Binds");
    p.setFont(QFont());

    QString filter = categoryCombo_->currentText();
    QList<CitationBindEntry> visible;
    for (const auto& e : entries_) {
        if (filter == "All" || e.category == filter) {
            visible.append(e);
        }
    }

    if (visible.isEmpty()) {
        p.setPen(QColor("#94a3b8"));
        QFont hintFont;
        hintFont.setPointSize(10);
        p.setFont(hintFont);
        p.drawText(rect, Qt::AlignCenter, "No citation binds yet. Use Source->Target to create one.");
        p.setFont(QFont());
        return;
    }

    int rowH = 52;
    int startY = rect.y() + 32;
    int maxRows = (rect.height() - 40) / rowH;
    int count = qMin(visible.size(), maxRows);

    QFont normalFont;
    normalFont.setPointSize(9);
    p.setFont(normalFont);

    for (int i = 0; i < count; ++i) {
        const auto& entry = visible[i];
        int y = startY + i * rowH;
        QRect rowRect(rect.x() + 8, y, rect.width() - 16, rowH - 4);

        // Row background
        QPainterPath rowPath;
        rowPath.addRoundedRect(rowRect, 6, 6);
        QColor rowBg = entry.color.lighter(190);
        rowBg.setAlpha(60);
        p.fillPath(rowPath, rowBg);

        // Source label
        QRect srcRect(rowRect.x() + 10, y + 6, rowRect.width() * 3 / 10 - 20, 20);
        p.setPen(QColor("#1e293b"));
        QFont srcFont;
        srcFont.setPointSize(9);
        srcFont.setBold(true);
        p.setFont(srcFont);
        p.drawText(srcRect, Qt::AlignLeft | Qt::AlignVCenter,
                   entry.source.length() > 20 ? entry.source.left(18) + "..." : entry.source);

        // Arrow
        int arrowX = rowRect.x() + rowRect.width() * 3 / 10;
        int arrowY = y + 16;
        p.setPen(QPen(entry.color, 2));
        p.drawLine(arrowX, arrowY, arrowX + 40, arrowY);

        // Arrowhead
        QPolygonF arrowHead;
        arrowHead << QPointF(arrowX + 40, arrowY)
                  << QPointF(arrowX + 34, arrowY - 5)
                  << QPointF(arrowX + 34, arrowY + 5);
        p.setBrush(entry.color);
        p.setPen(Qt::NoPen);
        p.drawPolygon(arrowHead);
        p.setBrush(Qt::NoBrush);

        // Target label
        QRect tgtRect(arrowX + 46, y + 6, rowRect.width() * 3 / 10 - 50, 20);
        p.setPen(QColor("#1e293b"));
        p.setFont(srcFont);
        p.drawText(tgtRect, Qt::AlignLeft | Qt::AlignVCenter,
                   entry.target.length() > 20 ? entry.target.left(18) + "..." : entry.target);
        p.setFont(normalFont);

        // Confidence bar
        int barX = rowRect.x() + rowRect.width() * 7 / 10;
        int barW = rowRect.width() * 15 / 100;
        int barY = y + 10;
        int barH = 12;

        QPainterPath barBg;
        barBg.addRoundedRect(QRect(barX, barY, barW, barH), 3, 3);
        p.fillPath(barBg, QColor("#e2e8f0"));

        int fillW = static_cast<int>(barW * qBound(0.0, entry.confidence, 1.0));
        if (fillW > 0) {
            QPainterPath barFill;
            barFill.addRoundedRect(QRect(barX, barY, fillW, barH), 3, 3);
            QColor barColor = entry.confidence >= 0.8 ? QColor("#16a34a") :
                              entry.confidence >= 0.5 ? QColor("#d97706") : QColor("#dc2626");
            p.fillPath(barFill, barColor);
        }

        // Confidence text
        p.setPen(QColor("#64748b"));
        p.drawText(QRect(barX + barW + 4, y + 6, 40, 20), Qt::AlignLeft | Qt::AlignVCenter,
                   QString::number(entry.confidence * 100, 'f', 0) + "%");

        // References count
        int refX = rowRect.x() + rowRect.width() * 87 / 100;
        p.setPen(QColor("#475569"));
        p.drawText(QRect(refX, y + 4, 40, 14), Qt::AlignLeft | Qt::AlignTop,
                   QString::number(entry.references) + " refs");

        // Verified badge
        if (entry.verified) {
            int badgeX = refX + 38;
            QPainterPath badge;
            badge.addRoundedRect(QRect(badgeX, y + 6, 8, 8), 4, 4);
            p.fillPath(badge, QColor("#16a34a"));
            p.setPen(QColor("#15803d"));
            QFont smallFont;
            smallFont.setPointSize(7);
            p.setFont(smallFont);
            p.drawText(QRect(badgeX + 10, y + 6, 24, 12), Qt::AlignLeft | Qt::AlignVCenter, "OK");
            p.setFont(normalFont);
        }
    }
}

void PaperCitationBinder::drawCategoryChart(QPainter& p, const QRect& rect) {
    QPainterPath bgPath;
    bgPath.addRoundedRect(rect, 8, 8);
    p.fillPath(bgPath, QColor("#f8fafc"));
    p.strokePath(bgPath, QPen(QColor("#e2e8f0"), 1));

    // Title
    p.setPen(QColor("#1e293b"));
    QFont titleFont = p.font();
    titleFont.setPointSize(11);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(12, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Category Distribution");
    p.setFont(QFont());

    QMap<QString, int> counts = categoryCounts();
    if (counts.isEmpty()) {
        p.setPen(QColor("#94a3b8"));
        QFont hintFont;
        hintFont.setPointSize(9);
        p.setFont(hintFont);
        p.drawText(rect, Qt::AlignCenter, "No data");
        p.setFont(QFont());
        return;
    }

    // Colors for categories
    QMap<QString, QColor> catColors;
    catColors["Direct"] = QColor("#3b82f6");
    catColors["Indirect"] = QColor("#16a34a");
    catColors["Self"] = QColor("#7c3aed");
    catColors["Cross-ref"] = QColor("#d97706");

    int total = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        total += it.value();
    }

    int chartSize = qMin(rect.width() - 24, rect.height() - 60);
    chartSize = qMax(chartSize, 60);
    int cx = rect.x() + (rect.width() - chartSize) / 2;
    int cy = rect.y() + 36 + (rect.height() - 50 - chartSize) / 2;
    QRectF chartRect(cx, cy, chartSize, chartSize);

    qreal startAngle = 0.0;
    QFont labelFont;
    labelFont.setPointSize(8);
    p.setFont(labelFont);

    int legendY = cy + chartSize + 8;
    int legendX = rect.x() + 12;

    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        qreal span = 360.0 * it.value() / total;
        QColor col = catColors.value(it.key(), QColor("#64748b"));

        // Draw pie slice
        QPainterPath slice;
        slice.moveTo(chartRect.center());
        slice.arcTo(chartRect, startAngle, span);
        slice.closeSubpath();
        p.fillPath(slice, col);
        p.strokePath(slice, QPen(QColor("#ffffff"), 2));

        // Draw percentage if slice is large enough
        if (span > 20) {
            qreal midAngle = qDegreesToRadians(startAngle + span / 2.0);
            qreal labelR = chartSize * 0.3;
            qreal lx = chartRect.center().x() + labelR * qCos(midAngle);
            qreal ly = chartRect.center().y() - labelR * qSin(midAngle);
            p.setPen(Qt::white);
            QFont pctFont;
            pctFont.setPointSize(8);
            pctFont.setBold(true);
            p.setFont(pctFont);
            QString pct = QString::number(it.value() * 100.0 / total, 'f', 0) + "%";
            QRectF pctRect(lx - 20, ly - 8, 40, 16);
            p.drawText(pctRect, Qt::AlignCenter, pct);
            p.setFont(labelFont);
        }

        // Legend entry
        p.setPen(Qt::NoPen);
        QPainterPath dot;
        dot.addEllipse(QRectF(legendX, legendY, 8, 8));
        p.fillPath(dot, col);
        p.setPen(QColor("#334155"));
        p.drawText(QRect(legendX + 12, legendY - 2, 100, 14), Qt::AlignLeft | Qt::AlignVCenter,
                   it.key() + " (" + QString::number(it.value()) + ")");

        legendX += 100;
        if (legendX + 100 > rect.right()) {
            legendX = rect.x() + 12;
            legendY += 16;
        }

        startAngle += span;
    }
}

void PaperCitationBinder::drawStats(QPainter& p, const QRect& rect) {
    QPainterPath bgPath;
    bgPath.addRoundedRect(rect, 8, 8);
    p.fillPath(bgPath, QColor("#f8fafc"));
    p.strokePath(bgPath, QPen(QColor("#e2e8f0"), 1));

    // Title
    p.setPen(QColor("#1e293b"));
    QFont titleFont = p.font();
    titleFont.setPointSize(11);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(12, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Statistics");
    p.setFont(QFont());

    QFont statFont;
    statFont.setPointSize(10);
    QFont valFont;
    valFont.setPointSize(14);
    valFont.setBold(true);

    int x = rect.x() + 16;
    int y = rect.y() + 36;
    int lineH = 44;
    int valW = rect.width() - 32;

    // Total entries
    p.setPen(QColor("#64748b"));
    p.setFont(statFont);
    p.drawText(QRect(x, y, valW, 18), Qt::AlignLeft, "Total Binds");
    p.setPen(QColor("#1e293b"));
    p.setFont(valFont);
    p.drawText(QRect(x, y + 16, valW, 24), Qt::AlignLeft, QString::number(entries_.size()));
    y += lineH;

    // Verified count
    int verified = verifiedCount();
    p.setPen(QColor("#64748b"));
    p.setFont(statFont);
    p.drawText(QRect(x, y, valW, 18), Qt::AlignLeft, "Verified");
    p.setPen(verified > 0 ? QColor("#16a34a") : QColor("#94a3b8"));
    p.setFont(valFont);
    p.drawText(QRect(x, y + 16, valW, 24), Qt::AlignLeft, QString::number(verified));
    y += lineH;

    // Average confidence
    qreal avg = avgConfidence();
    p.setPen(QColor("#64748b"));
    p.setFont(statFont);
    p.drawText(QRect(x, y, valW, 18), Qt::AlignLeft, "Avg Confidence");
    QColor confColor = avg >= 0.8 ? QColor("#16a34a") : avg >= 0.5 ? QColor("#d97706") : QColor("#dc2626");
    p.setPen(confColor);
    p.setFont(valFont);
    p.drawText(QRect(x, y + 16, valW, 24), Qt::AlignLeft,
               QString::number(avg * 100, 'f', 1) + "%");
    y += lineH;

    // Category breakdown mini bars
    QMap<QString, int> counts = categoryCounts();
    QMap<QString, QColor> catColors;
    catColors["Direct"] = QColor("#3b82f6");
    catColors["Indirect"] = QColor("#16a34a");
    catColors["Self"] = QColor("#7c3aed");
    catColors["Cross-ref"] = QColor("#d97706");

    int maxCount = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        maxCount = qMax(maxCount, it.value());
    }

    p.setFont(statFont);
    for (auto it = counts.constBegin(); it != counts.constEnd() && y + 22 < rect.bottom() - 8; ++it) {
        QColor col = catColors.value(it.key(), QColor("#64748b"));
        int barMaxW = valW - 80;

        // Label
        p.setPen(QColor("#475569"));
        p.drawText(QRect(x, y, 70, 16), Qt::AlignLeft | Qt::AlignVCenter, it.key());

        // Bar background
        int barX = x + 74;
        QPainterPath barBg;
        barBg.addRoundedRect(QRect(barX, y + 3, barMaxW, 10), 3, 3);
        p.fillPath(barBg, QColor("#e2e8f0"));

        // Bar fill
        int fillW = maxCount > 0 ? barMaxW * it.value() / maxCount : 0;
        if (fillW > 0) {
            QPainterPath barFill;
            barFill.addRoundedRect(QRect(barX, y + 3, fillW, 10), 3, 3);
            p.fillPath(barFill, col);
        }

        y += 20;
    }
}

void PaperCitationBinder::onBind() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QString source;
    QString target;
    int sep = text.indexOf("->");
    if (sep > 0) {
        source = text.left(sep).trimmed();
        target = text.mid(sep + 2).trimmed();
    } else {
        // Try other separators
        sep = text.indexOf("->");
        if (sep < 0) sep = text.indexOf(">");
        if (sep < 0) sep = text.indexOf(":");
        if (sep > 0) {
            source = text.left(sep).trimmed();
            target = text.mid(sep + 1).trimmed();
        } else {
            source = text;
            target = "Unknown";
        }
    }

    if (source.isEmpty()) source = "Unknown";
    if (target.isEmpty()) target = "Unknown";

    static int nextId = 1;
    QString category = categoryCombo_->currentText();
    if (category == "All") category = "Direct";

    QMap<QString, QColor> catColors;
    catColors["Direct"] = QColor("#3b82f6");
    catColors["Indirect"] = QColor("#16a34a");
    catColors["Self"] = QColor("#7c3aed");
    catColors["Cross-ref"] = QColor("#d97706");

    CitationBindEntry entry;
    entry.id = nextId++;
    entry.source = source;
    entry.target = target;
    entry.category = category;
    entry.confidence = 0.5 + QRandomGenerator::global()->bounded(50) / 100.0;
    entry.references = QRandomGenerator::global()->bounded(1, 30);
    entry.verified = entry.confidence >= 0.75;
    entry.color = catColors.value(category, QColor("#64748b"));

    entries_.append(entry);
    inputField_->clear();
    updateInfo();
    saveSettings();
    emit citationBound(entry.id, entry.confidence);
    update();
}

void PaperCitationBinder::onClear() {
    entries_.clear();
    inputField_->clear();
    categoryCombo_->setCurrentIndex(0);
    updateInfo();
    saveSettings();
    update();
}

void PaperCitationBinder::updateInfo() {
    int total = entries_.size();
    int verified = verifiedCount();
    qreal avg = avgConfidence();

    // Apply filter count
    QString filter = categoryCombo_->currentText();
    int filtered = 0;
    for (const auto& e : entries_) {
        if (filter == "All" || e.category == filter) {
            ++filtered;
        }
    }

    infoLabel_->setText(
        QString("Binds: %1 (showing %2) | Verified: %3 | Avg Confidence: %4%")
            .arg(total)
            .arg(filtered)
            .arg(verified)
            .arg(QString::number(avg * 100, 'f', 1))
    );
}

void PaperCitationBinder::loadSettings() {
    int size = settings_.beginReadArray("bindEntries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        CitationBindEntry entry;
        entry.id = settings_.value("id").toInt();
        entry.source = settings_.value("source").toString();
        entry.target = settings_.value("target").toString();
        entry.category = settings_.value("category").toString();
        entry.confidence = settings_.value("confidence").toDouble();
        entry.references = settings_.value("references").toInt();
        entry.verified = settings_.value("verified").toBool();
        entry.color = QColor(settings_.value("color").toString());

        // Re-derive color from category if saved color is invalid
        QMap<QString, QColor> catColors;
        catColors["Direct"] = QColor("#3b82f6");
        catColors["Indirect"] = QColor("#16a34a");
        catColors["Self"] = QColor("#7c3aed");
        catColors["Cross-ref"] = QColor("#d97706");
        if (!entry.color.isValid()) {
            entry.color = catColors.value(entry.category, QColor("#64748b"));
        }

        entries_.append(entry);
    }
    settings_.endArray();
    updateInfo();
    update();
}

void PaperCitationBinder::saveSettings() {
    settings_.beginWriteArray("bindEntries", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& e = entries_[i];
        settings_.setValue("id", e.id);
        settings_.setValue("source", e.source);
        settings_.setValue("target", e.target);
        settings_.setValue("category", e.category);
        settings_.setValue("confidence", e.confidence);
        settings_.setValue("references", e.references);
        settings_.setValue("verified", e.verified);
        settings_.setValue("color", e.color.name());
    }
    settings_.endArray();
}

QList<CitationBindEntry> PaperCitationBinder::entries() const {
    return entries_;
}

int PaperCitationBinder::verifiedCount() const {
    int count = 0;
    for (const auto& e : entries_) {
        if (e.verified) ++count;
    }
    return count;
}

qreal PaperCitationBinder::avgConfidence() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) {
        sum += e.confidence;
    }
    return sum / entries_.size();
}

QMap<QString, int> PaperCitationBinder::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) {
        counts[e.category]++;
    }
    return counts;
}

void PaperCitationBinder::addEntry(const CitationBindEntry& entry) {
    entries_.append(entry);
    updateInfo();
    saveSettings();
    update();
}
