#include "reading/PaperBookmarkVault.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainterPath>
#include <QRandomGenerator>

PaperBookmarkVault::PaperBookmarkVault(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "BookmarkVault")
{
    setupUI();
    loadSettings();
}

void PaperBookmarkVault::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Paper", "Article", "Book", "Website"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 5px 8px; border: 1px solid #cbd5e1; border-radius: 4px; "
        "background: white; min-width: 90px; }"
        "QComboBox::drop-down { border: none; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter title...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    saveBtn_ = new QPushButton("Save");
    saveBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 5px 14px; "
        "border-radius: 4px; border: none; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(saveBtn_, &QPushButton::clicked, this, &PaperBookmarkVault::onSave);
    toolbar->addWidget(saveBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("QPushButton { color: #dc2626; border: none; }"
                              "QPushButton:hover { color: #b91c1c; }");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperBookmarkVault::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Save bookmarks to your vault");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    layout->addStretch();
    setMinimumSize(640, 520);
}

void PaperBookmarkVault::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(250, 250, 252));

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 13));
        p.drawText(rect(), Qt::AlignCenter, "Save bookmarks to your vault");
        return;
    }

    int w = width(), h = height();
    int toolbarH = 80;
    int vaultH = (h - toolbarH) / 2;
    int bottomH = h - toolbarH - vaultH;
    int halfW = w / 2;

    drawVaultView(p, QRect(10, toolbarH, w - 20, vaultH - 10));
    drawCategoryChart(p, QRect(10, toolbarH + vaultH, halfW - 15, bottomH - 10));
    drawStats(p, QRect(halfW + 5, toolbarH + vaultH, halfW - 15, bottomH - 10));
}

void PaperBookmarkVault::drawVaultView(QPainter& p, const QRect& rect) {
    // Section header
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x() + 4, rect.y() + 18, "Bookmark Vault");

    int headerH = 28;
    int show = qMin(8, entries_.size());
    int cardGap = 4;
    int cardH = qMin(52, (rect.height() - headerH - 10) / qMax(show, 1) - cardGap);

    // Category color map
    QMap<QString, QColor> catColors;
    catColors["Paper"]    = QColor(59, 130, 246);   // #3b82f6
    catColors["Article"]  = QColor(22, 163, 74);     // #16a34a
    catColors["Book"]     = QColor(124, 58, 237);    // #7c3aed
    catColors["Website"]  = QColor(217, 119, 6);     // #d97706

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + headerH + 8 + i * (cardH + cardGap);

        // Card background
        QPainterPath cardPath;
        cardPath.addRoundedRect(QRectF(rect.x(), y, rect.width(), cardH), 6.0, 6.0);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 255, 255));
        p.drawPath(cardPath);

        // Category color strip on the left
        QColor stripColor = catColors.contains(e.category) ? catColors[e.category] : QColor(100, 116, 139);
        QPainterPath stripPath;
        stripPath.addRoundedRect(QRectF(rect.x(), y, 5, cardH), 2.5, 2.5);
        p.setBrush(stripColor);
        p.drawPath(stripPath);

        int textX = rect.x() + 12;

        // Pinned star
        if (e.pinned) {
            p.setPen(QColor(234, 179, 8));
            p.setFont(QFont("Arial", 11));
            p.drawText(textX, y, 16, cardH, Qt::AlignVCenter, QString::fromUtf8("\xe2\x98\x85"));
            textX += 16;
        }

        // Title
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        QString displayTitle = e.title.length() > 24 ? e.title.left(24) + "..." : e.title;
        p.drawText(textX, y + 3, rect.width() - textX + rect.x() - 120, 18,
                   Qt::AlignVCenter, displayTitle);

        // Location line
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        QString displayLoc = e.location.length() > 30 ? e.location.left(30) + "..." : e.location;
        p.drawText(textX, y + 20, rect.width() - textX + rect.x() - 120, 14,
                   Qt::AlignVCenter, displayLoc);

        // Relevance bar
        int barX = rect.x() + rect.width() - 110;
        int barY = y + 8;
        int barMaxW = 70;
        int barH2 = 7;

        // Bar background
        QPainterPath barBg;
        barBg.addRoundedRect(QRectF(barX, barY, barMaxW, barH2), 3.5, 3.5);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        p.drawPath(barBg);

        // Bar fill
        int fillW = static_cast<int>(e.relevance * barMaxW);
        if (fillW > 0) {
            QPainterPath barFill;
            barFill.addRoundedRect(QRectF(barX, barY, fillW, barH2), 3.5, 3.5);
            p.setBrush(stripColor);
            p.drawPath(barFill);
        }

        // Relevance text
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(barX + barMaxW + 4, barY + 7, QString::number(e.relevance, 'f', 2));

        // Visits count
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 7));
        p.drawText(barX, y + 30, barMaxW, 14, Qt::AlignVCenter,
                   QString("Visits: %1").arg(e.visits));
    }
}

