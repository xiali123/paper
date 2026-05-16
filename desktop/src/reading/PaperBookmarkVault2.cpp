#include "reading/PaperBookmarkVault2.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainterPath>
#include <QRandomGenerator>
#include <numeric>

PaperBookmarkVault2::PaperBookmarkVault2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "BookmarkVault2")
{
    setupUI();
    loadSettings();
}

void PaperBookmarkVault2::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "AI", "Physics", "Biology", "Chemistry", "Math"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 5px 8px; border: 1px solid #cbd5e1; border-radius: 4px; "
        "background: white; min-width: 90px; }"
        "QComboBox::drop-down { border: none; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter bookmark title...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    addBtn_ = new QPushButton("Add");
    addBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 5px 14px; "
        "border-radius: 4px; border: none; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperBookmarkVault2::onAdd);
    toolbar->addWidget(addBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet(
        "QPushButton { color: #dc2626; border: none; }"
        "QPushButton:hover { color: #b91c1c; }");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperBookmarkVault2::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Add bookmarks to your vault");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    layout->addStretch();
    setMinimumSize(640, 520);
}

void PaperBookmarkVault2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(250, 250, 252));

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 13));
        p.drawText(rect(), Qt::AlignCenter, "Add bookmarks to your vault");
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

void PaperBookmarkVault2::drawVaultView(QPainter& p, const QRect& rect) {
    // Section header
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x() + 4, rect.y() + 18, "Bookmark Vault");

    int headerH = 28;
    int show = qMin(8, entries_.size());
    int cardGap = 4;
    int cardH = qMin(52, (rect.height() - headerH - 10) / qMax(show, 1) - cardGap);

    // Folder color map
    QMap<QString, QColor> folderColors;
    folderColors["Reading List"] = QColor(59, 130, 246);   // #3b82f6
    folderColors["To Read"]     = QColor(217, 119, 6);     // #d97706
    folderColors["Favorites"]   = QColor(22, 163, 74);     // #16a34a
    folderColors["Archive"]     = QColor(124, 58, 237);    // #7c3aed

    // Category color map
    QMap<QString, QColor> catColors;
    catColors["AI"]        = QColor(59, 130, 246);   // #3b82f6
    catColors["Physics"]   = QColor(22, 163, 74);    // #16a34a
    catColors["Biology"]   = QColor(217, 119, 6);    // #d97706
    catColors["Chemistry"] = QColor(220, 38, 38);    // #dc2626
    catColors["Math"]      = QColor(124, 58, 237);   // #7c3aed

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
        QColor stripColor = catColors.contains(e.category)
            ? catColors[e.category] : QColor(100, 116, 139);
        QPainterPath stripPath;
        stripPath.addRoundedRect(QRectF(rect.x(), y, 5, cardH), 2.5, 2.5);
        p.setBrush(stripColor);
        p.drawPath(stripPath);

        int textX = rect.x() + 12;

        // Pinned icon
        if (e.pinned) {
            p.setPen(QColor(234, 179, 8));
            p.setFont(QFont("Arial", 11));
            p.drawText(textX, y, 16, cardH, Qt::AlignVCenter,
                       QString::fromUtf8("\xe2\x98\x85"));
            textX += 16;
        }

        // Title
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        QString displayTitle = e.title.length() > 22
            ? e.title.left(22) + "..." : e.title;
        p.drawText(textX, y + 3, rect.width() - textX + rect.x() - 160, 18,
                   Qt::AlignVCenter, displayTitle);

        // Folder badge
        int badgeX = rect.x() + rect.width() - 150;
        QColor badgeBg = folderColors.contains(e.folder)
            ? folderColors[e.folder] : QColor(100, 116, 139);
        QString displayFolder = e.folder.length() > 12
            ? e.folder.left(12) + ".." : e.folder;

        QFontMetrics fm(p.font());
        int badgeTextW = QFontMetrics(QFont("Arial", 7)).horizontalAdvance(displayFolder);
        int badgePad = 6;
        int badgeW = badgeTextW + badgePad * 2;

        QPainterPath badgePath;
        badgePath.addRoundedRect(
            QRectF(badgeX, y + 4, badgeW, 14), 4.0, 4.0);
        p.setPen(Qt::NoPen);
        p.setBrush(badgeBg.lighter(160));
        p.drawPath(badgePath);

        p.setPen(badgeBg);
        p.setFont(QFont("Arial", 7));
        p.drawText(badgeX + badgePad, y + 4, badgeW - badgePad, 14,
                   Qt::AlignVCenter, displayFolder);

        // Category label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(textX, y + 20, rect.width() - textX + rect.x() - 160, 14,
                   Qt::AlignVCenter, e.category);

        // Relevance bar
        int barX = rect.x() + rect.width() - 120;
        int barY = y + 6;
        int barMaxW = 70;
        int barH = 7;

        QPainterPath barBg;
        barBg.addRoundedRect(QRectF(barX, barY, barMaxW, barH), 3.5, 3.5);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        p.drawPath(barBg);

        int fillW = static_cast<int>(e.relevance * barMaxW);
        if (fillW > 0) {
            QPainterPath barFill;
            barFill.addRoundedRect(QRectF(barX, barY, fillW, barH), 3.5, 3.5);
            p.setBrush(stripColor);
            p.drawPath(barFill);
        }

        // Relevance text
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(barX + barMaxW + 4, barY + 7,
                   QString::number(e.relevance, 'f', 2));

        // Visit count
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 7));
        p.drawText(barX, y + 28, barMaxW + 30, 14, Qt::AlignVCenter,
                   QString("Visits: %1").arg(e.visits));
    }
}

