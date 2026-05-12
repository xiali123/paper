#include "reading/PaperGoalSummit.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>

PaperGoalSummit::PaperGoalSummit(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "GoalSummit")
{
    setupUI();
    loadSettings();
}

void PaperGoalSummit::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Daily", "Weekly", "Monthly", "Annual"});
    toolbar->addWidget(categoryCombo_, 1);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter goal...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 2);
    trackBtn_ = new QPushButton("Track");
    trackBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(trackBtn_, &QPushButton::clicked, this, &PaperGoalSummit::onTrack);
    toolbar->addWidget(trackBtn_);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperGoalSummit::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    infoLabel_ = new QLabel("Track your reading goals");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperGoalSummit::addEntry(const GoalSummitEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    if (entry.achieved) {
        emit goalReached(entry.id, entry.progress);
    }
    update();
}

QList<GoalSummitEntry> PaperGoalSummit::entries() const { return entries_; }

int PaperGoalSummit::achievedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.achieved) c++;
    return c;
}

qreal PaperGoalSummit::avgProgress() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.progress;
    return sum / entries_.size();
}

QMap<QString, int> PaperGoalSummit::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperGoalSummit::onTrack() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"Daily", "Weekly", "Monthly", "Annual"};
    QStringList milestones = {"Started", "In Progress", "Halfway", "Nearly Done", "Complete"};
    QMap<QString, QColor> catColors = {
        {"Daily", QColor(59, 130, 246)},
        {"Weekly", QColor(22, 163, 74)},
        {"Monthly", QColor(124, 58, 237)},
        {"Annual", QColor(217, 119, 6)}
    };
    int cIdx = categoryCombo_->currentIndex();
    int count = 2 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        GoalSummitEntry e;
        e.id = entries_.size() + 1;
        e.goal = text.left(20) + " #" + QString::number(e.id);
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.milestone = milestones[QRandomGenerator::global()->bounded(milestones.size())];
        e.progress = QRandomGenerator::global()->bounded(101) / 100.0;
        e.daysLeft = QRandomGenerator::global()->bounded(366);
        e.achieved = e.progress >= 1.0;
        e.color = catColors.value(e.category, QColor(59, 130, 246));
        if (e.achieved) e.daysLeft = 0;
        addEntry(e);
    }
    inputField_->clear();
}

void PaperGoalSummit::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Track your reading goals");
    update();
}

void PaperGoalSummit::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Track your reading goals");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Goal Summit");
    int w = width(), h = height();
    drawSummitView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperGoalSummit::drawSummitView(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    if (show == 0) return;
    int slotW = (rect.width() - 20) / qMax(show, 1);
    int baseY = rect.y() + rect.height() - 20;
    int maxPeakH = rect.height() - 50;

    // ground line
    p.setPen(QPen(QColor(203, 213, 225), 1));
    p.drawLine(rect.x(), baseY, rect.x() + rect.width(), baseY);

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int cx = rect.x() + 10 + i * slotW + slotW / 2;
        int peakH = static_cast<int>(e.progress * maxPeakH);
        int peakY = baseY - peakH;
        int halfW = qMax(slotW / 2 - 6, 12);

        // mountain shape using QPainterPath
        QPainterPath mountain;
        mountain.moveTo(cx - halfW, baseY);
        mountain.lineTo(cx - halfW / 3, peakY + peakH * 0.3);
        mountain.lineTo(cx, peakY);
        mountain.lineTo(cx + halfW / 3, peakY + peakH * 0.25);
        mountain.lineTo(cx + halfW, baseY);
        mountain.closeSubpath();

        // fill mountain with gradient based on progress
        QColor fillBase = e.color.lighter(160);
        QColor fillTop = e.achieved ? QColor(16, 185, 129) : e.color;
        QLinearGradient grad(cx, baseY, cx, peakY);
        grad.setColorAt(0.0, fillBase);
        grad.setColorAt(1.0, fillTop);
        p.setPen(Qt::NoPen);
        p.setBrush(grad);
        p.drawPath(mountain);

        // progress fill overlay (snow cap effect for high progress)
        if (e.progress > 0.6) {
            qreal snowRatio = (e.progress - 0.6) / 0.4;
            int snowH = static_cast<int>(snowRatio * peakH * 0.35);
            QPainterPath snow;
            snow.moveTo(cx - halfW * 0.3, peakY + snowH);
            snow.lineTo(cx - halfW * 0.1, peakY + 2);
            snow.lineTo(cx, peakY);
            snow.lineTo(cx + halfW * 0.1, peakY + 3);
            snow.lineTo(cx + halfW * 0.3, peakY + snowH);
            snow.closeSubpath();
            p.setBrush(QColor(255, 255, 255, 180));
            p.drawPath(snow);
        }

        // mountain outline
        p.setPen(QPen(e.color.darker(120), 1.2));
        p.setBrush(Qt::NoBrush);
        p.drawPath(mountain);

        // achieved flag
        if (e.achieved) {
            int flagPoleTop = peakY - 18;
            p.setPen(QPen(e.color, 1.5));
            p.drawLine(cx, peakY, cx, flagPoleTop);
            QPainterPath flag;
            flag.moveTo(cx, flagPoleTop);
            flag.lineTo(cx + 12, flagPoleTop + 5);
            flag.lineTo(cx, flagPoleTop + 10);
            flag.closeSubpath();
            p.setBrush(QColor(16, 185, 129));
            p.setPen(Qt::NoPen);
            p.drawPath(flag);
        }

        // peak label: goal name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        QString label = e.goal.left(8);
        int labelY = e.achieved ? peakY - 28 : peakY - 12;
        p.drawText(cx - slotW / 2 + 2, labelY, slotW - 4, 12, Qt::AlignCenter, label);

        // days countdown
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        if (e.achieved) {
            p.drawText(cx - slotW / 2 + 2, baseY + 4, slotW - 4, 12, Qt::AlignCenter, "Done!");
        } else {
            p.drawText(cx - slotW / 2 + 2, baseY + 4, slotW - 4, 12, Qt::AlignCenter,
                       QString::number(e.daysLeft) + "d left");
        }

        // progress percentage on the slope
        p.setPen(e.color.darker(110));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        int pctY = baseY - peakH / 2;
        p.drawText(cx - slotW / 2 + 2, pctY - 6, slotW - 4, 14, Qt::AlignCenter,
                   QString::number(e.progress * 100, 'f', 0) + "%");
    }
}

