#include "reading/PaperHighlightCollector.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>

PaperHighlightCollector::PaperHighlightCollector(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "HighlightCollector")
{
    setupUI();
    loadSettings();
}

void PaperHighlightCollector::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Yellow", "Green", "Blue", "Pink"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 6px 10px; border: 1px solid #cbd5e1; border-radius: 4px; "
        "background: white; min-width: 90px; }"
        "QComboBox::drop-down { border: none; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter highlight text...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px 10px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    collectBtn_ = new QPushButton("Collect");
    collectBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 6px 16px; "
        "border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(collectBtn_, &QPushButton::clicked, this, &PaperHighlightCollector::onCollect);
    toolbar->addWidget(collectBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet(
        "QPushButton { background: #f1f5f9; color: #dc2626; padding: 6px 14px; "
        "border-radius: 4px; border: 1px solid #e2e8f0; }"
        "QPushButton:hover { background: #fee2e2; }");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperHighlightCollector::onClear);
    toolbar->addWidget(clearBtn_);

    mainLayout->addLayout(toolbar);

    infoLabel_ = new QLabel("No highlights collected");
    infoLabel_->setStyleSheet("font-size: 12px; color: #64748b; padding: 4px 2px;");
    mainLayout->addWidget(infoLabel_);

    mainLayout->addStretch(1);
    setMinimumSize(700, 520);
}

void PaperHighlightCollector::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Collect highlights to visualize");
        return;
    }

    int w = width(), h = height();

    int toolbarH = 80;
    int topH = (h - toolbarH) * 55 / 100;
    int bottomH = h - toolbarH - topH;

    QRect topRect(10, toolbarH, w - 20, topH);
    drawCollectionView(p, topRect);

    int bottomLeftW = (w - 30) / 2;
    QRect bottomLeft(10, toolbarH + topH, bottomLeftW, bottomH);
    drawCategoryChart(p, bottomLeft);

    QRect bottomRight(10 + bottomLeftW + 10, toolbarH + topH, w - 30 - bottomLeftW, bottomH);
    drawStats(p, bottomRight);
}

void PaperHighlightCollector::drawCollectionView(QPainter& p, const QRect& rect) {
    // Section title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 18, "Highlight Collection");

    QColor colorMap[] = {
        QColor("#3b82f6"), // Yellow slot -> blue
        QColor("#facc15"), // Yellow
        QColor("#16a34a"), // Green
        QColor("#7c3aed"), // Blue
        QColor("#d97706")  // Pink
    };

    int maxShow = qMin(8, entries_.size());
    int cardH = qMin(52, (rect.height() - 30) / qMax(maxShow, 1));
    int cardW = rect.width() - 20;
    int startX = rect.x() + 10;
    int startY = rect.y() + 28;

    for (int i = 0; i < maxShow; ++i) {
        const auto& e = entries_[i];
        int y = startY + i * (cardH + 4);

        // Card background
        QPainterPath cardPath;
        cardPath.addRoundedRect(QRect(startX, y, cardW, cardH), 6, 6);
        p.fillPath(cardPath, QColor("#f8fafc"));
        p.setPen(QPen(QColor("#e2e8f0"), 1));
        p.drawPath(cardPath);

        // Colored strip on the left edge
        QPainterPath stripPath;
        stripPath.addRoundedRect(QRect(startX, y, 5, cardH), 2, 2);
        p.fillPath(stripPath, e.highlightColor);

        // Text snippet (truncated)
        p.setPen(QColor(30, 41, 59));
        p.setFont(QFont("Arial", 10));
        QString snippet = e.text.length() > 60 ? e.text.left(57) + "..." : e.text;
        p.drawText(startX + 14, y + cardH / 2 - 6, snippet);

        // Relevance bar (bottom portion of card)
        int barX = startX + 14;
        int barY = y + cardH - 14;
        int barMaxW = cardW / 3;
        int barFilledW = static_cast<int>(barMaxW * qBound(0.0, e.relevance, 1.0));
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#e2e8f0"));
        p.drawRoundedRect(QRect(barX, barY, barMaxW, 6), 3, 3);
        p.setBrush(e.highlightColor);
        p.drawRoundedRect(QRect(barX, barY, barFilledW, 6), 3, 3);

        // Relevance label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(barX + barMaxW + 6, barY + 6,
                   QString("rel: %1%").arg(static_cast<int>(e.relevance * 100)));

        // Annotations count badge
        int annX = startX + cardW - 120;
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(annX, y + cardH / 2,
                   QString::number(e.annotations) + " notes");

        // Starred indicator
        if (e.starred) {
            int starX = startX + cardW - 30;
            QPainterPath star;
            int cx = starX + 8, cy = y + cardH / 2 - 2;
            for (int j = 0; j < 5; ++j) {
                qreal angle = -M_PI / 2 + j * 2 * M_PI / 5;
                qreal outerR = 8;
                qreal innerR = 3.5;
                star.moveTo(cx + outerR * std::cos(angle), cy + outerR * std::sin(angle));
                qreal innerAngle = angle + M_PI / 5;
                star.lineTo(cx + innerR * std::cos(innerAngle), cy + innerR * std::sin(innerAngle));
            }
            star.closeSubpath();
            p.setPen(Qt::NoPen);
            p.fillPath(star, QColor("#f59e0b"));
        }
    }
}

