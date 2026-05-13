#include "workspace/PaperRebuttalBuilder2.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QtMath>
#include <numeric>

PaperRebuttalBuilder2::PaperRebuttalBuilder2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "RebuttalBuilder2")
{
    setupUI();
    loadSettings();
    if (entries_.isEmpty()) {
        QStringList points = {"Methodology is flawed", "Sample size too small",
                              "Results not reproducible", "Missing control group",
                              "Statistical analysis weak", "Overclaimed contributions",
                              "Ethical concerns raised", "Limited generalizability"};
        QStringList categories = {"Methodology", "Data", "Theory", "Ethics", "Impact"};
        QStringList responses = {"Robust validation confirms methodology",
                                 "Sample meets power analysis threshold",
                                 "Independent replication dataset provided",
                                 "Control experiments included in supplement",
                                 "Multiple statistical tests corroborate findings",
                                 "Claims aligned with empirical evidence",
                                 "IRB approval and consent documented",
                                 "Cross-domain validation strengthens scope"};
        QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6),
                           QColor(220,38,38), QColor(124,58,237)};
        for (int i = 0; i < 8; ++i) {
            RebuttalBuilder2Entry e;
            e.id = i + 1;
            e.point = points[i];
            e.category = categories[i % categories.size()];
            e.response = responses[i];
            e.strength = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
            e.evidence = 1 + QRandomGenerator::global()->bounded(9);
            e.convincing = e.strength >= 0.7;
            e.color = colors[i % 5];
            entries_.append(e);
        }
        saveSettings();
        updateInfo();
    }
}

void PaperRebuttalBuilder2::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Methodology", "Data", "Theory", "Ethics", "Impact"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Search rebuttals...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    buildBtn_ = new QPushButton("Build");
    buildBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(buildBtn_, &QPushButton::clicked, this, &PaperRebuttalBuilder2::onBuild);
    toolbar->addWidget(buildBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperRebuttalBuilder2::onClear);
    toolbar->addWidget(clearBtn_);

    toolbar->addStretch();

    infoLabel_ = new QLabel();
    infoLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    toolbar->addWidget(infoLabel_);

    mainLayout->addLayout(toolbar);
    setMinimumSize(720, 580);
}

void PaperRebuttalBuilder2::addEntry(const RebuttalBuilder2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit rebuttalBuilt(entry.id, entry.strength);
    update();
}

QList<RebuttalBuilder2Entry> PaperRebuttalBuilder2::entries() const { return entries_; }

int PaperRebuttalBuilder2::convincingCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.convincing) ++c;
    return c;
}

qreal PaperRebuttalBuilder2::avgStrength() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.strength;
    return sum / entries_.size();
}

QMap<QString, int> PaperRebuttalBuilder2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperRebuttalBuilder2::onBuild() {
    static const QStringList points = {"Methodology is flawed", "Sample size too small",
                                       "Results not reproducible", "Missing control group"};
    static const QStringList categories = {"Methodology", "Data", "Theory", "Ethics", "Impact"};
    static const QStringList responses = {"Robust validation confirms methodology",
                                          "Sample meets power analysis threshold",
                                          "Independent replication dataset provided",
                                          "Control experiments included in supplement"};
    static const QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6),
                                    QColor(220,38,38), QColor(124,58,237)};
    int cIdx = categoryCombo_->currentIndex();
    RebuttalBuilder2Entry e;
    e.id = entries_.isEmpty() ? 1 : entries_.last().id + 1;
    e.point = points[QRandomGenerator::global()->bounded(points.size())];
    e.category = (cIdx <= 0) ? categories[QRandomGenerator::global()->bounded(categories.size())]
                             : categories[cIdx - 1];
    e.response = responses[QRandomGenerator::global()->bounded(responses.size())];
    e.strength = 0.2 + QRandomGenerator::global()->bounded(80) / 100.0;
    e.evidence = 1 + QRandomGenerator::global()->bounded(9);
    e.convincing = e.strength >= 0.7;
    e.color = colors[QRandomGenerator::global()->bounded(5)];
    addEntry(e);
    inputField_->clear();
}