void PaperBookmarkVault::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(rect.x() + 4, rect.y() + 18, "Category Breakdown");

    auto counts = categoryCounts();
    QStringList categories = {"Paper", "Article", "Book", "Website"};
    QColor colors[] = {
        QColor(59, 130, 246),   // Paper #3b82f6
        QColor(22, 163, 74),    // Article #16a34a
        QColor(124, 58, 237),   // Book #7c3aed
        QColor(217, 119, 6)     // Website #d97706
    };

    int total = 0;
    for (const auto& cat : categories) {
        total += counts.contains(cat) ? counts[cat] : 0;
    }

    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 30 + (rect.height() - 50) / 2;
    int outerR = qMin(rect.width(), rect.height() - 50) / 2 - 10;
    int innerR = outerR * 55 / 100;

    if (total == 0) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect, Qt::AlignCenter, "No data");
        return;
    }

    // Draw donut segments
    qreal startAngle = 0.0;
    for (int i = 0; i < 4; ++i) {
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        if (count == 0) continue;
        qreal sweep = 360.0 * count / total;

        QPainterPath slice;
        slice.moveTo(cx, cy);
        slice.arcTo(cx - outerR, cy - outerR, outerR * 2, outerR * 2,
                    startAngle * 16, sweep * 16);
        slice.arcTo(cx - innerR, cy - innerR, innerR * 2, innerR * 2,
                    (startAngle + sweep) * 16, -sweep * 16);
        slice.closeSubpath();

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawPath(slice);

        startAngle += sweep;
    }

    // Center hole
    QPainterPath hole;
    hole.addEllipse(QPointF(cx, cy), innerR, innerR);
    p.setBrush(QColor(250, 250, 252));
    p.drawPath(hole);

    // Center label
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 14, QFont::Bold));
    p.drawText(QRect(cx - innerR, cy - 10, innerR * 2, 20),
               Qt::AlignCenter, QString::number(total));
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 7));
    p.drawText(QRect(cx - innerR, cy + 6, innerR * 2, 14),
               Qt::AlignCenter, "total");

    // Legend below chart
    int legendY = cy + outerR + 14;
    int legendX = rect.x() + 10;
    int legendItemW = (rect.width() - 20) / 2;

    for (int i = 0; i < 4; ++i) {
        int row = i / 2;
        int col = i % 2;
        int lx = legendX + col * legendItemW;
        int ly = legendY + row * 16;

        QPainterPath dot;
        dot.addEllipse(QPointF(lx + 5, ly + 6), 4.0, 4.0);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawPath(dot);

        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 8));
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        p.drawText(lx + 14, ly + 10, QString("%1 (%2)").arg(categories[i]).arg(count));
    }
}

void PaperBookmarkVault::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(rect.x() + 4, rect.y() + 18, "Statistics");

    struct Stat {
        QString label;
        QString value;
        QColor color;
    };

    QList<Stat> stats = {
        {"Total Entries",  QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Avg Relevance",  QString::number(avgRelevance(), 'f', 2), QColor(22, 163, 74)},
        {"Pinned",         QString::number(pinnedCount()), QColor(124, 58, 237)},
    };

    int boxH = qMin(52, (rect.height() - 40) / 3);
    int boxGap = 8;

    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + 30 + i * (boxH + boxGap);

        QPainterPath boxPath;
        boxPath.addRoundedRect(QRectF(rect.x() + 4, y, rect.width() - 8, boxH), 6.0, 6.0);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawPath(boxPath);

        // Left accent bar
        QPainterPath accent;
        accent.addRoundedRect(QRectF(rect.x() + 4, y, 4, boxH), 2.0, 2.0);
        p.setBrush(stats[i].color);
        p.drawPath(accent);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 16, QFont::Bold));
        p.drawText(rect.x() + 16, y + 4, rect.width() - 28, 26,
                   Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 16, y + 30, rect.width() - 28, 16,
                   Qt::AlignVCenter, stats[i].label);
    }

    // Category breakdown mini-bars
    int miniY = rect.y() + 30 + stats.size() * (boxH + boxGap) + 4;
    if (miniY + 60 < rect.y() + rect.height()) {
        auto counts = categoryCounts();
        QStringList cats = {"Paper", "Article", "Book", "Website"};
        QColor catColors[] = {
            QColor(59, 130, 246), QColor(22, 163, 74),
            QColor(124, 58, 237), QColor(217, 119, 6)
        };
        int maxVal = 1;
        for (const auto& cat : cats) {
            if (counts.contains(cat)) maxVal = qMax(maxVal, counts[cat]);
        }

        int barH = qMin(12, (rect.y() + rect.height() - miniY - 10) / 4);
        for (int i = 0; i < 4; ++i) {
            int y2 = miniY + i * (barH + 4);
            if (y2 + barH > rect.y() + rect.height()) break;

            int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
            p.setPen(QColor(71, 85, 105));
            p.setFont(QFont("Arial", 7));
            p.drawText(rect.x() + 8, y2, 40, barH, Qt::AlignVCenter, cats[i]);

            int barX = rect.x() + 52;
            int barMaxW = rect.width() - 85;

            QPainterPath bgPath;
            bgPath.addRoundedRect(QRectF(barX, y2 + 2, barMaxW, barH - 4), 3.0, 3.0);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(226, 232, 240));
            p.drawPath(bgPath);

            int fillW = static_cast<int>((static_cast<qreal>(count) / maxVal) * barMaxW);
            if (fillW > 0) {
                QPainterPath fillPath;
                fillPath.addRoundedRect(QRectF(barX, y2 + 2, fillW, barH - 4), 3.0, 3.0);
                p.setBrush(catColors[i]);
                p.drawPath(fillPath);
            }

            p.setPen(QColor(100, 116, 139));
            p.setFont(QFont("Arial", 7));
            p.drawText(barX + barMaxW + 4, y2 + barH - 2, QString::number(count));
        }
    }
}