void PaperHighlightCollector::drawCategoryChart(QPainter& p, const QRect& rect) {
    // Section title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x() + 10, rect.y() + 18, "Categories");

    QMap<QString, int> counts = categoryCounts();
    if (counts.isEmpty()) return;

    QList<QColor> sliceColors = {
        QColor("#facc15"), QColor("#16a34a"), QColor("#7c3aed"), QColor("#d97706")
    };
    QStringList catNames = {"Yellow", "Green", "Blue", "Pink"};

    int total = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        total += it.value();
    if (total == 0) return;

    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + rect.height() / 2 + 10;
    int outerR = qMin(rect.width(), rect.height()) / 2 - 30;
    int innerR = outerR * 55 / 100;

    qreal startAngle = 0.0;

    // Draw slices for each known category
    for (int i = 0; i < catNames.size(); ++i) {
        int count = counts.value(catNames[i], 0);
        if (count == 0) continue;
        qreal span = 360.0 * count / total;

        QPainterPath slice;
        slice.moveTo(cx + innerR * std::cos(qDegreesToRadians(startAngle)),
                     cy - innerR * std::sin(qDegreesToRadians(startAngle)));
        slice.arcTo(cx - outerR, cy - outerR, 2 * outerR, 2 * outerR, startAngle, span);
        qreal endAngle = startAngle + span;
        slice.lineTo(cx + innerR * std::cos(qDegreesToRadians(endAngle)),
                     cy - innerR * std::sin(qDegreesToRadians(endAngle)));
        slice.arcTo(cx - innerR, cy - innerR, 2 * innerR, 2 * innerR, endAngle, -span);
        slice.closeSubpath();

        p.setPen(Qt::NoPen);
        p.fillPath(slice, sliceColors[i]);

        // Label outside the donut
        qreal midAngle = startAngle + span / 2;
        int labelR = outerR + 16;
        int lx = cx + static_cast<int>(labelR * std::cos(qDegreesToRadians(midAngle)));
        int ly = cy - static_cast<int>(labelR * std::sin(qDegreesToRadians(midAngle)));
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 9));
        p.drawText(lx - 20, ly + 4, catNames[i]);

        startAngle += span;
    }

    // Draw any extra categories not in the standard four
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        if (catNames.contains(it.key())) continue;
        qreal span = 360.0 * it.value() / total;

        QPainterPath slice;
        slice.moveTo(cx + innerR * std::cos(qDegreesToRadians(startAngle)),
                     cy - innerR * std::sin(qDegreesToRadians(startAngle)));
        slice.arcTo(cx - outerR, cy - outerR, 2 * outerR, 2 * outerR, startAngle, span);
        qreal endAngle = startAngle + span;
        slice.lineTo(cx + innerR * std::cos(qDegreesToRadians(endAngle)),
                     cy - innerR * std::sin(qDegreesToRadians(endAngle)));
        slice.arcTo(cx - innerR, cy - innerR, 2 * innerR, 2 * innerR, endAngle, -span);
        slice.closeSubpath();

        p.setPen(Qt::NoPen);
        p.fillPath(slice, QColor("#94a3b8"));

        qreal midAngle = startAngle + span / 2;
        int labelR = outerR + 16;
        int lx = cx + static_cast<int>(labelR * std::cos(qDegreesToRadians(midAngle)));
        int ly = cy - static_cast<int>(labelR * std::sin(qDegreesToRadians(midAngle)));
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 9));
        p.drawText(lx - 20, ly + 4, it.key());

        startAngle += span;
    }

    // Center text showing total
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 16, QFont::Bold));
    p.drawText(QRect(cx - 30, cy - 14, 60, 28), Qt::AlignCenter, QString::number(total));
}

