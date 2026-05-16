#include "workspace/PaperGrantTracker2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QFontMetrics>
#include <cmath>

PaperGrantTracker2::PaperGrantTracker2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "GrantTracker2")
{
    setupUI();
    loadSettings();

    if (entries_.isEmpty()) {
        QColor catColors[] = {
            QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
            QColor("#dc2626"), QColor("#7c3aed")
        };
        QString categories[] = {"Federal", "Foundation", "Industry", "Fellowship", "Travel"};
        QString grants[] = {
            "NSF CAREER Award", "Moore Foundation Grant", "Google Research Award",
            "HHMI Fellowship", "ACS Travel Grant", "DOE Computational Science",
            "Simons Foundation Bridge", "IBM PhD Fellowship"
        };
        QString agencies[] = {
            "NSF", "Moore Foundation", "Google", "HHMI",
            "ACS", "DOE", "Simons Foundation", "IBM"
        };
        qreal amounts[] = {500.0, 350.0, 150.0, 80.0, 5.0, 750.0, 200.0, 120.0};
        int milestones[] = {8, 6, 4, 3, 2, 10, 5, 4};
        bool funded[] = {true, true, false, true, true, false, true, false};

        for (int i = 0; i < 8; ++i) {
            GrantTracker2Entry e;
            e.id = i + 1;
            e.grant = grants[i];
            e.category = categories[i % 5];
            e.agency = agencies[i];
            e.amount = amounts[i];
            e.milestones = milestones[i];
            e.funded = funded[i];
            e.color = catColors[i % 5];
            entries_.append(e);
        }
        saveSettings();
        updateInfo();
    }
}

void PaperGrantTracker2::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Federal", "Foundation", "Industry", "Fellowship", "Travel"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; "
        "background: white; min-width: 120px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Grant name...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_);

    trackBtn_ = new QPushButton("Track");
    trackBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 16px; "
        "border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(trackBtn_, &QPushButton::clicked, this, &PaperGrantTracker2::onTrack);
    toolbar->addWidget(trackBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet(
        "QPushButton { color: #dc2626; padding: 4px 12px; border: 1px solid #fca5a5; "
        "border-radius: 4px; background: #fef2f2; }"
        "QPushButton:hover { background: #fee2e2; }");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperGrantTracker2::onClear);
    toolbar->addWidget(clearBtn_);

    toolbar->addStretch();
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Track research grants by category");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(700, 520);
}

void PaperGrantTracker2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No grants tracked yet");
        return;
    }

    int w = width(), h = height();
    int topH = (h - 120) * 2 / 3;
    int botH = h - 120 - topH;

    drawTrackerView(p, QRect(10, 50, w / 2 - 15, topH));
    drawCategoryChart(p, QRect(w / 2 + 5, 50, w / 2 - 15, topH));
    drawStats(p, QRect(10, 60 + topH, w - 20, botH));
}