void PaperBookmarkVault2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(rect.x() + 4, rect.y() + 18, "Category Breakdown");

    auto counts = categoryCounts();
    QStringList categories = {"AI", "Physics", "Biology", "Chemistry", "Math"};
    QColor colors[] = {
        QColor(59, 130, 246),   // AI       #3b82f6
        QColor(22, 163, 74),    // Physics  #16a34a
        QColor(217, 119, 6),    // Biology  #d97706
        QColor(220, 38, 38),    // Chemistry #dc2626
        QColor(124, 58, 237)    // Math     #7c3aed
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
    for (int i = 0; i < 5; ++i) {
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

    for (int i = 0; i < 5; ++i) {
        int row = i / 2;
        int col = i % 2;
        // Shift second row of 3 items to center
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
        p.drawText(lx + 14, ly + 10,
                   QString("%1 (%2)").arg(categories[i]).arg(count));
    }
}

void PaperBookmarkVault2::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(rect.x() + 4, rect.y() + 18, "Statistics");

    struct Stat {
        QString label;
        QString value;
        QColor color;
    };

    QList<Stat> stats = {
        {"Total Bookmarks", QString::number(entries_.size()),
         QColor(59, 130, 246)},                                        // #3b82f6
        {"Avg Relevance",   QString::number(avgRelevance(), 'f', 2),
         QColor(22, 163, 74)},                                         // #16a34a
        {"Pinned",          QString::number(pinnedCount()),
         QColor(217, 119, 6)},                                         // #d97706
        {"Total Visits",    QString::number(
             std::accumulate(entries_.begin(), entries_.end(), 0,
                 [](int s, const BookmarkVault2Entry& e) { return s + e.visits; })),
         QColor(220, 38, 38)}                                          // #dc2626
    };

    int boxH = qMin(48, (rect.height() - 40) / 4);
    int boxGap = 6;

    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + 30 + i * (boxH + boxGap);

        // Box background
        QPainterPath boxPath;
        boxPath.addRoundedRect(
            QRectF(rect.x() + 4, y, rect.width() - 8, boxH), 6.0, 6.0);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawPath(boxPath);

        // Left accent bar
        QPainterPath accent;
        accent.addRoundedRect(
            QRectF(rect.x() + 4, y, 4, boxH), 2.0, 2.0);
        p.setBrush(stats[i].color);
        p.drawPath(accent);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 16, y + 2, rect.width() - 28, 24,
                   Qt::AlignVCenter, stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 16, y + 26, rect.width() - 28, 16,
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperBookmarkVault2::onAdd() {
    QString title = inputField_->text().trimmed();

    QMap<QString, QColor> catColors;
    catColors["AI"]        = QColor(59, 130, 246);
    catColors["Physics"]   = QColor(22, 163, 74);
    catColors["Biology"]   = QColor(217, 119, 6);
    catColors["Chemistry"] = QColor(220, 38, 38);
    catColors["Math"]      = QColor(124, 58, 237);

    QStringList folders = {"Reading List", "To Read", "Favorites", "Archive"};

    if (title.isEmpty()) {
        // Seed with a generated title
        QStringList seedTitles = {
            "Deep Reinforcement Learning Survey",
            "Quantum Entanglement in Photon Pairs",
            "CRISPR Gene Editing Techniques",
            "Organic Synthesis Catalysis",
            "Riemann Hypothesis Progress",
            "Transformer Architecture Analysis",
            "Dark Matter Detection Methods"
        };
        title = seedTitles[QRandomGenerator::global()->bounded(seedTitles.size())];
    }

    int cIdx = categoryCombo_->currentIndex();
    QString category;
    if (cIdx == 0) {
        QStringList cats = {"AI", "Physics", "Biology", "Chemistry", "Math"};
        category = cats[QRandomGenerator::global()->bounded(cats.size())];
    } else {
        category = categoryCombo_->currentText();
    }

    BookmarkVault2Entry entry;
    entry.id = entries_.isEmpty() ? 1 : entries_.last().id + 1;
    entry.title = title;
    entry.category = category;
    entry.folder = folders[QRandomGenerator::global()->bounded(folders.size())];
    entry.relevance = QRandomGenerator::global()->bounded(100) / 100.0;
    entry.visits = 1 + QRandomGenerator::global()->bounded(200);
    entry.pinned = QRandomGenerator::global()->bounded(3) == 0;
    entry.color = catColors.contains(category)
        ? catColors[category] : QColor(100, 116, 139);

    addEntry(entry);
    inputField_->clear();
}

void PaperBookmarkVault2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperBookmarkVault2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Add bookmarks to your vault");
        return;
    }
    infoLabel_->setText(
        QString("Vault: %1 entries | Pinned: %2 | Avg Relevance: %3")
            .arg(entries_.size())
            .arg(pinnedCount())
            .arg(avgRelevance(), 0, 'f', 2));
}

