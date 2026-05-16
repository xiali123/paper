#include "workspace/PaperTimelineRuler.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>

PaperTimelineRuler::PaperTimelineRuler(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "TimelineRuler")
{
    setupUI();
    loadSettings();
}

void PaperTimelineRuler::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Planning", "Execution", "Review", "Delivery"});
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter milestone...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 2);

    planBtn_ = new QPushButton("Plan");
    planBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(planBtn_, &QPushButton::clicked, this, &PaperTimelineRuler::onPlan);
    toolbar->addWidget(planBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperTimelineRuler::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Plan milestones");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(640, 520);
}

void PaperTimelineRuler::addEntry(const TimelineRulerEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit milestoneReached(entry.id, entry.progress);
    update();
}

QList<TimelineRulerEntry> PaperTimelineRuler::entries() const { return entries_; }

int PaperTimelineRuler::onTrackCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.onTrack) c++;
    return c;
}

qreal PaperTimelineRuler::avgProgress() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.progress;
    return sum / entries_.size();
}

QMap<QString, int> PaperTimelineRuler::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperTimelineRuler::onPlan() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"planning", "execution", "review", "delivery"};
    QStringList phases = {"Phase 1", "Phase 2", "Phase 3", "Phase 4"};
    QMap<QString, QColor> phaseColors = {
        {"planning", QColor(59, 130, 246)},
        {"execution", QColor(22, 163, 74)},
        {"review", QColor(124, 58, 237)},
        {"delivery", QColor(217, 119, 6)}
    };

    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);

    for (int i = 0; i < count; ++i) {
        TimelineRulerEntry e;
        e.id = entries_.size() + 1;
        e.milestone = text.left(12) + " ms" + QString::number(i);
        e.category = cIdx == 0
            ? categories[QRandomGenerator::global()->bounded(categories.size())]
            : categories[cIdx - 1];
        e.phase = phases[QRandomGenerator::global()->bounded(phases.size())];
        e.progress = QRandomGenerator::global()->bounded(100) / 100.0;
        e.days = QRandomGenerator::global()->bounded(60);
        e.onTrack = e.days <= 30 && e.progress >= 0.3;
        e.color = phaseColors.value(e.category, QColor(59, 130, 246));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperTimelineRuler::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Plan milestones");
    update();
}

void PaperTimelineRuler::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Plan milestones");
        return;
    }

    int w = width(), h = height();

    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Timeline Ruler");

    // Top: ruler view
    drawRulerView(p, QRect(20, 50, w - 40, h / 2 - 30));

    // Bottom-left: category chart
    drawCategoryChart(p, QRect(20, h / 2 + 30, w / 2 - 30, h / 2 - 50));

    // Bottom-right: stats
    drawStats(p, QRect(w / 2 + 10, h / 2 + 30, w / 2 - 30, h / 2 - 50));
}

void PaperTimelineRuler::drawRulerView(QPainter& p, const QRect& r) {
    int show = qMin(8, entries_.size());
    int rowH = qMin(38, (r.height() - 30) / qMax(show, 1));
    int labelW = 120;
    int barX = r.x() + labelW;
    int barW = r.width() - labelW - 70;

    // Draw horizontal ruler line
    p.setPen(QPen(QColor(203, 213, 225), 1));
    p.drawLine(r.x(), r.y() + 18, r.x() + r.width(), r.y() + 18);

    // Tick marks on ruler
    p.setFont(QFont("Arial", 7));
    p.setPen(QColor(148, 163, 184));
    for (int t = 0; t <= 10; ++t) {
        int tx = barX + static_cast<int>(t / 10.0 * barW);
        p.drawLine(tx, r.y() + 14, tx, r.y() + 22);
        p.drawText(tx - 10, r.y() + 12, 20, 10, Qt::AlignCenter,
                   QString::number(t * 10) + "%");
    }

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = r.y() + 26 + i * (rowH + 4);
        int fullH = rowH;

        // Background row
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(248, 250, 252));
        QPainterPath bgPath;
        bgPath.addRoundedRect(r.x(), y, r.width(), fullH, 6, 6);
        p.drawPath(bgPath);

        // Phase color indicator (left stripe)
        p.setBrush(e.color);
        QPainterPath stripePath;
        stripePath.addRoundedRect(r.x(), y, 5, fullH, 2, 2);
        p.drawPath(stripePath);

        // Milestone name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(r.x() + 10, y, labelW - 15, fullH, Qt::AlignVCenter,
                   e.milestone.left(16));

        // Progress bar background
        int pbY = y + 6;
        int pbH = fullH - 20;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        QPainterPath barBg;
        barBg.addRoundedRect(barX, pbY, barW, pbH, 3, 3);
        p.drawPath(barBg);

        // Progress bar fill
        int fillW = static_cast<int>(e.progress * barW);
        if (fillW > 0) {
            p.setBrush(e.color);
            QPainterPath barFill;
            barFill.addRoundedRect(barX, pbY, fillW, pbH, 3, 3);
            p.drawPath(barFill);
        }

        // Progress text on bar
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        if (fillW > 30) {
            p.drawText(barX + 4, pbY, fillW - 8, pbH,
                       Qt::AlignVCenter, QString::number(e.progress * 100, 'f', 0) + "%");
        }

        // Days countdown
        p.setPen(e.onTrack ? QColor(22, 163, 74) : QColor(220, 38, 38));
        p.setFont(QFont("Arial", 8));
        p.drawText(barX + barW + 6, y, 60, fullH, Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.days) + "d");

        // On-track indicator dot
        if (e.onTrack) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(22, 163, 74));
            p.drawEllipse(r.x() + r.width() - 14, y + fullH / 2 - 4, 8, 8);
        } else {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(220, 38, 38));
            p.drawEllipse(r.x() + r.width() - 14, y + fullH / 2 - 4, 8, 8);
        }

        // Phase label below progress bar
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 6));
        p.drawText(barX, y + fullH - 8, barW, 10, Qt::AlignLeft, e.phase);
    }
}

