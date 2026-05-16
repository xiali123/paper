#include "analysis/PaperFallacyMapper2.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperFallacyMapper2::PaperFallacyMapper2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "FallacyMapper2")
{
    setupUI();
    loadSettings();

    if (entries_.isEmpty()) {
        QStringList categories = {"Logic", "Rhetoric", "Statistical", "Causal", "Analogical"};
        QStringList fallacies = {"Ad Hominem", "Straw Man", "Appeal to Authority",
                                 "False Dilemma", "Slippery Slope"};
        QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6),
                           QColor(220,38,38), QColor(124,58,237)};
        QString demoArgs[] = {
            "The author lacks credentials",
            "Opponent wants total deregulation",
            "Nobel laureate endorses this",
            "Either we ban it or society collapses",
            "One step leads to catastrophe",
            "Prior work is fundamentally flawed",
            "Data shows impossible correlation",
            "Like comparing apples to nuclear reactors"
        };

        for (int i = 0; i < 8; ++i) {
            FallacyMapper2Entry e;
            e.id = i + 1;
            e.argument = demoArgs[i];
            e.category = categories[i % categories.size()];
            e.fallacy = fallacies[i % fallacies.size()];
            e.severity = 0.2 + QRandomGenerator::global()->bounded(60) / 100.0;
            e.occurrences = 1 + QRandomGenerator::global()->bounded(10);
            e.critical = e.severity >= 0.7;
            e.color = colors[i % 5];
            entries_.append(e);
        }
        saveSettings();
        updateInfo();
        update();
    }
}

void PaperFallacyMapper2::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Logic", "Rhetoric", "Statistical", "Causal", "Analogical"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; "
        "min-width: 100px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Search arguments...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    mapBtn_ = new QPushButton("Map");
    mapBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; "
        "border-radius: 4px; font-weight: bold; }");
    connect(mapBtn_, &QPushButton::clicked, this, &PaperFallacyMapper2::onMap);
    toolbar->addWidget(mapBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626; padding: 4px 8px;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperFallacyMapper2::onClear);
    toolbar->addWidget(clearBtn_);

    mainLayout->addLayout(toolbar);

    infoLabel_ = new QLabel();
    infoLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 2px 4px;");
    mainLayout->addWidget(infoLabel_);

    setMinimumSize(720, 520);
}

void PaperFallacyMapper2::addEntry(const FallacyMapper2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit fallacyDetected(entry.id, entry.severity);
    update();
}

QList<FallacyMapper2Entry> PaperFallacyMapper2::entries() const {
    return entries_;
}

int PaperFallacyMapper2::criticalCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.critical) ++c;
    return c;
}

qreal PaperFallacyMapper2::avgSeverity() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.severity;
    return sum / entries_.size();
}

QMap<QString, int> PaperFallacyMapper2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperFallacyMapper2::onMap() {
    QStringList categories = {"Logic", "Rhetoric", "Statistical", "Causal", "Analogical"};
    QStringList fallacies = {"Ad Hominem", "Straw Man", "Appeal to Authority",
                             "False Dilemma", "Slippery Slope"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6),
                       QColor(220,38,38), QColor(124,58,237)};

    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) text = "Argument " + QString::number(entries_.size() + 1);

    int ci = QRandomGenerator::global()->bounded(categories.size());
    int fi = QRandomGenerator::global()->bounded(fallacies.size());

    FallacyMapper2Entry e;
    e.id = entries_.isEmpty() ? 1 : entries_.last().id + 1;
    e.argument = text;
    e.category = categories[ci];
    e.fallacy = fallacies[fi];
    e.severity = 0.15 + QRandomGenerator::global()->bounded(85) / 100.0;
    e.occurrences = 1 + QRandomGenerator::global()->bounded(12);
    e.critical = e.severity >= 0.7;
    e.color = colors[QRandomGenerator::global()->bounded(5)];

    addEntry(e);
    inputField_->clear();
}

void PaperFallacyMapper2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperFallacyMapper2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(248, 250, 252));

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 13));
        p.drawText(rect(), Qt::AlignCenter, "No fallacies mapped yet — click Map to begin");
        return;
    }

    int w = width();
    int h = height();
    int toolbarH = 70;

    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 14, QFont::Bold));
    p.drawText(20, toolbarH + 20, "Fallacy Mapper 2");

    int contentTop = toolbarH + 34;
    int contentH = h - contentTop - 10;
    int statsH = qMax(60, static_cast<int>(contentH * 0.25));
    int mapH = contentH - statsH - 10;
    int leftW = static_cast<int>(w * 0.6) - 20;
    int rightW = w - leftW - 40;

    drawMapperView(p, QRect(20, contentTop, leftW, mapH));
    drawCategoryChart(p, QRect(20 + leftW + 10, contentTop, rightW, mapH));
    drawStats(p, QRect(20, contentTop + mapH + 10, w - 40, statsH));
}