void PaperGrantTracker2::drawTrackerView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(rect.x(), rect.y() - 5, "Grant Tracker");

    int show = qMin(6, entries_.size());
    int cardH = qMin(60, (rect.height() - 10) / qMax(show, 1));
    qreal maxAmount = 1.0;
    for (const auto& e : entries_) maxAmount = qMax(maxAmount, e.amount);

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (cardH + 4);

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(248, 250, 252));
        p.drawRoundedRect(rect.x(), y, rect.width(), cardH, 6, 6);

        // Left color bar
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 5, cardH, 2, 2);

        // Grant name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 12, y + 3, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.grant.left(22));

        // Agency + category
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 12, y + 19, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.agency + " | " + e.category);

        // Milestone progress bar
        int barX = rect.x() + 12;
        int barW = rect.width() / 2 - 20;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        p.drawRoundedRect(barX, y + 36, barW, 6, 3, 3);
        int filledW = static_cast<int>(barW * qMin(e.milestones, 10) / 10.0);
        p.setBrush(e.color);
        p.drawRoundedRect(barX, y + 36, filledW, 6, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(barX, y + 44, barW, 14, Qt::AlignVCenter,
                   QString::number(e.milestones) + " milestones");

        // Amount
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 10, QFont::Bold));
        p.drawText(rect.x() + rect.width() / 2, y + 3, rect.width() / 2 - 12, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "$" + QString::number(e.amount, 'f', 0) + "K");

        // Amount bar
        int amtBarX = rect.x() + rect.width() / 2;
        int amtBarW = rect.width() / 2 - 15;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(241, 245, 249));
        p.drawRoundedRect(amtBarX, y + 24, amtBarW, 8, 4, 4);
        int amtFill = static_cast<int>(amtBarW * e.amount / maxAmount);
        p.setBrush(e.color);
        p.drawRoundedRect(amtBarX, y + 24, amtFill, 8, 4, 4);

        // Funded badge
        if (e.funded) {
            int badgeX = rect.x() + rect.width() - 60;
            int badgeY = y + 42;
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(220, 252, 231));
            p.drawRoundedRect(badgeX, badgeY, 52, 16, 8, 8);
            p.setPen(QColor(22, 163, 74));
            p.setFont(QFont("Arial", 7, QFont::Bold));
            p.drawText(badgeX, badgeY, 52, 16, Qt::AlignCenter, "Funded");
        }
    }
}

void PaperGrantTracker2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(rect.x(), rect.y() - 5, "Category Distribution");

    auto counts = categoryCounts();
    QStringList cats = {"Federal", "Foundation", "Industry", "Fellowship", "Travel"};
    QColor catColors[] = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };

    int total = 0;
    for (const auto& c : cats) total += counts.value(c, 0);
    if (total == 0) total = 1;

    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + rect.height() / 2;
    int outerR = qMin(rect.width(), rect.height()) / 2 - 30;
    int innerR = outerR * 55 / 100;

    // Draw donut segments
    qreal startAngle = 90.0 * 16;
    for (int i = 0; i < 5; ++i) {
        int count = counts.value(cats[i], 0);
        if (count == 0) continue;
        qreal span = 360.0 * 16 * count / total;

        p.setPen(Qt::NoPen);
        p.setBrush(catColors[i]);
        p.drawPie(cx - outerR, cy - outerR, outerR * 2, outerR * 2,
                  static_cast<int>(startAngle), static_cast<int>(span));
        startAngle += span;
    }

    // Inner circle (donut hole)
    p.setBrush(Qt::white);
    p.drawEllipse(cx - innerR, cy - innerR, innerR * 2, innerR * 2);

    // Center text
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 16, QFont::Bold));
    p.drawText(cx - innerR, cy - innerR, innerR * 2, innerR * 2,
               Qt::AlignCenter, QString::number(total));

    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 8));
    p.drawText(cx - innerR, cy + 4, innerR * 2, innerR,
               Qt::AlignHCenter | Qt::AlignTop, "grants");

    // Legend below donut
    int legendY = cy + outerR + 15;
    int legendW = rect.width() / 3;
    for (int i = 0; i < 5; ++i) {
        int lx = rect.x() + (i % 3) * legendW;
        int ly = legendY + (i / 3) * 18;

        p.setPen(Qt::NoPen);
        p.setBrush(catColors[i]);
        p.drawRoundedRect(lx, ly, 10, 10, 2, 2);

        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 8));
        p.drawText(lx + 14, ly, legendW - 14, 10, Qt::AlignVCenter,
                   cats[i] + " (" + QString::number(counts.value(cats[i], 0)) + ")");
    }
}