void PaperRebuttalBuilder2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperRebuttalBuilder2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No rebuttals yet. Click Build to begin.");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 14, QFont::Bold));
    p.drawText(20, 28, "Rebuttal Builder");

    int w = width(), h = height();
    int topY = 42;
    int bottomH = static_cast<int>(h * 0.22);
    int chartTop = h - bottomH;

    drawBuilderView(p, QRect(0, topY, static_cast<int>(w * 0.6), chartTop - topY));
    drawCategoryChart(p, QRect(static_cast<int>(w * 0.6), topY, w - static_cast<int>(w * 0.6), chartTop - topY));
    drawStats(p, QRect(0, chartTop, w, bottomH));
}

void PaperRebuttalBuilder2::drawBuilderView(QPainter& p, const QRect& rect) {
    QString filter = categoryCombo_->currentText();
    QString search = inputField_->text().trimmed().toLower();

    QList<const RebuttalBuilder2Entry*> visible;
    for (const auto& e : entries_) {
        if (filter != "All" && e.category != filter) continue;
        if (!search.isEmpty() && !e.point.toLower().contains(search)) continue;
        visible.append(&e);
    }

    int count = qMin(6, visible.size());
    if (count == 0) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 10));
        p.drawText(rect, Qt::AlignCenter, "No matching rebuttals");
        return;
    }

    int cardH = qMin(80, (rect.height() - 20) / count);
    int cardW = rect.width() - 40;

    for (int i = 0; i < count; ++i) {
        const auto& e = *visible[i];
        int cx = rect.x() + 20;
        int cy = rect.y() + 10 + i * (cardH + 6);

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(190));
        p.drawRoundedRect(cx, cy, cardW, cardH, 8, 8);

        // Left color bar
        p.setBrush(e.color);
        p.drawRoundedRect(cx, cy, 6, cardH, 3, 3);

        // Critique point (title)
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 10, QFont::Bold));
        p.drawText(cx + 14, cy + 4, cardW - 80, 18, Qt::AlignVCenter, e.point);

        // Convincing indicator
        if (e.convincing) {
            p.setPen(QColor(22, 163, 74));
            p.setFont(QFont("Arial", 9, QFont::Bold));
            p.drawText(cx + cardW - 60, cy + 4, 50, 18, Qt::AlignVCenter,
                       QString::fromUtf8("✓ Convincing"));
        } else {
            p.setPen(QColor(220, 38, 38));
            p.setFont(QFont("Arial", 9));
            p.drawText(cx + cardW - 50, cy + 4, 40, 18, Qt::AlignVCenter, "Weak");
        }

        // Category badge
        int badgeW = 70;
        int badgeH = 16;
        int badgeX = cx + 14;
        int badgeY = cy + 24;
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(150));
        p.drawRoundedRect(badgeX, badgeY, badgeW, badgeH, 8, 8);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(badgeX, badgeY, badgeW, badgeH, Qt::AlignCenter, e.category);

        // Evidence count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(badgeX + badgeW + 8, badgeY, 80, badgeH, Qt::AlignVCenter,
                   QString::number(e.evidence) + " evidence");

        // Response preview (truncated)
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 8));
        QString preview = e.response.length() > 50 ? e.response.left(47) + "..." : e.response;
        p.drawText(cx + 14, cy + 44, cardW - 80, 16, Qt::AlignVCenter, preview);

        // Strength gauge (arc)
        int arcSize = qMin(cardH - 24, 40);
        arcSize = qMax(arcSize, 30);
        int arcX = cx + cardW - arcSize - 10;
        int arcY = cy + cardH / 2 - arcSize / 2 + 6;
        QRectF arcRect(arcX, arcY, arcSize, arcSize);

        // Background arc
        p.setPen(QPen(QColor(226, 232, 240), 3));
        p.setBrush(Qt::NoBrush);
        p.drawArc(arcRect, 0, 360 * 16);

        // Strength arc
        int spanAngle = static_cast<int>(e.strength * 360 * 16);
        QColor arcColor = e.strength >= 0.7 ? QColor(22, 163, 74) :
                          e.strength >= 0.4 ? QColor(217, 119, 6) : QColor(220, 38, 38);
        p.setPen(QPen(arcColor, 3));
        p.drawArc(arcRect, 90 * 16, -spanAngle);

        // Strength percentage text
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(arcRect, Qt::AlignCenter,
                   QString::number(static_cast<int>(e.strength * 100)) + "%");
    }
}

