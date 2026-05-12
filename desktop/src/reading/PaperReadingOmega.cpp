#include "reading/PaperReadingOmega.hpp"
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>
#include <QPainterPath>
#include <QFontMetrics>

PaperReadingOmega::PaperReadingOmega(QWidget* parent)
    : QWidget(parent), settings_("PaperCrawler", "ReadingOmega") {
    setupUI();
    loadSettings();
}

void PaperReadingOmega::setupUI() {
    auto* topBar = new QHBoxLayout();

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Science", "Technology", "Philosophy", "Literature"});
    categoryCombo_->setStyleSheet(
        "QComboBox{border:1px solid #ccc;border-radius:4px;padding:4px 8px;min-width:120px;}"
        "QComboBox::drop-down{border:none;}");

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Enter title...");
    inputField_->setStyleSheet(
        "QLineEdit{border:1px solid #ccc;border-radius:4px;padding:4px 8px;}");

    trackBtn_ = new QPushButton("Track", this);
    trackBtn_->setStyleSheet(
        "QPushButton{background:#3b82f6;color:#fff;border:none;border-radius:4px;"
        "padding:6px 16px;font-weight:bold;}"
        "QPushButton:hover{background:#2563eb;}");

    clearBtn_ = new QPushButton("Clear", this);
    clearBtn_->setStyleSheet(
        "QPushButton{background:#dc2626;color:#fff;border:none;border-radius:4px;"
        "padding:6px 16px;font-weight:bold;}"
        "QPushButton:hover{background:#b91c1c;}");

    topBar->addWidget(categoryCombo_);
    topBar->addWidget(inputField_, 1);
    topBar->addWidget(trackBtn_);
    topBar->addWidget(clearBtn_);

    infoLabel_ = new QLabel("Entries: 0 | Finished: 0% | Avg Completion: 0.0%", this);
    infoLabel_->setStyleSheet("QLabel{color:#555;font-size:12px;padding:4px 0;}");

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(topBar);
    mainLayout->addWidget(infoLabel_);
    mainLayout->addStretch(1);

    setMinimumSize(600, 500);

    connect(trackBtn_, &QPushButton::clicked, this, &PaperReadingOmega::onTrack);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingOmega::onClear);
}

void PaperReadingOmega::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int toolbarH = 80;
    int w = width();
    int h = height() - toolbarH;
    if (h < 100) h = 100;

    QRect omegaRect(0, toolbarH, w, h / 2);
    QRect chartRect(0, toolbarH + h / 2, w / 2, h / 2);
    QRect statsRect(w / 2, toolbarH + h / 2, w / 2, h / 2);

    drawOmegaView(p, omegaRect);
    drawCategoryChart(p, chartRect);
    drawStats(p, statsRect);
}

