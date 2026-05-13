#include "reading/PaperHighlightCollector2.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>
#include <QtMath>

PaperHighlightCollector2::PaperHighlightCollector2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "HighlightCollector2")
{
    setupUI();
    loadSettings();
}

void PaperHighlightCollector2::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    auto* toolbar = new QHBoxLayout();
    toolbar->setSpacing(8);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Method", "Result", "Discussion", "Introduction", "Conclusion"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 6px 10px; border: 1px solid #cbd5e1; border-radius: 4px; "
        "background: white; min-width: 110px; font-size: 12px; }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView { selection-background-color: #3b82f6; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter highlight text...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px 10px; border: 1px solid #cbd5e1; border-radius: 4px; "
        "font-size: 12px; }");
    toolbar->addWidget(inputField_, 1);

    collectBtn_ = new QPushButton("Collect");
    collectBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 6px 16px; "
        "border-radius: 4px; font-weight: bold; font-size: 12px; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(collectBtn_, &QPushButton::clicked, this, &PaperHighlightCollector2::onCollect);
    toolbar->addWidget(collectBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet(
        "QPushButton { background: #f1f5f9; color: #dc2626; padding: 6px 14px; "
        "border-radius: 4px; border: 1px solid #e2e8f0; font-size: 12px; }"
        "QPushButton:hover { background: #fee2e2; }");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperHighlightCollector2::onClear);
    toolbar->addWidget(clearBtn_);

    mainLayout->addLayout(toolbar);

    infoLabel_ = new QLabel("No highlights collected");
    infoLabel_->setStyleSheet("font-size: 11px; color: #64748b; padding: 2px 2px;");
    mainLayout->addWidget(infoLabel_);

    mainLayout->addStretch(1);
    setMinimumSize(780, 580);
}

void PaperHighlightCollector2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor("#fafbfc"));

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 13));
        p.drawText(rect(), Qt::AlignCenter, "Collect highlights to visualize");
        return;
    }

    int w = width(), h = height();
    int toolbarH = 76;

    int topH = (h - toolbarH) * 58 / 100;
    int bottomH = h - toolbarH - topH;

    QRect topRect(6, toolbarH, w - 12, topH);
    drawCollectorView(p, topRect);

    int bottomLeftW = (w - 22) * 45 / 100;
    QRect bottomLeft(6, toolbarH + topH + 4, bottomLeftW, bottomH - 4);
    drawCategoryChart(p, bottomLeft);

    QRect bottomRight(6 + bottomLeftW + 10, toolbarH + topH + 4,
                      w - 22 - bottomLeftW, bottomH - 4);
    drawStats(p, bottomRight);
}