void PaperTimelineRuler::drawCategoryChart(QPainter& p, const QRect& r) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(r.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList categories = {"planning", "execution", "review", "delivery"};
    QString labels[] = {"Planning", "Execution", "Review", "Delivery"};
    QColor colors[] = {
        QColor(59, 130, 246),
        QColor(22, 163, 74),
        QColor(124, 58, 237),
        QColor(217, 119, 6)
    };

    int maxVal = 1;
    for (const auto& cat : categories) {
        if (counts.contains(cat)) maxVal = qMax(maxVal, counts[cat]);
    }

    int barH = qMin(24, (r.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = r.y() + 22 + i * (barH + 6);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int bw = static_cast<int>((static_cast<qreal>(count) / maxVal) * (r.width() - 130));

        // Label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(r.x(), y, 70, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        // Bar background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(241, 245, 249));
        QPainterPath bgPath;
        bgPath.addRoundedRect(r.x() + 75, y, r.width() - 130, barH - 2, 4, 4);
        p.drawPath(bgPath);

        // Bar fill
        if (bw > 0) {
            p.setBrush(colors[i]);
            QPainterPath fillPath;
            fillPath.addRoundedRect(r.x() + 75, y, bw, barH - 2, 4, 4);
            p.drawPath(fillPath);
        }

        // Count label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(r.x() + 78 + bw, y, 40, barH, Qt::AlignVCenter,
                   QString::number(count));
    }
}

void PaperTimelineRuler::drawStats(QPainter& p, const QRect& r) {
    struct Stat {
        QString label;
        QString value;
        QColor color;
    };

    QList<Stat> stats = {
        {"Total Milestones", QString::number(entries_.size()), QColor(59, 130, 246)},
        {"On Track", QString::number(onTrackCount()), QColor(22, 163, 74)},
        {"Avg Progress", QString::number(avgProgress() * 100, 'f', 0) + "%", QColor(245, 158, 11)},
        {"Categories", QString::number(categoryCounts().size()), QColor(124, 58, 237)}
    };

    int boxH = qMin(42, (r.height() - 10) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = r.y() + i * (boxH + 5);

        // Box background
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        QPainterPath boxPath;
        boxPath.addRoundedRect(r.x(), y, r.width(), boxH, 6, 6);
        p.drawPath(boxPath);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(r.x() + 10, y + 5, r.width() - 20, 22, Qt::AlignVCenter,
                   stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(r.x() + 10, y + 26, r.width() - 20, 14, Qt::AlignVCenter,
                   stats[i].label);
    }
}

void PaperTimelineRuler::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Plan milestones");
        return;
    }
    int onTrack = onTrackCount();
    infoLabel_->setText(QString("%1 milestones | %2 on track | %3% avg progress")
        .arg(entries_.size())
        .arg(onTrack)
        .arg(avgProgress() * 100, 0, 'f', 0));
}

void PaperTimelineRuler::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        TimelineRulerEntry e;
        e.id = settings_.value("id").toInt();
        e.milestone = settings_.value("milestone").toString();
        e.category = settings_.value("category").toString();
        e.phase = settings_.value("phase").toString();
        e.progress = settings_.value("progress").toDouble();
        e.days = settings_.value("days").toInt();
        e.onTrack = settings_.value("onTrack").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperTimelineRuler::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("milestone", entries_[i].milestone);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("phase", entries_[i].phase);
        settings_.setValue("progress", entries_[i].progress);
        settings_.setValue("days", entries_[i].days);
        settings_.setValue("onTrack", entries_[i].onTrack);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