void PaperGoalSummit::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList categories = {"Daily", "Weekly", "Monthly", "Annual"};
    QMap<QString, QColor> catColors = {
        {"Daily", QColor(59, 130, 246)},
        {"Weekly", QColor(22, 163, 74)},
        {"Monthly", QColor(124, 58, 237)},
        {"Annual", QColor(217, 119, 6)}
    };

    int total = 0;
    for (const auto& c : counts) total += c;
    if (total == 0) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.adjusted(0, 20, 0, 0), Qt::AlignCenter, "No data");
        return;
    }

    // donut chart
    int donutSize = qMin(rect.width(), rect.height() - 50);
    int donutX = rect.x() + (rect.width() - donutSize) / 2;
    int donutY = rect.y() + 22;
    int outerR = donutSize / 2;
    int innerR = outerR * 55 / 100;
    QRectF outerRect(donutX, donutY, donutSize, donutSize);
    QRectF innerRect(donutX + (outerR - innerR), donutY + (outerR - innerR), innerR * 2, innerR * 2);

    qreal startAngle = 0.0;
    for (int i = 0; i < categories.size(); ++i) {
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        if (count == 0) continue;
        qreal sweep = (static_cast<qreal>(count) / total) * 360.0;
        QPainterPath slice;
        slice.moveTo(outerRect.center());
        slice.arcTo(outerRect, startAngle, sweep);
        slice.closeSubpath();
        p.setPen(Qt::NoPen);
        p.setBrush(catColors.value(categories[i], QColor(148, 163, 184)));
        p.drawPath(slice);

        // label on the slice
        qreal midAngle = (startAngle + sweep / 2.0) * M_PI / 180.0;
        int labelR = outerR - 12;
        qreal lx = outerRect.center().x() + labelR * qCos(midAngle);
        qreal ly = outerRect.center().y() - labelR * qSin(midAngle);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(QRectF(lx - 20, ly - 8, 40, 16), Qt::AlignCenter,
                   categories[i].left(3) + "\n" + QString::number(count));

        startAngle += sweep;
    }

    // inner circle (donut hole)
    p.setBrush(Qt::white);
    p.setPen(Qt::NoPen);
    p.drawEllipse(innerRect);

    // center text
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 14, QFont::Bold));
    p.drawText(innerRect, Qt::AlignCenter, QString::number(total));
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 7));
    QRectF labelRect = innerRect.adjusted(0, 14, 0, 14);
    p.drawText(labelRect, Qt::AlignCenter, "goals");
}

void PaperGoalSummit::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Goals", QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Achieved", QString::number(achievedCount()), QColor(16, 185, 129)},
        {"Avg Progress", QString::number(avgProgress() * 100, 'f', 0) + "%", QColor(245, 158, 11)},
        {"Categories", QString::number(categoryCounts().size()), QColor(124, 58, 237)}
    };
    int boxH = qMin(42, (rect.height() - 10) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 22, Qt::AlignVCenter, stats[i].value);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 26, rect.width() - 20, 14, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperGoalSummit::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Track your reading goals"); return; }
    infoLabel_->setText(QString("%1 goals | %2 achieved | %3% progress")
        .arg(entries_.size()).arg(achievedCount()).arg(avgProgress() * 100, 0, 'f', 0));
}

void PaperGoalSummit::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        GoalSummitEntry e;
        e.id = settings_.value("id").toInt();
        e.goal = settings_.value("goal").toString();
        e.category = settings_.value("category").toString();
        e.milestone = settings_.value("milestone").toString();
        e.progress = settings_.value("progress").toDouble();
        e.daysLeft = settings_.value("daysLeft").toInt();
        e.achieved = settings_.value("achieved").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperGoalSummit::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("goal", entries_[i].goal);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("milestone", entries_[i].milestone);
        settings_.setValue("progress", entries_[i].progress);
        settings_.setValue("daysLeft", entries_[i].daysLeft);
        settings_.setValue("achieved", entries_[i].achieved);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