void PaperFallacyMapper2::drawMapperView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.x(), rect.y() - 2, "Argument Cards");

    // Filter by category
    QString filter = categoryCombo_->currentText();
    QList<const FallacyMapper2Entry*> visible;
    for (const auto& e : entries_) {
        if (filter == "All" || e.category == filter)
            visible.append(&e);
    }

    int show = qMin(8, visible.size());
    if (show == 0) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 10));
        p.drawText(rect, Qt::AlignCenter, "No entries for this category");
        return;
    }

    int cardH = qMin(48, (rect.height() - 10) / qMax(show, 1));
    int gap = 4;

    for (int i = 0; i < show; ++i) {
        const auto& e = *visible[i];
        int y = rect.y() + i * (cardH + gap);

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 255, 255));
        p.drawRoundedRect(rect.x(), y, rect.width(), cardH, 6, 6);

        // Color accent bar on left
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, cardH, 2, 2);

        // Fallacy name badge
        int badgeW = qMin(120, rect.width() / 4);
        p.setBrush(e.color.lighter(140));
        p.drawRoundedRect(rect.x() + 10, y + 4, badgeW, 16, 3, 3);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(rect.x() + 14, y + 4, badgeW - 8, 16,
                   Qt::AlignVCenter, e.fallacy);

        // Argument text
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        QString argText = e.argument.length() > 30
            ? e.argument.left(28) + "..." : e.argument;
        p.drawText(rect.x() + 10, y + 22, rect.width() - 20, 14,
                   Qt::AlignVCenter, argText);

        // Occurrence count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + rect.width() - 120, y + 22, 50, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.occurrences) + "x");

        // Critical warning icon
        if (e.critical) {
            p.setPen(QColor(220, 38, 38));
            p.setFont(QFont("Arial", 11, QFont::Bold));
            p.drawText(rect.x() + rect.width() - 60, y + 4, 24, 20,
                       Qt::AlignVCenter, "!!");
        }

        // Severity gauge — crescent arc
        int gaugeSize = qMin(28, cardH - 8);
        int gx = rect.x() + rect.width() - gaugeSize - 8;
        int gy = y + (cardH - gaugeSize) / 2;

        // Arc background
        p.setPen(QPen(QColor(226, 232, 240), 3));
        p.setBrush(Qt::NoBrush);
        p.drawArc(gx, gy, gaugeSize, gaugeSize, 30 * 16, 120 * 16);

        // Arc fill — color by severity
        QColor arcColor;
        if (e.severity > 0.7) arcColor = QColor(220, 38, 38);      // red
        else if (e.severity > 0.4) arcColor = QColor(217, 119, 6); // amber
        else arcColor = QColor(22, 163, 74);                        // green

        int spanAngle = static_cast<int>(120 * 16 * e.severity);
        p.setPen(QPen(arcColor, 3));
        p.drawArc(gx, gy, gaugeSize, gaugeSize, 30 * 16, spanAngle);

        // Severity text inside arc
        p.setPen(arcColor);
        p.setFont(QFont("Arial", 6, QFont::Bold));
        p.drawText(QRect(gx, gy, gaugeSize, gaugeSize), Qt::AlignCenter,
                   QString::number(e.severity * 100, 'f', 0) + "%");
    }
}