void PaperReadingOmega::drawOmegaView(QPainter& p, const QRect& rect) {
    p.save();

    QPainterPath bgPath;
    bgPath.addRoundedRect(rect.adjusted(4, 4, -4, -4), 8, 8);
    p.fillPath(bgPath, QColor(248, 250, 252));
    p.strokePath(bgPath, QPen(QColor(226, 232, 240), 1));

    QFont titleFont("Segoe UI", 10, QFont::Bold);
    p.setFont(titleFont);
    p.setPen(QColor(51, 65, 85));
    p.drawText(rect.adjusted(14, 10, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Omega Reading Progress");

    if (entries_.isEmpty()) {
        QFont hintFont("Segoe UI", 11);
        p.setFont(hintFont);
        p.setPen(QColor(148, 163, 184));
        p.drawText(rect, Qt::AlignCenter, "No entries tracked yet");
        p.restore();
        return;
    }

    QMap<QString, QColor> phaseColors = {
        {"phase1", QColor(0x3b82f6)},
        {"phase2", QColor(0x16a34a)},
        {"phase3", QColor(0x7c3aed)},
        {"phase4", QColor(0xd97706)}
    };

    int cols = qMax(1, qMin(4, entries_.size()));
    int rows = (entries_.size() + cols - 1) / cols;
    int cellW = (rect.width() - 28) / cols;
    int cellH = qMin(80, (rect.height() - 50) / qMax(1, rows));
    int startX = rect.x() + 14;
    int startY = rect.y() + 34;

    QFont entryFont("Segoe UI", 8);
    QFont phaseFont("Segoe UI", 7);

    for (int i = 0; i < entries_.size(); ++i) {
        const auto& entry = entries_[i];
        int col = i % cols;
        int row = i / cols;
        int cx = startX + col * cellW + cellW / 2;
        int cy = startY + row * cellH + cellH / 2 - 4;
        int radius = qMin(cellW, cellH) / 2 - 12;
        if (radius < 12) radius = 12;

        QRectF arcRect(cx - radius, cy - radius, radius * 2, radius * 2);

        QColor phaseColor = phaseColors.value(entry.phase.toLower(), QColor(0x3b82f6));

        int startAngle = 90 * 16;
        int fullSpan = 360 * 16;
        int completedSpan = -static_cast<int>(entry.completion / 100.0 * fullSpan);

        QRadialGradient grad(arcRect.center(), radius);
        grad.setColorAt(0.0, QColor(241, 245, 249));
        grad.setColorAt(1.0, QColor(226, 232, 240));
        p.setPen(Qt::NoPen);
        p.setBrush(grad);
        p.drawEllipse(arcRect);

        p.setPen(QPen(phaseColor, 3, Qt::SolidLine, Qt::RoundCap));
        p.setBrush(Qt::NoBrush);
        if (entry.completion > 0) {
            p.drawArc(arcRect, startAngle, completedSpan);
        }

        p.setFont(entryFont);
        p.setPen(QColor(30, 41, 59));
        QString displayTitle = entry.title;
        if (displayTitle.length() > 12) displayTitle = displayTitle.left(11) + "...";
        QFontMetrics fm(entryFont);
        QRect textRect(cx - cellW / 2 + 4, cy + radius + 2, cellW - 8, 16);
        p.drawText(textRect, Qt::AlignCenter, fm.elidedText(displayTitle, Qt::ElideRight, cellW - 8));

        p.setFont(phaseFont);
        p.setPen(phaseColor);
        p.drawText(textRect.adjusted(0, 14, 0, 14), Qt::AlignCenter, entry.phase);

        if (entry.finished) {
            p.setPen(QPen(QColor(34, 197, 94), 2));
            int checkSize = 6;
            int checkX = cx + radius - 6;
            int checkY = cy - radius + 6;
            QPainterPath checkPath;
            checkPath.moveTo(checkX - checkSize, checkY);
            checkPath.lineTo(checkX - checkSize / 3, checkY + checkSize / 2);
            checkPath.lineTo(checkX + checkSize, checkY - checkSize / 2);
            p.drawPath(checkPath);
        }
    }

    p.restore();
}

void PaperReadingOmega::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.save();

    QPainterPath bgPath;
    bgPath.addRoundedRect(rect.adjusted(4, 4, -4, -4), 8, 8);
    p.fillPath(bgPath, QColor(248, 250, 252));
    p.strokePath(bgPath, QPen(QColor(226, 232, 240), 1));

    QFont titleFont("Segoe UI", 10, QFont::Bold);
    p.setFont(titleFont);
    p.setPen(QColor(51, 65, 85));
    p.drawText(rect.adjusted(14, 10, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Categories");

    auto counts = categoryCounts();
    if (counts.isEmpty()) {
        QFont hintFont("Segoe UI", 10);
        p.setFont(hintFont);
        p.setPen(QColor(148, 163, 184));
        p.drawText(rect, Qt::AlignCenter, "No data");
        p.restore();
        return;
    }

    QList<QColor> sliceColors = {
        QColor(0x3b82f6), QColor(0x16a34a), QColor(0x7c3aed),
        QColor(0xd97706), QColor(0xdc2626), QColor(0x0891b2)
    };

    int total = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        total += it.value();
    }

    int chartSize = qMin(rect.width(), rect.height()) - 80;
    if (chartSize < 60) chartSize = 60;
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + rect.height() / 2 + 8;
    int outerR = chartSize / 2;
    int innerR = outerR * 0.55;

    QRectF outerRect(cx - outerR, cy - outerR, outerR * 2, outerR * 2);

    int colorIdx = 0;
    qreal startAngle = 0.0;
    QFont labelFont("Segoe UI", 8);
    p.setFont(labelFont);

    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        qreal sliceAngle = 360.0 * it.value() / total;
        QColor color = sliceColors[colorIdx % sliceColors.size()];

        QPainterPath slicePath;
        slicePath.moveTo(cx + innerR * qCos(qDegreesToRadians(startAngle)),
                         cy - innerR * qSin(qDegreesToRadians(startAngle)));
        slicePath.arcTo(outerRect, startAngle, sliceAngle);
        qreal endAngle = startAngle + sliceAngle;
        slicePath.lineTo(cx + innerR * qCos(qDegreesToRadians(endAngle)),
                         cy - innerR * qSin(qDegreesToRadians(endAngle)));
        slicePath.arcTo(outerRect.adjusted(outerR * 2 - innerR * 2, outerR * 2 - innerR * 2,
                                            -(outerR * 2 - innerR * 2), -(outerR * 2 - innerR * 2)),
                        startAngle + sliceAngle, -sliceAngle);
        slicePath.closeSubpath();

        QRadialGradient grad(QPointF(cx, cy), outerR);
        grad.setColorAt(0.6, color);
        grad.setColorAt(1.0, color.darker(115));
        p.setPen(QPen(QColor(255, 255, 255), 2));
        p.setBrush(grad);

        int start16 = static_cast<int>(startAngle * 16);
        int span16 = static_cast<int>(sliceAngle * 16);

        p.drawPie(outerRect, -start16 + 90 * 16, -span16);

        QPainterPath innerPath;
        innerPath.addEllipse(QPointF(cx, cy), innerR, innerR);
        p.fillPath(innerPath, QColor(248, 250, 252));
        p.drawEllipse(QPointF(cx, cy), innerR, innerR);

        qreal midAngle = startAngle + sliceAngle / 2.0;
        int labelR = outerR + 16;
        qreal labelX = cx + labelR * qCos(qDegreesToRadians(midAngle));
        qreal labelY = cy - labelR * qSin(qDegreesToRadians(midAngle));
        p.setPen(color);
        p.drawText(QRectF(labelX - 50, labelY - 8, 100, 16), Qt::AlignCenter,
                   QString("%1 (%2)").arg(it.key()).arg(it.value()));

        startAngle += sliceAngle;
        colorIdx++;
    }

    QFont centerFont("Segoe UI", 11, QFont::Bold);
    p.setFont(centerFont);
    p.setPen(QColor(30, 41, 59));
    p.drawText(QRectF(cx - 30, cy - 12, 60, 24), Qt::AlignCenter, QString::number(total));

    p.restore();
}

void PaperReadingOmega::drawStats(QPainter& p, const QRect& rect) {
    p.save();

    QPainterPath bgPath;
    bgPath.addRoundedRect(rect.adjusted(4, 4, -4, -4), 8, 8);
    p.fillPath(bgPath, QColor(248, 250, 252));
    p.strokePath(bgPath, QPen(QColor(226, 232, 240), 1));

    QFont titleFont("Segoe UI", 10, QFont::Bold);
    p.setFont(titleFont);
    p.setPen(QColor(51, 65, 85));
    p.drawText(rect.adjusted(14, 10, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Statistics");

    int startY = rect.y() + 36;
    int leftMargin = rect.x() + 20;
    int lineH = 28;
    int barMaxW = rect.width() - 120;

    struct StatRow {
        QString label;
        QString value;
        qreal ratio;
        QColor color;
    };

    QList<StatRow> rows;

    rows.append({"Total Entries", QString::number(entries_.size()), 1.0, QColor(0x3b82f6)});

    int finished = finishedCount();
    qreal finishedPct = entries_.isEmpty() ? 0.0 : 100.0 * finished / entries_.size();
    rows.append({"Finished", QString("%1 (%2%)").arg(finished).arg(qRound(finishedPct)),
                 finishedPct / 100.0, QColor(0x16a34a)});

    qreal avg = avgCompletion();
    rows.append({"Avg Completion", QString("%1%").arg(qRound(avg)),
                 avg / 100.0, QColor(0x7c3aed)});

    int unfinished = entries_.size() - finished;
    qreal unfinishedPct = entries_.isEmpty() ? 0.0 : 100.0 * unfinished / entries_.size();
    rows.append({"In Progress", QString("%1 (%2%)").arg(unfinished).arg(qRound(unfinishedPct)),
                 unfinishedPct / 100.0, QColor(0xd97706)});

    QFont labelFont("Segoe UI", 9);
    QFont valueFont("Segoe UI", 9, QFont::Bold);

    for (int i = 0; i < rows.size(); ++i) {
        const auto& row = rows[i];
        int y = startY + i * lineH;

        p.setFont(labelFont);
        p.setPen(QColor(71, 85, 105));
        p.drawText(QRect(leftMargin, y, 100, 20), Qt::AlignLeft | Qt::AlignVCenter, row.label);

        int barX = leftMargin + 105;
        int barH = 12;
        int barW = static_cast<int>(barMaxW * qBound(0.0, row.ratio, 1.0));

        QPainterPath barBg;
        barBg.addRoundedRect(QRectF(barX, y + 4, barMaxW, barH), barH / 2, barH / 2);
        p.fillPath(barBg, QColor(226, 232, 240));

        if (barW > 0) {
            QPainterPath barFill;
            barFill.addRoundedRect(QRectF(barX, y + 4, qMax(barW, barH), barH), barH / 2, barH / 2);
            p.fillPath(barFill, row.color);
        }

        p.setFont(valueFont);
        p.setPen(row.color);
        p.drawText(QRect(barX + barMaxW + 6, y, 80, 20), Qt::AlignLeft | Qt::AlignVCenter, row.value);
    }

    p.restore();
}

void PaperReadingOmega::onTrack() {
    QString title = inputField_->text().trimmed();
    if (title.isEmpty()) return;

    QString category = categoryCombo_->currentText();
    if (category == "All") category = "Science";

    QStringList phases = {"phase1", "phase2", "phase3", "phase4"};
    QMap<QString, QColor> phaseColors = {
        {"phase1", QColor(0x3b82f6)},
        {"phase2", QColor(0x16a34a)},
        {"phase3", QColor(0x7c3aed)},
        {"phase4", QColor(0xd97706)}
    };

    QString phase = phases[QRandomGenerator::global()->bounded(phases.size())];
    qreal completion = QRandomGenerator::global()->generateDouble() * 100.0;
    int chapters = QRandomGenerator::global()->bounded(1, 25);
    bool finished = completion >= 95.0;

    OmegaEntry entry;
    entry.id = entries_.isEmpty() ? 1 : entries_.last().id + 1;
    entry.title = title;
    entry.category = category;
    entry.phase = phase;
    entry.completion = completion;
    entry.chapters = chapters;
    entry.finished = finished;
    entry.color = phaseColors.value(phase, QColor(0x3b82f6));

    entries_.append(entry);
    inputField_->clear();

    updateInfo();
    saveSettings();
    update();
    emit phaseCompleted(entry.id, entry.completion);
}

void PaperReadingOmega::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperReadingOmega::addEntry(const OmegaEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<OmegaEntry> PaperReadingOmega::entries() const {
    return entries_;
}

int PaperReadingOmega::finishedCount() const {
    int count = 0;
    for (const auto& e : entries_) {
        if (e.finished) ++count;
    }
    return count;
}

qreal PaperReadingOmega::avgCompletion() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) {
        sum += e.completion;
    }
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingOmega::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) {
        counts[e.category]++;
    }
    return counts;
}

void PaperReadingOmega::updateInfo() {
    int total = entries_.size();
    int finished = finishedCount();
    qreal finishedPct = total > 0 ? 100.0 * finished / total : 0.0;
    qreal avg = avgCompletion();

    infoLabel_->setText(
        QString("Entries: %1 | Finished: %2% | Avg Completion: %3%")
            .arg(total)
            .arg(qRound(finishedPct))
            .arg(qRound(avg * 10.0) / 10.0));
}

void PaperReadingOmega::loadSettings() {
    int size = settings_.beginReadArray("entries");
    entries_.clear();
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        OmegaEntry e;
        e.id = settings_.value("id", i + 1).toInt();
        e.title = settings_.value("title").toString();
        e.category = settings_.value("category", "Science").toString();
        e.phase = settings_.value("phase", "phase1").toString();
        e.completion = settings_.value("completion", 0.0).toReal();
        e.chapters = settings_.value("chapters", 0).toInt();
        e.finished = settings_.value("finished", false).toBool();
        e.color = QColor(settings_.value("color", "#3b82f6").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingOmega::saveSettings() {
    settings_.beginWriteArray("entries", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& e = entries_[i];
        settings_.setValue("id", e.id);
        settings_.setValue("title", e.title);
        settings_.setValue("category", e.category);
        settings_.setValue("phase", e.phase);
        settings_.setValue("completion", e.completion);
        settings_.setValue("chapters", e.chapters);
        settings_.setValue("finished", e.finished);
        settings_.setValue("color", e.color.name());
    }
    settings_.endArray();
}