void PaperRebuttalBuilder2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(rect.x() + 12, rect.y() + 16, "Category Distribution");

    auto counts = categoryCounts();
    static const QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6),
                                    QColor(220,38,38), QColor(124,58,237)};
    static const QString labels[] = {"Methodology", "Data", "Theory", "Ethics", "Impact"};

    int maxCount = 0;
    for (int i = 0; i < 5; ++i)
        maxCount = qMax(maxCount, counts.value(labels[i], 0));
    if (maxCount == 0) return;

    int chartX = rect.x() + 12;
    int chartY = rect.y() + 32;
    int chartW = rect.width() - 60;
    int barH = 22;
    int gap = 8;

    for (int i = 0; i < 5; ++i) {
        int count = counts.value(labels[i], 0);
        int by = chartY + i * (barH + gap);

        // Label
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 9));
        p.drawText(chartX, by, 70, barH, Qt::AlignVCenter | Qt::AlignRight, labels[i]);

        // Bar background
        int barX = chartX + 76;
        int fillW = static_cast<int>((static_cast<qreal>(count) / maxCount) * (chartW - 30));
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(241, 245, 249));
        p.drawRoundedRect(barX, by + 2, chartW - 30, barH - 4, 4, 4);

        // Bar fill
        if (fillW > 0) {
            p.setBrush(colors[i]);
            p.drawRoundedRect(barX, by + 2, fillW, barH - 4, 4, 4);
        }

        // Count label
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(barX + chartW - 28, by, 28, barH, Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(count));
    }
}

void PaperRebuttalBuilder2::drawStats(QPainter& p, const QRect& rect) {
    int totalEvidence = std::accumulate(entries_.begin(), entries_.end(), 0,
        [](int sum, const RebuttalBuilder2Entry& e) { return sum + e.evidence; });

    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Rebuttals", QString::number(entries_.size()), QColor(59,130,246)},
        {"Convincing Count", QString::number(convincingCount()), QColor(22,163,74)},
        {"Avg Strength", QString::number(avgStrength() * 100, 'f', 1) + "%", QColor(217,119,6)},
        {"Total Evidence", QString::number(totalEvidence), QColor(124,58,237)}
    };

    int boxW = (rect.width() - 50) / 4;
    int boxH = qMin(56, rect.height() - 16);
    int startX = rect.x() + 10;
    int startY = rect.y() + 8;

    for (int i = 0; i < stats.size(); ++i) {
        int bx = startX + i * (boxW + 10);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(192));
        p.drawRoundedRect(bx, startY, boxW, boxH, 8, 8);

        // Top accent line
        p.setBrush(stats[i].color);
        p.drawRoundedRect(bx, startY, boxW, 4, 8, 8);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 16, QFont::Bold));
        p.drawText(bx + 8, startY + 8, boxW - 16, 28, Qt::AlignVCenter, stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(bx + 8, startY + 36, boxW - 16, 16, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperRebuttalBuilder2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("No rebuttals");
        return;
    }
    infoLabel_->setText(QString("%1 rebuttals | %2 convincing | %3% avg | %4 evidence")
        .arg(entries_.size())
        .arg(convincingCount())
        .arg(avgStrength() * 100, 0, 'f', 0)
        .arg(std::accumulate(entries_.begin(), entries_.end(), 0,
            [](int s, const RebuttalBuilder2Entry& e) { return s + e.evidence; })));
}

void PaperRebuttalBuilder2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        RebuttalBuilder2Entry e;
        e.id = settings_.value("id").toInt();
        e.point = settings_.value("point").toString();
        e.category = settings_.value("category").toString();
        e.response = settings_.value("response").toString();
        e.strength = settings_.value("strength").toDouble();
        e.evidence = settings_.value("evidence").toInt();
        e.convincing = settings_.value("convincing").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperRebuttalBuilder2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("point", entries_[i].point);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("response", entries_[i].response);
        settings_.setValue("strength", entries_[i].strength);
        settings_.setValue("evidence", entries_[i].evidence);
        settings_.setValue("convincing", entries_[i].convincing);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