void PaperHighlightCollector::drawStats(QPainter& p, const QRect& rect) {
    // Section title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x() + 10, rect.y() + 18, "Statistics");

    int x = rect.x() + 20;
    int y = rect.y() + 40;
    int lineH = 28;

    // Total entries
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(x, y, "Total Highlights:");
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(x + 140, y, QString::number(entries_.size()));
    y += lineH;

    // Starred count
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(x, y, "Starred:");
    p.setPen(QColor("#f59e0b"));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(x + 140, y, QString::number(starredCount()));
    y += lineH;

    // Average relevance
    qreal avg = avgRelevance();
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(x, y, "Avg Relevance:");
    p.setPen(QColor("#3b82f6"));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(x + 140, y, QString::number(avg, 'f', 2));
    y += lineH;

    // Relevance bar visual
    int barX = x;
    int barW = rect.width() - 50;
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#e2e8f0"));
    p.drawRoundedRect(QRect(barX, y, barW, 8), 4, 4);
    int fillW = static_cast<int>(barW * qBound(0.0, avg, 1.0));
    p.setBrush(QColor("#3b82f6"));
    p.drawRoundedRect(QRect(barX, y, fillW, 8), 4, 4);
    y += lineH;

    // Category breakdown
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(x, y, "Category Breakdown:");
    y += lineH - 4;

    QMap<QString, int> counts = categoryCounts();
    QColor catColors[] = {QColor("#facc15"), QColor("#16a34a"), QColor("#7c3aed"), QColor("#d97706")};
    QStringList catOrder = {"Yellow", "Green", "Blue", "Pink"};
    for (int i = 0; i < catOrder.size(); ++i) {
        int count = counts.value(catOrder[i], 0);
        // Color dot
        p.setPen(Qt::NoPen);
        p.setBrush(catColors[i]);
        p.drawEllipse(QPoint(x + 6, y - 4), 5, 5);
        // Label
        p.setPen(QColor(51, 65, 85));
        p.setFont(QFont("Arial", 9));
        p.drawText(x + 16, y, catOrder[i] + ": " + QString::number(count));
        y += 20;
    }
}

void PaperHighlightCollector::onCollect() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    int catIdx = categoryCombo_->currentIndex();

    QColor highlightColor;
    QString category;
    QString color;

    struct CatInfo { QString name; QColor color; QString hex; };
    static const CatInfo categories[] = {
        {"Yellow", QColor("#facc15"), "#facc15"},
        {"Green",  QColor("#16a34a"), "#16a34a"},
        {"Blue",   QColor("#7c3aed"), "#7c3aed"},
        {"Pink",   QColor("#d97706"), "#d97706"}
    };

    if (catIdx == 0) {
        // "All" -> random category
        int r = QRandomGenerator::global()->bounded(4);
        category = categories[r].name;
        highlightColor = categories[r].color;
        color = categories[r].hex;
    } else {
        int idx = catIdx - 1;
        category = categories[idx].name;
        highlightColor = categories[idx].color;
        color = categories[idx].hex;
    }

    HighlightEntry entry;
    entry.id = entries_.isEmpty() ? 1 : entries_.last().id + 1;
    entry.text = text;
    entry.category = category;
    entry.color = color;
    entry.relevance = QRandomGenerator::global()->bounded(100) / 100.0;
    entry.annotations = QRandomGenerator::global()->bounded(5);
    entry.starred = QRandomGenerator::global()->bounded(3) == 0;
    entry.highlightColor = highlightColor;

    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit highlightCollected(entry.id, entry.relevance);
    inputField_->clear();
    update();
}

void PaperHighlightCollector::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperHighlightCollector::updateInfo() {
    int total = entries_.size();
    int starred = starredCount();
    qreal avg = avgRelevance();
    infoLabel_->setText(
        QString("Highlights: %1  |  Starred: %2  |  Avg Relevance: %3")
            .arg(total)
            .arg(starred)
            .arg(avg, 0, 'f', 2));
}

QList<HighlightEntry> PaperHighlightCollector::entries() const {
    return entries_;
}

int PaperHighlightCollector::starredCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.starred) c++;
    return c;
}

qreal PaperHighlightCollector::avgRelevance() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_)
        sum += e.relevance;
    return sum / entries_.size();
}

QMap<QString, int> PaperHighlightCollector::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperHighlightCollector::loadSettings() {
    int size = settings_.beginReadArray("highlights");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        HighlightEntry e;
        e.id = settings_.value("id", i + 1).toInt();
        e.text = settings_.value("text").toString();
        e.category = settings_.value("category", "Yellow").toString();
        e.color = settings_.value("color", "#facc15").toString();
        e.relevance = settings_.value("relevance", 0.5).toReal();
        e.annotations = settings_.value("annotations", 0).toInt();
        e.starred = settings_.value("starred", false).toBool();
        e.highlightColor = QColor(e.color);
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperHighlightCollector::saveSettings() {
    settings_.beginWriteArray("highlights");
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setArrayIndex(i);
        settings_.setValue("id", e.id);
        settings_.setValue("text", e.text);
        settings_.setValue("category", e.category);
        settings_.setValue("color", e.color);
        settings_.setValue("relevance", e.relevance);
        settings_.setValue("annotations", e.annotations);
        settings_.setValue("starred", e.starred);
    }
    settings_.endArray();
}
