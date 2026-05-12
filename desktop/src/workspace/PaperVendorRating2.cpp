#include "workspace/PaperVendorRating2.hpp"
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperVendorRating2::PaperVendorRating2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "VendorRating2")
{
    setupUI();
    loadSettings();
}

void PaperVendorRating2::setupUI() {
    auto* layout = new QHBoxLayout(this);

    // Left group: input controls
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Publisher", "Software", "Equipment", "Service"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Vendor name...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");

    rateBtn_ = new QPushButton("Rate");
    rateBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(rateBtn_, &QPushButton::clicked, this, &PaperVendorRating2::onRate);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperVendorRating2::onClear);

    infoLabel_ = new QLabel("Vendors: 0 | Recommended: 0 | Avg Rating: 0.0");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");

    auto* leftLayout = new QVBoxLayout();
    leftLayout->addWidget(categoryCombo_);
    leftLayout->addWidget(inputField_);
    auto* btnRow = new QHBoxLayout();
    btnRow->addWidget(rateBtn_);
    btnRow->addWidget(clearBtn_);
    leftLayout->addLayout(btnRow);
    leftLayout->addWidget(infoLabel_);
    leftLayout->addStretch();

    layout->addLayout(leftLayout, 1);
    layout->addStretch(2);

    setMinimumSize(640, 520);
}

void PaperVendorRating2::addEntry(const VendorRatingEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit vendorRated(entry.id, entry.rating);
    update();
}

QList<VendorRatingEntry> PaperVendorRating2::entries() const {
    return entries_;
}

int PaperVendorRating2::recommendedCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.recommended) ++c;
    return c;
}

qreal PaperVendorRating2::avgRating() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.rating;
    return sum / entries_.size();
}

QMap<QString, int> PaperVendorRating2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperVendorRating2::onRate() {
    QString name = inputField_->text().trimmed();
    if (name.isEmpty()) return;

    static const QStringList categories = {"publisher", "software", "equipment", "service"};
    static const QStringList serviceNames = {
        "Academic Press", "SciTools Inc", "LabSupply Co", "DataVerse Ltd",
        "ResearchHub", "BioChem Corp", "TechServe", "Quantum Labs"};

    int cIdx = categoryCombo_->currentIndex();

    VendorRatingEntry e;
    e.id = entries_.size() + 1;
    e.vendor = name.left(20);
    e.category = (cIdx == 0)
        ? categories[QRandomGenerator::global()->bounded(categories.size())]
        : categories[cIdx - 1];
    e.service = serviceNames[QRandomGenerator::global()->bounded(serviceNames.size())];
    e.rating = 1.0 + QRandomGenerator::global()->bounded(41) / 10.0; // 1.0 - 5.0
    e.reviews = 1 + QRandomGenerator::global()->bounded(500);
    e.recommended = e.rating > 4.0;

    // Assign color by category
    QColor catColors[] = {
        QColor(59, 130, 246),   // #3b82f6  publisher -> blue
        QColor(22, 163, 74),    // #16a34a  software  -> green
        QColor(217, 119, 6),    // #d97706  equipment -> amber
        QColor(220, 38, 38),    // #dc2626  service   -> red
        QColor(124, 58, 237)    // #7c3aed  fallback  -> purple
    };
    int ci = categories.indexOf(e.category);
    e.color = (ci >= 0 && ci < 4) ? catColors[ci] : catColors[4];

    addEntry(e);
    inputField_->clear();
}

void PaperVendorRating2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    repaint();
}

void PaperVendorRating2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No vendor ratings yet");
        return;
    }

    int w = width();
    int h = height();
    int colW = w / 3;

    // Column 1: Vendor list
    drawVendorList(p, QRect(0, 0, colW, h));
    // Column 2: Category chart
    drawCategoryChart(p, QRect(colW, 0, colW, h));
    // Column 3: Stats
    drawStats(p, QRect(2 * colW, 0, w - 2 * colW, h));
}