void PaperHighlightCollector2::drawCollectorView(QPainter& p, const QRect& rect) {
    // Section background
    QPainterPath sectionBg;
    sectionBg.addRoundedRect(rect, 8, 8);
    p.fillPath(sectionBg, QColor("#ffffff"));
    p.setPen(QPen(QColor("#e2e8f0"), 1));
    p.drawPath(sectionBg);

    // Section title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x() + 14, rect.y() + 22, "Highlight Collection");

    // Color legend for palette
    struct ColorDef { QString name; QColor color; };
    static const ColorDef palette[] = {
        {"Yellow", QColor("#eab308")},
        {"Green",  QColor("#16a34a")},
        {"Blue",   QColor("#3b82f6")},
        {"Pink",   QColor("#ec4899")},
        {"Orange", QColor("#d97706")}
    };

    int maxShow = qMin(8, entries_.size());
    int cardH = qMin(48, (rect.height() - 40) / qMax(maxShow, 1));
    int cardW = rect.width() - 28;
    int startX = rect.x() + 14;
    int startY = rect.y() + 34;

    for (int i = 0; i < maxShow; ++i) {
        const auto& e = entries_[i];
        int y = startY + i * (cardH + 3);

        // Card background
        QPainterPath cardPath;
        cardPath.addRoundedRect(QRect(startX, y, cardW, cardH), 5, 5);
        p.fillPath(cardPath, QColor("#f8fafc"));
        p.setPen(QPen(QColor("#e2e8f0"), 1));
        p.drawPath(cardPath);

        // Color strip on left edge
        QPainterPath stripPath;
        stripPath.addRoundedRect(QRect(startX, y + 2, 4, cardH - 4), 2, 2);
        p.fillPath(stripPath, e.entryColor);
        p.setPen(Qt::NoPen);
        p.drawRect(QRect(startX + 2, y + 2, 2, cardH - 4));

        // Text preview (truncated)
        p.setPen(QColor(30, 41, 59));
        p.setFont(QFont("Arial", 10));
        QString snippet = e.text.length() > 55 ? e.text.left(52) + "..." : e.text;
        p.drawText(startX + 14, y + cardH / 2 - 4, snippet);

        // Relevance bar (bottom of card)
        int barX = startX + 14;
        int barY = y + cardH - 12;
        int barMaxW = cardW / 4;
        int barFilledW = static_cast<int>(barMaxW * qBound(0.0, e.relevance, 1.0));

        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#e2e8f0"));
        p.drawRoundedRect(QRect(barX, barY, barMaxW, 5), 2, 2);

        // Gradient bar fill based on entry color
        QLinearGradient barGrad(barX, barY, barX + barFilledW, barY);
        barGrad.setColorAt(0.0, e.entryColor.lighter(120));
        barGrad.setColorAt(1.0, e.entryColor);
        p.setBrush(barGrad);
        p.drawRoundedRect(QRect(barX, barY, barFilledW, 5), 2, 2);

        // Relevance percentage label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(barX + barMaxW + 6, barY + 5,
                   QString::number(static_cast<int>(e.relevance * 100)) + "%");

        // Highlight count badge
        int badgeX = startX + cardW - 130;
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(badgeX, y + cardH / 2 - 2,
                   QString::number(e.highlights) + " highlights");

        // Category tag
        int tagX = startX + cardW - 200;
        QRect tagRect(tagX, y + cardH / 2 - 12, 62, 18);
        QPainterPath tagPath;
        tagPath.addRoundedRect(tagRect, 9, 9);
        p.fillPath(tagPath, e.entryColor.lighter(160));
        p.setPen(e.entryColor.darker(110));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(tagRect, Qt::AlignCenter, e.category);

        // Starred icon (five-pointed star)
        if (e.starred) {
            int starX = startX + cardW - 28;
            int cx = starX, cy = y + cardH / 2 - 2;
            QPainterPath star;
            for (int j = 0; j < 5; ++j) {
                qreal outerAngle = -M_PI / 2 + j * 2 * M_PI / 5;
                qreal innerAngle = outerAngle + M_PI / 5;
                qreal outerR = 7.0;
                qreal innerR = 3.0;
                if (j == 0)
                    star.moveTo(cx + outerR * std::cos(outerAngle),
                                cy + outerR * std::sin(outerAngle));
                else
                    star.lineTo(cx + outerR * std::cos(outerAngle),
                                cy + outerR * std::sin(outerAngle));
                star.lineTo(cx + innerR * std::cos(innerAngle),
                            cy + innerR * std::sin(innerAngle));
            }
            star.closeSubpath();
            p.setPen(Qt::NoPen);
            p.fillPath(star, QColor("#f59e0b"));
        }
    }

    // Show count hint if more entries exist
    if (entries_.size() > maxShow) {
        p.setPen(QColor(148, 163, 184));
        p.setFont(QFont("Arial", 9));
        p.drawText(startX + 14, startY + maxShow * (cardH + 3) + 4,
                   QString("+ %1 more entries").arg(entries_.size() - maxShow));
    }
}