void PaperBookmarkVault2::loadSettings() {
    settings_.beginGroup("BookmarkVault2");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        BookmarkVault2Entry e;
        e.id = settings_.value("id").toInt();
        e.title = settings_.value("title").toString();
        e.category = settings_.value("category").toString();
        e.folder = settings_.value("folder").toString();
        e.relevance = settings_.value("relevance").toDouble();
        e.visits = settings_.value("visits").toInt();
        e.pinned = settings_.value("pinned").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();

    // Seed 8 default entries if first launch
    if (entries_.isEmpty()) {
        QMap<QString, QColor> catColors;
        catColors["AI"]        = QColor(59, 130, 246);
        catColors["Physics"]   = QColor(22, 163, 74);
        catColors["Biology"]   = QColor(217, 119, 6);
        catColors["Chemistry"] = QColor(220, 38, 38);
        catColors["Math"]      = QColor(124, 58, 237);

        QStringList folders = {"Reading List", "To Read", "Favorites", "Archive"};

        struct Seed {
            QString title; QString category; QString folder;
            qreal relevance; int visits; bool pinned;
        };

        Seed seeds[8] = {
            {"Attention Is All You Need",       "AI",        "Favorites",     0.95, 187, true },
            {"Quantum Field Theory Basics",     "Physics",   "Reading List",  0.82,  94, false},
            {"CRISPR-Cas9 Gene Editing",        "Biology",   "To Read",       0.88, 132, true },
            {"Organic Chemistry Reactions",     "Chemistry", "Archive",       0.71,  56, false},
            {"Riemann Hypothesis Overview",     "Math",      "Reading List",  0.79,  78, false},
            {"Neural Architecture Search",      "AI",        "To Read",       0.91, 145, true },
            {"Dark Energy Observations",        "Physics",   "Favorites",     0.85, 103, false},
            {"Protein Folding Prediction",      "Biology",   "Reading List",  0.93, 168, true },
        };

        for (int i = 0; i < 8; ++i) {
            BookmarkVault2Entry e;
            e.id = i + 1;
            e.title = seeds[i].title;
            e.category = seeds[i].category;
            e.folder = seeds[i].folder;
            e.relevance = seeds[i].relevance;
            e.visits = seeds[i].visits;
            e.pinned = seeds[i].pinned;
            e.color = catColors.contains(seeds[i].category)
                ? catColors[seeds[i].category] : QColor(100, 116, 139);
            entries_.append(e);
        }
        saveSettings();
    }

    updateInfo();
}

void PaperBookmarkVault2::saveSettings() {
    settings_.beginGroup("BookmarkVault2");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("title", entries_[i].title);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("folder", entries_[i].folder);
        settings_.setValue("relevance", entries_[i].relevance);
        settings_.setValue("visits", entries_[i].visits);
        settings_.setValue("pinned", entries_[i].pinned);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}

void PaperBookmarkVault2::addEntry(const BookmarkVault2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit bookmarkPinned(entry.id, entry.relevance);
    update();
}

QList<BookmarkVault2Entry> PaperBookmarkVault2::entries() const {
    return entries_;
}

int PaperBookmarkVault2::pinnedCount() const {
    int count = 0;
    for (const auto& e : entries_)
        if (e.pinned) ++count;
    return count;
}

qreal PaperBookmarkVault2::avgRelevance() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.relevance;
    return sum / entries_.size();
}

QMap<QString, int> PaperBookmarkVault2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}