void PaperVendorRating2::drawVendorList(QPainter& p, const QRect& rect) {
    int pad = 10;
    int x = rect.x() + pad;
    int y = rect.y() + pad;
    int w = rect.width() - 2 * pad;

    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(x, y, w, 24, Qt::AlignLeft | Qt::AlignVCenter, "Vendor Ratings");
    y += 30;

    int show = qMin(12, entries_.size());
    int itemH = qMin(38, (rect.height() - y - pad) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int iy = y + i * (itemH + 3);

        // Background row
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(x, iy, w, itemH, 4, 4);

        // Left accent bar
        p.setBrush(e.color);
        p.drawRoundedRect(x, iy, 4, itemH, 2, 2);

        // Vendor name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(x + 10, iy + 2, w / 2 - 10, 18, Qt::AlignVCenter,
                   e.vendor);

        // Star rating (1-5 filled stars)
        int starX = x + w / 2 + 10;
        p.setFont(QFont("Arial", 10));
        int stars = qRound(e.rating);
        stars = qBound(1, stars, 5);
        QString starStr;
        for (int s = 0; s < 5; ++s)
            starStr += (s < stars) ? QChar(0x2605) : QChar(0x2606);
        p.setPen(QColor(217, 119, 6)); // amber for stars
        p.drawText(starX, iy + 2, w / 2 - 20, 18, Qt::AlignVCenter, starStr);

        // Recommended badge (green)
        if (e.recommended) {
            int badgeX = x + w - 48;
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(22, 163, 74)); // #16a34a
            p.drawRoundedRect(badgeX, iy + 3, 42, 14, 3, 3);
            p.setPen(Qt::white);
            p.setFont(QFont("Arial", 7, QFont::Bold));
            p.drawText(badgeX, iy + 3, 42, 14, Qt::AlignCenter, "REC");
        }

        // Sub-info: reviews count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(x + 10, iy + 18, w - 20, 16, Qt::AlignVCenter,
                   e.service + " | " + QString::number(e.reviews) + " reviews");
    }
}

void PaperVendorRating2::drawCategoryChart(QPainter& p, const QRect& rect) {
    int pad = 10;
    int x = rect.x() + pad;
    int y = rect.y() + pad;
    int w = rect.width() - 2 * pad;

    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(x, y, w, 24, Qt::AlignLeft | Qt::AlignVCenter, "Categories");
    y += 30;

    auto counts = categoryCounts();
    QStringList cats = {"publisher", "software", "equipment", "service"};
    QString labels[] = {"Publisher", "Software", "Equipment", "Service"};
    QColor colors[] = {
        QColor(59, 130, 246),  // #3b82f6
        QColor(22, 163, 74),   // #16a34a
        QColor(217, 119, 6),   // #d97706
        QColor(220, 38, 38)    // #dc2626
    };

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(28, (rect.height() - y - pad) / 4);

    for (int i = 0; i < 4; ++i) {
        int by = y + i * (barH + 6);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>(
            (static_cast<qreal>(count) / maxVal) * (w - 120));

        // Label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(x, by, 70, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        // Bar
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(x + 75, by + 2, barW, barH - 4, 3, 3);

        // Count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(x + 80 + barW, by, 40, barH, Qt::AlignVCenter,
                   QString::number(count));
    }
}

void PaperVendorRating2::drawStats(QPainter& p, const QRect& rect) {
    int pad = 10;
    int x = rect.x() + pad;
    int y = rect.y() + pad;
    int w = rect.width() - 2 * pad;

    struct Stat {
        QString label;
        QString value;
        QColor color;
    };

    QList<Stat> stats = {
        {"Total Vendors",     QString::number(entries_.size()), QColor(59, 130, 246)},   // #3b82f6
        {"Recommended",       QString::number(recommendedCount()), QColor(22, 163, 74)}, // #16a34a
        {"Avg Rating",        QString::number(avgRating(), 'f', 1) + "/5.0", QColor(124, 58, 237)} // #7c3aed
    };

    int boxH = qMin(60, (rect.height() - y - pad) / stats.size());

    for (int i = 0; i < stats.size(); ++i) {
        int by = y + i * (boxH + 8);

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(x, by, w, boxH, 6, 6);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 18, QFont::Bold));
        p.drawText(x + 12, by + 4, w - 24, boxH / 2, Qt::AlignVCenter,
                   stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(x + 12, by + boxH / 2, w - 24, boxH / 2 - 4,
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperVendorRating2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Vendors: 0 | Recommended: 0 | Avg Rating: 0.0");
        return;
    }
    infoLabel_->setText(
        QString("Vendors: %1 | Recommended: %2 | Avg Rating: %3")
            .arg(entries_.size())
            .arg(recommendedCount())
            .arg(avgRating(), 0, 'f', 1));
}

void PaperVendorRating2::loadSettings() {
    settings_.beginGroup("VendorRating2");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        VendorRatingEntry e;
        e.id          = settings_.value("id").toInt();
        e.vendor      = settings_.value("vendor").toString();
        e.category    = settings_.value("category").toString();
        e.service     = settings_.value("service").toString();
        e.rating      = settings_.value("rating").toDouble();
        e.reviews     = settings_.value("reviews").toInt();
        e.recommended = settings_.value("recommended").toBool();
        e.color       = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperVendorRating2::saveSettings() {
    settings_.beginGroup("VendorRating2");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& e = entries_[i];
        settings_.setValue("id",          e.id);
        settings_.setValue("vendor",      e.vendor);
        settings_.setValue("category",    e.category);
        settings_.setValue("service",     e.service);
        settings_.setValue("rating",      e.rating);
        settings_.setValue("reviews",     e.reviews);
        settings_.setValue("recommended", e.recommended);
        settings_.setValue("color",       e.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