void PaperHighlightCollector2::drawCategoryChart(QPainter& p, const QRect& rect) {
    // Section background
    QPainterPath sectionBg;
    sectionBg.addRoundedRect(rect, 8, 8);
    p.fillPath(sectionBg, QColor("#ffffff"));
    p.setPen(QPen(QColor("#e2e8f0"), 1));
    p.drawPath(sectionBg);

    // Section title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 12, QFont::Bold));
    p.drawText(rect.x() + 12, rect.y() + 20, "Categories");

    QMap<QString, int> counts = categoryCounts();
    if (counts.isEmpty()) return;

    // Palette and category mapping
    struct SliceInfo { QString name; QColor color; };
    static const SliceInfo slices[] = {
        {"Method",       QColor("#3b82f6")},
        {"Result",       QColor("#16a34a")},
        {"Discussion",   QColor("#d97706")},
        {"Introduction", QColor("#dc2626")},
        {"Conclusion",   QColor("#7c3aed")}
    };
    static const int sliceCount = 5;

    int total = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        total += it.value();
    if (total == 0) return;

    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + rect.height() / 2 + 8;
    int outerR = qMin(rect.width(), rect.height()) / 2 - 34;
    int innerR = outerR * 55 / 100;

    if (outerR < 20) return;

    qreal startAngle = 90.0;

    for (int i = 0; i < sliceCount; ++i) {
        int count = counts.value(slices[i].name, 0);
        if (count == 0) continue;
        qreal span = 360.0 * count / total;

        // Build donut slice path
        QPainterPath slicePath;
        qreal saRad = qDegreesToRadians(startAngle);
        qreal eaRad = qDegreesToRadians(startAngle + span);

        slicePath.arcMoveTo(cx - outerR, cy - outerR, 2 * outerR, 2 * outerR, startAngle);
        slicePath.arcTo(cx - outerR, cy - outerR, 2 * outerR, 2 * outerR, startAngle, span);
        slicePath.arcTo(cx - innerR, cy - innerR, 2 * innerR, 2 * innerR,
                        startAngle + span, -span);
        slicePath.closeSubpath();

        p.setPen(QPen(QColor("#ffffff"), 2));
        p.fillPath(slicePath, slices[i].color);

        // Label with count
        qreal midAngle = qDegreesToRadians(startAngle + span / 2);
        int labelR = outerR + 14;
        int lx = cx + static_cast<int>(labelR * std::cos(midAngle));
        int ly = cy - static_cast<int>(labelR * std::sin(midAngle));
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 8));
        p.drawText(lx - 24, ly + 4, slices[i].name + " (" + QString::number(count) + ")");

        startAngle += span;
    }

    // Handle extra categories not in standard five
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        bool known = false;
        for (int i = 0; i < sliceCount; ++i) {
            if (it.key() == slices[i].name) { known = true; break; }
        }
        if (known) continue;

        qreal span = 360.0 * it.value() / total;
        QPainterPath slicePath;
        slicePath.arcMoveTo(cx - outerR, cy - outerR, 2 * outerR, 2 * outerR, startAngle);
        slicePath.arcTo(cx - outerR, cy - outerR, 2 * outerR, 2 * outerR, startAngle, span);
        slicePath.arcTo(cx - innerR, cy - innerR, 2 * innerR, 2 * innerR,
                        startAngle + span, -span);
        slicePath.closeSubpath();

        p.setPen(QPen(QColor("#ffffff"), 2));
        p.fillPath(slicePath, QColor("#94a3b8"));

        qreal midAngle = qDegreesToRadians(startAngle + span / 2);
        int labelR = outerR + 14;
        int lx = cx + static_cast<int>(labelR * std::cos(midAngle));
        int ly = cy - static_cast<int>(labelR * std::sin(midAngle));
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 8));
        p.drawText(lx - 24, ly + 4, it.key() + " (" + QString::number(it.value()) + ")");

        startAngle += span;
    }

    // Center text
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 18, QFont::Bold));
    p.drawText(QRect(cx - 30, cy - 12, 60, 24), Qt::AlignCenter, QString::number(total));
    p.setFont(QFont("Arial", 8));
    p.setPen(QColor(100, 116, 139));
    p.drawText(QRect(cx - 30, cy + 10, 60, 14), Qt::AlignCenter, "total");
}