void PaperFallacyMapper2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.x(), rect.y() - 2, "Fallacy Distribution");

    QStringList categories = {"Logic", "Rhetoric", "Statistical", "Causal", "Analogical"};
    QStringList fallacies = {"Ad Hominem", "Straw Man", "Appeal to Authority",
                             "False Dilemma", "Slippery Slope"};
    QColor catColors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6),
                          QColor(220,38,38), QColor(124,58,237)};

    // Build stacked data: category -> fallacy counts
    QMap<QString, QMap<QString, int>> stacked;
    for (const auto& cat : categories) stacked[cat] = QMap<QString, int>();
    for (const auto& e : entries_) {
        if (stacked.contains(e.category))
            stacked[e.category][e.fallacy]++;
    }

    int barCount = categories.size();
    int labelW = 70;
    int barGap = 6;
    int maxBarH = qMin(30, (rect.height() - 30) / qMax(barCount, 1));

    // Find max total per category for scaling
    int maxTotal = 1;
    for (const auto& cat : categories) {
        int total = 0;
        for (const auto& f : fallacies) total += stacked[cat][f];
        maxTotal = qMax(maxTotal, total);
    }

    int availW = rect.width() - labelW - 20;

    for (int i = 0; i < barCount; ++i) {
        int y = rect.y() + 18 + i * (maxBarH + barGap);
        const QString& cat = categories[i];

        // Category label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y, labelW, maxBarH,
                   Qt::AlignRight | Qt::AlignVCenter, cat);

        // Stacked segments
        int x = rect.x() + labelW + 6;
        int totalForCat = 0;
        for (int f = 0; f < fallacies.size(); ++f) {
            int count = stacked[cat][fallacies[f]];
            if (count == 0) continue;
            totalForCat += count;
            int segW = static_cast<int>(
                (static_cast<qreal>(count) / maxTotal) * availW);

            p.setPen(Qt::NoPen);
            // Use fallacy index to pick a shade from the category color
            QColor segColor = catColors[i].lighter(100 + f * 25);
            p.setBrush(segColor);
            p.drawRoundedRect(x, y + 2, qMax(segW, 2), maxBarH - 4, 3, 3);
            x += segW + 1;
        }

        // Count at end
        if (totalForCat > 0) {
            p.setPen(QColor(100, 116, 139));
            p.setFont(QFont("Arial", 7));
            p.drawText(x + 4, y, 30, maxBarH,
                       Qt::AlignVCenter, QString::number(totalForCat));
        }
    }

    // Legend
    int legendY = rect.y() + 18 + barCount * (maxBarH + barGap) + 4;
    if (legendY + 14 < rect.y() + rect.height()) {
        p.setFont(QFont("Arial", 6));
        int lx = rect.x() + labelW + 6;
        for (int f = 0; f < fallacies.size(); ++f) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(100, 116, 139).lighter(140 - f * 10));
            p.drawRoundedRect(lx, legendY, 8, 8, 2, 2);
            p.setPen(QColor(100, 116, 139));
            p.drawText(lx + 11, legendY + 8, fallacies[f]);
            lx += fallacies[f].length() * 5 + 20;
        }
    }
}

void PaperFallacyMapper2::drawStats(QPainter& p, const QRect& rect) {
    int totalOccurrences = 0;
    for (const auto& e : entries_) totalOccurrences += e.occurrences;

    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Fallacies",  QString::number(entries_.size()), QColor(59,130,246)},
        {"Critical Count",   QString::number(criticalCount()), QColor(220,38,38)},
        {"Avg Severity",     QString::number(avgSeverity() * 100, 'f', 1) + "%", QColor(217,119,6)},
        {"Total Occurrences",QString::number(totalOccurrences), QColor(124,58,237)}
    };

    int boxCount = stats.size();
    int gap = 12;
    int boxW = (rect.width() - (boxCount - 1) * gap) / boxCount;

    for (int i = 0; i < boxCount; ++i) {
        int x = rect.x() + i * (boxW + gap);

        // Box background
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(x, rect.y(), boxW, rect.height(), 8, 8);

        // Top accent line
        p.setBrush(stats[i].color);
        p.drawRoundedRect(x, rect.y(), boxW, 3, 2, 2);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 18, QFont::Bold));
        p.drawText(x + 10, rect.y() + 6, boxW - 20, rect.height() / 2,
                   Qt::AlignVCenter, stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(x + 10, rect.y() + rect.height() / 2, boxW - 20, rect.height() / 2 - 4,
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperFallacyMapper2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Ready to map fallacies");
        return;
    }
    infoLabel_->setText(
        QString("%1 fallacies | %2 critical | avg %3% severity | %4 total occurrences")
            .arg(entries_.size())
            .arg(criticalCount())
            .arg(avgSeverity() * 100, 0, 'f', 0)
            .arg([&]{ int t=0; for(const auto& e: entries_) t+=e.occurrences; return t; }()));
}

void PaperFallacyMapper2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        FallacyMapper2Entry e;
        e.id = settings_.value("id").toInt();
        e.argument = settings_.value("argument").toString();
        e.category = settings_.value("category").toString();
        e.fallacy = settings_.value("fallacy").toString();
        e.severity = settings_.value("severity").toDouble();
        e.occurrences = settings_.value("occurrences").toInt();
        e.critical = settings_.value("critical").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperFallacyMapper2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("argument", entries_[i].argument);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("fallacy", entries_[i].fallacy);
        settings_.setValue("severity", entries_[i].severity);
        settings_.setValue("occurrences", entries_[i].occurrences);
        settings_.setValue("critical", entries_[i].critical);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
