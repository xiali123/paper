#include "workspace/PaperSprintRetro.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>

PaperSprintRetro::PaperSprintRetro(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "SprintRetro")
{
    setupUI();
    loadSettings();
}

void PaperSprintRetro::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // Top toolbar row
    auto* toolbar = new QHBoxLayout();
    toolbar->setSpacing(6);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Went Well", "Improve", "Action Item", "Blocker"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 5px 8px; border: 1px solid #cbd5e1; border-radius: 4px; "
        "background: white; min-width: 110px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter feedback...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    reviewBtn_ = new QPushButton("Review");
    reviewBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 5px 14px; "
        "border-radius: 4px; border: none; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(reviewBtn_, &QPushButton::clicked, this, &PaperSprintRetro::onReview);
    toolbar->addWidget(reviewBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet(
        "QPushButton { color: #dc2626; padding: 5px 14px; border: 1px solid #fca5a5; "
        "border-radius: 4px; background: white; }"
        "QPushButton:hover { background: #fef2f2; }");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperSprintRetro::onClear);
    toolbar->addWidget(clearBtn_);

    mainLayout->addLayout(toolbar);

    // Paint area (stretch to fill available space)
    mainLayout->addStretch(1);

    // Bottom info label
    infoLabel_ = new QLabel("Entries: 0 | Resolved: 0% | Avg Score: 0.0");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    mainLayout->addWidget(infoLabel_);

    setMinimumSize(640, 480);
}

void PaperSprintRetro::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    int w = width(), h = height();

    // Account for toolbar (~40px top) + info label (~24px bottom) + margins
    int topMargin = 44;
    int bottomMargin = 30;
    int sideMargin = 8;
    int usableH = h - topMargin - bottomMargin;

    if (usableH < 100 || w < 200) return;

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(QRect(sideMargin, topMargin, w - 2 * sideMargin, usableH),
                   Qt::AlignCenter, "Add feedback to start your sprint retrospective");
        return;
    }

    // Top half: retro sticky-note view
    int retroH = usableH / 2;
    drawRetroView(p, QRect(sideMargin, topMargin, w - 2 * sideMargin, retroH));

    // Bottom-left quarter: category chart
    int halfW = (w - 2 * sideMargin) / 2 - 4;
    int chartTop = topMargin + retroH + 4;
    int chartH = usableH - retroH - 4;
    drawCategoryChart(p, QRect(sideMargin, chartTop, halfW, chartH));

    // Bottom-right quarter: stats
    int statsX = sideMargin + halfW + 8;
    drawStats(p, QRect(statsX, chartTop, w - statsX - sideMargin, chartH));
}

void PaperSprintRetro::drawRetroView(QPainter& p, const QRect& rect) {
    // Section title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 18, "Sprint Retrospective");

    // Filter by category combo
    QString filter = categoryCombo_->currentText();
    QList<int> visible;
    for (int i = 0; i < entries_.size(); ++i) {
        if (filter == "All" || entries_[i].category == filter)
            visible.append(i);
    }

    if (visible.isEmpty()) {
        p.setPen(QColor(148, 163, 184));
        p.setFont(QFont("Arial", 10));
        p.drawText(rect.adjusted(0, 30, 0, 0), Qt::AlignHCenter | Qt::AlignTop,
                   "No entries for this category");
        return;
    }

    // Layout sticky-note cards in a grid
    int headerH = 26;
    int cardPad = 6;
    int cardW = qMin(200, (rect.width() - cardPad) / qMax(1, qMin(4, visible.size())) - cardPad);
    int cols = qMax(1, rect.width() / (cardW + cardPad));
    int cardH = qMin(72, (rect.height() - headerH - 10) / qMax(1, (visible.size() + cols - 1) / cols) - cardPad);

    for (int idx = 0; idx < visible.size(); ++idx) {
        int row = idx / cols;
        int col = idx % cols;
        int x = rect.x() + col * (cardW + cardPad);
        int y = rect.y() + headerH + row * (cardH + cardPad);

        const auto& e = entries_[visible[idx]];

        // Card background with QPainterPath rounded rect
        QPainterPath cardPath;
        cardPath.addRoundedRect(x, y, cardW, cardH, 6, 6);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(248, 250, 252));
        p.drawPath(cardPath);

        // Category color border (left accent)
        QPainterPath accent;
        accent.addRoundedRect(x, y, 5, cardH, 2, 2);
        p.setBrush(e.color);
        p.drawPath(accent);

        // Sprint label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(x + 10, y + 4, cardW - 18, 12, Qt::AlignVCenter, e.sprint);

        // Feedback text (truncated)
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        QString feedback = e.feedback;
        if (feedback.length() > 24) feedback = feedback.left(22) + "...";
        p.drawText(x + 10, y + 16, cardW - 18, 14, Qt::AlignVCenter, feedback);

        // Score badge
        p.setPen(Qt::NoPen);
        QColor badgeBg = e.score >= 7 ? QColor(34, 197, 94) :
                         e.score >= 4 ? QColor(234, 179, 8) :
                                        QColor(239, 68, 68);
        QPainterPath badgePath;
        qreal badgeX = x + cardW - 38;
        qreal badgeY = y + 4;
        badgePath.addRoundedRect(badgeX, badgeY, 30, 14, 4, 4);
        p.setBrush(badgeBg);
        p.drawPath(badgePath);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(QRectF(badgeX, badgeY, 30, 14), Qt::AlignCenter,
                   QString::number(static_cast<int>(e.score)));

        // Action items count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(x + 10, y + 32, cardW / 2 - 10, 12, Qt::AlignVCenter,
                   "Actions: " + QString::number(e.actionItems));

        // Resolved checkmark
        if (e.resolved) {
            qreal checkX = x + cardW - 20;
            qreal checkY = y + cardH - 18;
            QPainterPath checkBg;
            checkBg.addEllipse(checkX, checkY, 14, 14);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(22, 163, 74));
            p.drawPath(checkBg);
            // Draw checkmark
            QPen checkPen(Qt::white, 2);
            p.setPen(checkPen);
            p.drawLine(QPointF(checkX + 3, checkY + 7),
                       QPointF(checkX + 6, checkY + 10));
            p.drawLine(QPointF(checkX + 6, checkY + 10),
                       QPointF(checkX + 11, checkY + 4));
        }
    }
}