void PaperHighlightCollector2::drawStats(QPainter& p, const QRect& rect) {
    // Section background
    QPainterPath sectionBg;
    sectionBg.addRoundedRect(rect, 8, 8);
    p.fillPath(sectionBg, QColor("#ffffff"));
    p.setPen(QPen(QColor("#e2e8f0"), 1));
    p.drawPath(sectionBg);

    // Section title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 12, QFont::Bold));
    p.drawText(rect.x() + 12, rect.y() + 20, "Statistics");

    int total = entries_.size();
    int starred = starredCount();
    qreal avg = avgRelevance();
    int totalHighlights = 0;
    for (const auto& e : entries_)
        totalHighlights += e.highlights;

    // Four stat boxes in 2x2 grid
    struct StatBox { QString label; QString value; QColor accent; };
    StatBox boxes[] = {
        {"Total Entries",    QString::number(total),          QColor("#3b82f6")},
        {"Starred",          QString::number(starred),        QColor("#f59e0b")},
        {"Avg Relevance",    QString::number(avg, 'f', 2),    QColor("#16a34a")},
        {"Total Highlights", QString::number(totalHighlights), QColor("#7c3aed")}
    };

    int boxPad = 10;
    int boxGap = 8;
    int boxW = (rect.width() - boxPad * 2 - boxGap) / 2;
    int boxH = qMin(60, (rect.height() - 56 - boxGap) / 2);
    int baseX = rect.x() + boxPad;
    int baseY = rect.y() + 30;

    for (int i = 0; i < 4; ++i) {
        int col = i % 2;
        int row = i / 2;
        int bx = baseX + col * (boxW + boxGap);
        int by = baseY + row * (boxH + boxGap);

        // Box background
        QPainterPath boxPath;
        boxPath.addRoundedRect(QRect(bx, by, boxW, boxH), 6, 6);
        p.fillPath(boxPath, QColor("#f8fafc"));
        p.setPen(QPen(boxes[i].accent.lighter(140), 1));
        p.drawPath(boxPath);

        // Accent strip on top
        QPainterPath topStrip;
        topStrip.addRoundedRect(QRect(bx, by, boxW, 3), 6, 6);
        p.fillPath(topStrip, boxes[i].accent);

        // Value
        p.setPen(boxes[i].accent);
        p.setFont(QFont("Arial", 16, QFont::Bold));
        p.drawText(QRect(bx, by + 10, boxW, 28), Qt::AlignCenter, boxes[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(QRect(bx, by + boxH - 18, boxW, 14), Qt::AlignCenter, boxes[i].label);
    }

    // Overall relevance bar below boxes
    int barY = baseY + 2 * (boxH + boxGap) + 8;
    int barW = rect.width() - boxPad * 2;
    if (barY + 20 < rect.y() + rect.height()) {
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(baseX, barY, "Overall Relevance");
        barY += 14;

        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#e2e8f0"));
        p.drawRoundedRect(QRect(baseX, barY, barW, 8), 4, 4);

        int fillW = static_cast<int>(barW * qBound(0.0, avg, 1.0));
        QLinearGradient grad(baseX, barY, baseX + fillW, barY);
        grad.setColorAt(0.0, QColor("#3b82f6"));
        grad.setColorAt(1.0, QColor("#7c3aed"));
        p.setBrush(grad);
        p.drawRoundedRect(QRect(baseX, barY, fillW, 8), 4, 4);
    }
}

void PaperHighlightCollector2::onCollect() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    struct CatDef { QString name; QString color; QColor qcolor; };
    static const CatDef cats[] = {
        {"Method",       "Yellow", QColor("#eab308")},
        {"Result",       "Green",  QColor("#16a34a")},
        {"Discussion",   "Blue",   QColor("#3b82f6")},
        {"Introduction", "Pink",   QColor("#ec4899")},
        {"Conclusion",   "Orange", QColor("#d97706")}
    };
    static const int catCount = 5;

    int idx = categoryCombo_->currentIndex();
    QString category, color;
    QColor entryColor;

    if (idx == 0) {
        int r = QRandomGenerator::global()->bounded(catCount);
        category = cats[r].name;
        color = cats[r].color;
        entryColor = cats[r].qcolor;
    } else if (idx >= 1 && idx <= catCount) {
        category = cats[idx - 1].name;
        color = cats[idx - 1].color;
        entryColor = cats[idx - 1].qcolor;
    } else {
        category = cats[0].name;
        color = cats[0].color;
        entryColor = cats[0].qcolor;
    }

    HighlightCollector2Entry entry;
    entry.id = entries_.isEmpty() ? 1 : entries_.last().id + 1;
    entry.text = text;
    entry.category = category;
    entry.color = color;
    entry.relevance = QRandomGenerator::global()->bounded(100) / 100.0;
    entry.highlights = QRandomGenerator::global()->bounded(1, 20);
    entry.starred = QRandomGenerator::global()->bounded(4) == 0;
    entry.entryColor = entryColor;

    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit highlightCollected(entry.id, entry.relevance);
    inputField_->clear();
    update();
}

void PaperHighlightCollector2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperHighlightCollector2::updateInfo() {
    int total = entries_.size();
    int starred = starredCount();
    qreal avg = avgRelevance();
    infoLabel_->setText(
        QString("Highlights: %1  |  Starred: %2  |  Avg Relevance: %3")
            .arg(total)
            .arg(starred)
            .arg(avg, 0, 'f', 2));
}

void PaperHighlightCollector2::addEntry(const HighlightCollector2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<HighlightCollector2Entry> PaperHighlightCollector2::entries() const {
    return entries_;
}

int PaperHighlightCollector2::starredCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.starred) c++;
    return c;
}