void PaperBookmarkVault::onSave() {
    QString title = inputField_->text().trimmed();
    if (title.isEmpty()) return;

    QMap<QString, QColor> catColors;
    catColors["Paper"]   = QColor(59, 130, 246);
    catColors["Article"] = QColor(22, 163, 74);
    catColors["Book"]    = QColor(124, 58, 237);
    catColors["Website"] = QColor(217, 119, 6);

    QStringList locations = {
        "arXiv:2301.00001", "DOI:10.1234/abc", "Nature Vol.583",
        "IEEE Xplore #4321", "JSTOR:2026.05.13", "ACM Digital Library",
        "Springer Link", "Wiley Online", "ScienceDirect",
        "Google Scholar Cache"
    };

    int cIdx = categoryCombo_->currentIndex();
    QString category;
    if (cIdx == 0) {
        QStringList cats = {"Paper", "Article", "Book", "Website"};
        category = cats[QRandomGenerator::global()->bounded(cats.size())];
    } else {
        category = categoryCombo_->currentText();
    }

    BookmarkVaultEntry entry;
    entry.id = entries_.isEmpty() ? 1 : entries_.last().id + 1;
    entry.title = title;
    entry.category = category;
    entry.location = locations[QRandomGenerator::global()->bounded(locations.size())];
    entry.relevance = QRandomGenerator::global()->bounded(100) / 100.0;
    entry.visits = 1 + QRandomGenerator::global()->bounded(200);
    entry.pinned = QRandomGenerator::global()->bounded(3) == 0;
    entry.color = catColors.contains(category) ? catColors[category] : QColor(100, 116, 139);

    addEntry(entry);
    inputField_->clear();
}

void PaperBookmarkVault::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperBookmarkVault::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Save bookmarks to your vault");
        return;
    }
    infoLabel_->setText(
        QString("Vault: %1 entries | Pinned: %2 | Avg Relevance: %3")
            .arg(entries_.size())
            .arg(pinnedCount())
            .arg(avgRelevance(), 0, 'f', 2));
}

void PaperBookmarkVault::loadSettings() {
    settings_.beginGroup("BookmarkVault");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        BookmarkVaultEntry e;
        e.id = settings_.value("id").toInt();
        e.title = settings_.value("title").toString();
        e.category = settings_.value("category").toString();
        e.location = settings_.value("location").toString();
        e.relevance = settings_.value("relevance").toDouble();
        e.visits = settings_.value("visits").toInt();
        e.pinned = settings_.value("pinned").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperBookmarkVault::saveSettings() {
    settings_.beginGroup("BookmarkVault");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("title", entries_[i].title);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("location", entries_[i].location);
        settings_.setValue("relevance", entries_[i].relevance);
        settings_.setValue("visits", entries_[i].visits);
        settings_.setValue("pinned", entries_[i].pinned);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}

void PaperBookmarkVault::addEntry(const BookmarkVaultEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit bookmarkAccessed(entry.id, entry.relevance);
    update();
}

QList<BookmarkVaultEntry> PaperBookmarkVault::entries() const {
    return entries_;
}

int PaperBookmarkVault::pinnedCount() const {
    int count = 0;
    for (const auto& e : entries_)
        if (e.pinned) ++count;
    return count;
}

qreal PaperBookmarkVault::avgRelevance() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.relevance;
    return sum / entries_.size();
}

QMap<QString, int> PaperBookmarkVault::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}