void PaperSprintRetro::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QString labels[] = {"Went Well", "Improve", "Action Item", "Blocker"};
    QColor colors[] = {
        QColor(59, 130, 246),   // #3b82f6 went well
        QColor(22, 163, 74),    // #16a34a improve
        QColor(124, 58, 237),   // #7c3aed action item
        QColor(220, 38, 38)     // #dc2626 blocker
    };

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(20, (rect.height() - 40) / 4);
    int chartWidth = rect.width() - 90;

    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 24 + i * (barH + 8);
        int count = counts.contains(labels[i]) ? counts[labels[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * chartWidth);
        barW = qMax(2, barW);

        // Label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + 2, 75, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        // Bar with QPainterPath
        QPainterPath barPath;
        barPath.addRoundedRect(rect.x() + 80, y, barW, barH - 2, 3, 3);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawPath(barPath);

        // Count label after bar
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 84 + barW, y + barH - 4, QString::number(count));
    }
}

void PaperSprintRetro::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Entries", QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Avg Score",     QString::number(avgScore(), 'f', 1), QColor(124, 58, 237)},
        {"Resolved",      QString("%1 (%2%)").arg(resolvedCount()).arg(
                              entries_.isEmpty() ? 0
                              : static_cast<int>(resolvedCount() * 100.0 / entries_.size())),
                          QColor(22, 163, 74)}
    };

    int boxH = qMin(42, (rect.height() - 10) / 3);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);

        // Background box
        QPainterPath boxPath;
        boxPath.addRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawPath(boxPath);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() - 20, 22,
                   Qt::AlignVCenter, stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 26, rect.width() - 20, 14,
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperSprintRetro::onReview() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    // Category mapping with dedicated colors
    struct CatInfo { QString name; QColor color; };
    CatInfo catMap[] = {
        {"Went Well",   QColor(59, 130, 246)},   // #3b82f6
        {"Improve",     QColor(22, 163, 74)},     // #16a34a
        {"Action Item", QColor(124, 58, 237)},    // #7c3aed
        {"Blocker",     QColor(220, 38, 38)}      // #dc2626
    };

    int cIdx = categoryCombo_->currentIndex();
    QString category;
    QColor color;

    if (cIdx == 0) {
        // "All" selected - pick random category
        int pick = QRandomGenerator::global()->bounded(4);
        category = catMap[pick].name;
        color = catMap[pick].color;
    } else {
        category = catMap[cIdx - 1].name;
        color = catMap[cIdx - 1].color;
    }

    SprintRetroEntry entry;
    entry.id = entries_.isEmpty() ? 1 : entries_.last().id + 1;
    entry.sprint = "Sprint " + QString::number((entry.id - 1) / 5 + 1);
    entry.category = category;
    entry.feedback = text;
    entry.score = static_cast<qreal>(QRandomGenerator::global()->bounded(5, 11)) / 1.0; // 5-10
    entry.actionItems = QRandomGenerator::global()->bounded(6); // 0-5
    entry.resolved = QRandomGenerator::global()->bounded(2) == 1;
    entry.color = color;

    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
    inputField_->clear();

    emit retroCompleted(entry.id, entry.score);
}

void PaperSprintRetro::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperSprintRetro::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Entries: 0 | Resolved: 0% | Avg Score: 0.0");
        return;
    }
    int resolvedPct = static_cast<int>(resolvedCount() * 100.0 / entries_.size());
    infoLabel_->setText(QString("Entries: %1 | Resolved: %2% | Avg Score: %3")
        .arg(entries_.size())
        .arg(resolvedPct)
        .arg(avgScore(), 0, 'f', 1));
}

void PaperSprintRetro::loadSettings() {
    settings_.beginGroup("SprintRetro");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        SprintRetroEntry e;
        e.id = settings_.value("id").toInt();
        e.sprint = settings_.value("sprint").toString();
        e.category = settings_.value("category").toString();
        e.feedback = settings_.value("feedback").toString();
        e.score = settings_.value("score").toDouble();
        e.actionItems = settings_.value("actionItems").toInt();
        e.resolved = settings_.value("resolved").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperSprintRetro::saveSettings() {
    settings_.beginGroup("SprintRetro");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("sprint", entries_[i].sprint);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("feedback", entries_[i].feedback);
        settings_.setValue("score", entries_[i].score);
        settings_.setValue("actionItems", entries_[i].actionItems);
        settings_.setValue("resolved", entries_[i].resolved);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}

void PaperSprintRetro::addEntry(const SprintRetroEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<SprintRetroEntry> PaperSprintRetro::entries() const {
    return entries_;
}

int PaperSprintRetro::resolvedCount() const {
    int count = 0;
    for (const auto& e : entries_)
        if (e.resolved) ++count;
    return count;
}

qreal PaperSprintRetro::avgScore() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.score;
    return sum / entries_.size();
}

QMap<QString, int> PaperSprintRetro::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}