qreal PaperHighlightCollector2::avgRelevance() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_)
        sum += e.relevance;
    return sum / entries_.size();
}

QMap<QString, int> PaperHighlightCollector2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperHighlightCollector2::loadSettings() {
    int size = settings_.beginReadArray("highlights2");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        HighlightCollector2Entry e;
        e.id = settings_.value("id", i + 1).toInt();
        e.text = settings_.value("text").toString();
        e.category = settings_.value("category", "Method").toString();
        e.color = settings_.value("color", "Yellow").toString();
        e.relevance = settings_.value("relevance", 0.5).toReal();
        e.highlights = settings_.value("highlights", 0).toInt();
        e.starred = settings_.value("starred", false).toBool();
        e.entryColor = QColor(settings_.value("entryColor", "#eab308").toString());
        entries_.append(e);
    }
    settings_.endArray();

    // Seed 8 entries if empty
    if (entries_.isEmpty()) {
        struct SeedDef {
            QString text; QString category; QString color; QColor qcolor;
            qreal relevance; int highlights; bool starred;
        };
        static const SeedDef seeds[] = {
            {"Novel transformer architecture reduces attention complexity from O(n^2) to O(n log n)",
             "Method",       "Yellow", QColor("#eab308"), 0.92, 15, true},
            {"The proposed model achieves 94.7% accuracy on GLUE benchmark, surpassing previous SOTA",
             "Result",       "Green",  QColor("#16a34a"), 0.88, 12, true},
            {"These findings suggest a paradigm shift in how we approach sequence modeling tasks",
             "Discussion",   "Blue",   QColor("#3b82f6"), 0.76, 8,  false},
            {"Self-attention mechanisms have become foundational in modern NLP systems since 2017",
             "Introduction", "Pink",   QColor("#ec4899"), 0.65, 5,  false},
            {"Future work should explore scaling laws and cross-modal extensions of this approach",
             "Conclusion",   "Orange", QColor("#d97706"), 0.71, 6,  false},
            {"Multi-head attention with 8 parallel heads provides optimal capacity-efficiency tradeoff",
             "Method",       "Yellow", QColor("#eab308"), 0.85, 11, true},
            {"Latency reduced by 42% while maintaining comparable BLEU scores across 4 languages",
             "Result",       "Green",  QColor("#16a34a"), 0.79, 9,  false},
            {"Limitations include dataset bias and narrow domain scope requiring broader validation",
             "Discussion",   "Blue",   QColor("#3b82f6"), 0.60, 4,  false}
        };

        for (int i = 0; i < 8; ++i) {
            const auto& s = seeds[i];
            HighlightCollector2Entry e;
            e.id = i + 1;
            e.text = s.text;
            e.category = s.category;
            e.color = s.color;
            e.relevance = s.relevance;
            e.highlights = s.highlights;
            e.starred = s.starred;
            e.entryColor = s.qcolor;
            entries_.append(e);
        }
        saveSettings();
    }

    updateInfo();
}

void PaperHighlightCollector2::saveSettings() {
    settings_.beginWriteArray("highlights2");
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setArrayIndex(i);
        settings_.setValue("id", e.id);
        settings_.setValue("text", e.text);
        settings_.setValue("category", e.category);
        settings_.setValue("color", e.color);
        settings_.setValue("relevance", e.relevance);
        settings_.setValue("highlights", e.highlights);
        settings_.setValue("starred", e.starred);
        settings_.setValue("entryColor", e.entryColor.name());
    }
    settings_.endArray();
}