void PaperGrantTracker2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Grants", QString::number(entries_.size()), QColor("#3b82f6")},
        {"Funded", QString::number(fundedCount()), QColor("#16a34a")},
        {"Total Amount", "$" + QString::number(totalAmount(), 'f', 0) + "K", QColor("#d97706")},
        {"Avg Milestones", QString::number(
            entries_.isEmpty() ? 0 :
            std::accumulate(entries_.begin(), entries_.end(), 0,
                [](int s, const GrantTracker2Entry& e) { return s + e.milestones; })
            / static_cast<double>(entries_.size()), 'f', 1), QColor("#7c3aed")}
    };

    int cols = 2;
    int rows = 2;
    int gap = 10;
    int boxW = (rect.width() - gap * (cols - 1)) / cols;
    int boxH = (rect.height() - gap * (rows - 1)) / rows;

    for (int i = 0; i < stats.size(); ++i) {
        int col = i % cols;
        int row = i / cols;
        int bx = rect.x() + col * (boxW + gap);
        int by = rect.y() + row * (boxH + gap);

        // Box background
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(195));
        p.drawRoundedRect(bx, by, boxW, boxH, 8, 8);

        // Top accent line
        p.setBrush(stats[i].color);
        p.drawRoundedRect(bx, by, boxW, 3, 2, 2);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(bx + 12, by + 8, boxW - 24, boxH / 2, Qt::AlignVCenter,
                   stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(bx + 12, by + boxH / 2, boxW - 24, boxH / 2 - 4, Qt::AlignVCenter,
                   stats[i].label);
    }
}

void PaperGrantTracker2::addEntry(const GrantTracker2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
    emit grantUpdated(entry.id, entry.amount);
}

QList<GrantTracker2Entry> PaperGrantTracker2::entries() const {
    return entries_;
}

int PaperGrantTracker2::fundedCount() const {
    int count = 0;
    for (const auto& e : entries_) if (e.funded) ++count;
    return count;
}

qreal PaperGrantTracker2::totalAmount() const {
    qreal total = 0;
    for (const auto& e : entries_) total += e.amount;
    return total;
}

QMap<QString, int> PaperGrantTracker2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperGrantTracker2::onTrack() {
    QString grantName = inputField_->text().trimmed();
    if (grantName.isEmpty()) return;

    QColor catColors[] = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };
    QString categories[] = {"Federal", "Foundation", "Industry", "Fellowship", "Travel"};

    int comboIdx = categoryCombo_->currentIndex();
    QString category = (comboIdx > 0 && comboIdx <= 5) ? categories[comboIdx - 1] : categories[0];
    int colorIdx = (comboIdx > 0 && comboIdx <= 5) ? comboIdx - 1 : 0;

    GrantTracker2Entry e;
    e.id = entries_.isEmpty() ? 1 : entries_.last().id + 1;
    e.grant = grantName;
    e.category = category;
    e.agency = "Custom";
    e.amount = 100.0 + QRandomGenerator::global()->bounded(500);
    e.milestones = 1 + QRandomGenerator::global()->bounded(8);
    e.funded = QRandomGenerator::global()->bounded(2) == 0;
    e.color = catColors[colorIdx];

    addEntry(e);
    inputField_->clear();
}

void PaperGrantTracker2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Track research grants by category");
    update();
}

void PaperGrantTracker2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Track research grants by category");
        return;
    }
    infoLabel_->setText(QString("%1 grants | %2 funded | $%3K total")
        .arg(entries_.size())
        .arg(fundedCount())
        .arg(totalAmount(), 0, 'f', 0));
}

void PaperGrantTracker2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        GrantTracker2Entry e;
        e.id = settings_.value("id").toInt();
        e.grant = settings_.value("grant").toString();
        e.category = settings_.value("category").toString();
        e.agency = settings_.value("agency").toString();
        e.amount = settings_.value("amount").toDouble();
        e.milestones = settings_.value("milestones").toInt();
        e.funded = settings_.value("funded").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperGrantTracker2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("grant", entries_[i].grant);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("agency", entries_[i].agency);
        settings_.setValue("amount", entries_[i].amount);
        settings_.setValue("milestones", entries_[i].milestones);
        settings_.setValue("funded", entries_[i].funded);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
