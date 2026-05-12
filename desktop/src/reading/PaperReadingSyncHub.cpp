#include "reading/PaperReadingSyncHub.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainterPath>
#include <QRandomGenerator>

PaperReadingSyncHub::PaperReadingSyncHub(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingSyncHub")
{
    setupUI();
    loadSettings();
}

void PaperReadingSyncHub::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Desktop", "Mobile", "Tablet", "Web"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter session...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    syncBtn_ = new QPushButton("Sync");
    syncBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(syncBtn_, &QPushButton::clicked, this, &PaperReadingSyncHub::onSync);
    toolbar->addWidget(syncBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingSyncHub::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Entries: 0 | Current: 0 | Avg Progress: 0.0%");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    layout->addStretch(1);

    setMinimumSize(720, 560);
}

void PaperReadingSyncHub::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No sync entries yet");
        return;
    }

    int w = width(), h = height();
    int halfH = h / 2;
    int quarterW = w / 2;

    drawSyncView(p, QRect(10, 10, w - 20, halfH - 20));
    drawCategoryChart(p, QRect(10, halfH, quarterW - 10, h - halfH - 10));
    drawStats(p, QRect(quarterW + 10, halfH, quarterW - 20, h - halfH - 10));
}

void PaperReadingSyncHub::drawSyncView(QPainter& p, const QRect& rect) {
    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 16, "Sync Hub");

    int contentY = rect.y() + 28;
    int contentH = rect.height() - 28;
    int maxShow = qMin(entries_.size(), 8);
    int cardH = qMin(52, (contentH - 4) / qMax(maxShow, 1));
    if (cardH < 20) cardH = 20;

    // Color map by device category
    QMap<QString, QColor> deviceColors;
    deviceColors["Desktop"] = QColor(59, 130, 246);   // #3b82f6
    deviceColors["Mobile"]  = QColor(22, 163, 74);    // #16a34a
    deviceColors["Tablet"]  = QColor(124, 58, 237);   // #7c3aed
    deviceColors["Web"]     = QColor(217, 119, 6);    // #d97706

    int shown = 0;
    for (int i = entries_.size() - 1; i >= 0 && shown < maxShow; --i) {
        const auto& e = entries_[i];
        int y = contentY + shown * (cardH + 4);

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(248, 250, 252));
        QPainterPath cardPath;
        cardPath.addRoundedRect(rect.x(), y, rect.width(), cardH, 6, 6);
        p.drawPath(cardPath);

        // Left accent bar (device color)
        QColor accent = deviceColors.value(e.device, QColor(100, 116, 139));
        p.setBrush(accent);
        QPainterPath accentPath;
        accentPath.addRoundedRect(QRectF(rect.x(), y, 5, cardH), 2.5, 2.5);
        p.drawPath(accentPath);

        // Device icon (circle with first letter)
        int iconX = rect.x() + 16;
        int iconCenterY = y + cardH / 2;
        int iconR = qMin(14, cardH / 2 - 4);
        p.setBrush(accent.lighter(140));
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(iconX + iconR, iconCenterY), iconR, iconR);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(QRect(iconX, iconCenterY - iconR, iconR * 2, iconR * 2),
                   Qt::AlignCenter, e.device.left(1));

        // Session name
        int textX = iconX + iconR * 2 + 10;
        int textW = rect.width() * 0.3;
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(QRect(textX, y + 2, textW, cardH / 2 - 2), Qt::AlignVCenter,
                   e.session.left(24));

        // Device label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(QRect(textX, y + cardH / 2, textW, cardH / 2 - 2), Qt::AlignVCenter,
                   e.device);

        // Progress bar
        int barX = textX + textW + 10;
        int barW = rect.width() * 0.35;
        int barH = 8;
        int barY = y + cardH / 2 - barH / 2;

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        QPainterPath barBg;
        barBg.addRoundedRect(QRectF(barX, barY, barW, barH), 3, 3);
        p.drawPath(barBg);

        int fillW = static_cast<int>(barW * qBound(0.0, e.progress, 1.0));
        if (fillW > 0) {
            p.setBrush(accent);
            QPainterPath barFill;
            barFill.addRoundedRect(QRectF(barX, barY, fillW, barH), 3, 3);
            p.drawPath(barFill);
        }

        // Progress percentage text
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(QRect(barX, barY - 10, barW, 10), Qt::AlignCenter,
                   QString::number(e.progress * 100, 'f', 0) + "%");

        // Synced items count
        int countX = barX + barW + 10;
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(QRect(countX, y + 2, 60, cardH / 2 - 2), Qt::AlignVCenter,
                   QString::number(e.synced) + " synced");

        // Category label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(QRect(countX, y + cardH / 2, 60, cardH / 2 - 2), Qt::AlignVCenter,
                   e.category);

        // Current indicator
        if (e.current) {
            int dotX = rect.x() + rect.width() - 18;
            int dotY = y + cardH / 2;
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(34, 197, 94));
            p.drawEllipse(QPointF(dotX, dotY), 5, 5);
            p.setPen(Qt::white);
            p.setFont(QFont("Arial", 6, QFont::Bold));
            p.drawText(QRect(dotX - 5, dotY - 5, 10, 10), Qt::AlignCenter, "C");
        }

        shown++;
    }
}

void PaperReadingSyncHub::drawCategoryChart(QPainter& p, const QRect& rect) {
    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 16, "Categories");

    QMap<QString, int> counts = categoryCounts();
    if (counts.isEmpty()) return;

    QList<QColor> sliceColors = {
        QColor(59, 130, 246), QColor(22, 163, 74), QColor(124, 58, 237),
        QColor(217, 119, 6), QColor(220, 38, 38)
    };

    int total = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        total += it.value();
    if (total == 0) return;

    // Donut chart dimensions
    int chartSize = qMin(rect.width() - 20, rect.height() - 40);
    if (chartSize < 40) chartSize = 40;
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 30 + chartSize / 2;
    int outerR = chartSize / 2;
    int innerR = outerR * 55 / 100;

    qreal startAngle = 0.0;
    int idx = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        qreal sweep = 360.0 * it.value() / total;
        QColor color = sliceColors[idx % sliceColors.size()];

        // Draw slice as filled arc between two circles
        QPainterPath slice;
        qreal sa16 = startAngle * 16;
        qreal sw16 = sweep * 16;
        slice.addEllipse(QPointF(cx, cy), outerR, outerR);
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawPie(QRect(cx - outerR, cy - outerR, outerR * 2, outerR * 2),
                  static_cast<int>(sa16), static_cast<int>(sw16));

        startAngle += sweep;
        idx++;
    }

    // Inner circle (donut hole)
    p.setBrush(Qt::white);
    p.setPen(Qt::NoPen);
    QPainterPath hole;
    hole.addEllipse(QPointF(cx, cy), innerR, innerR);
    p.drawPath(hole);

    // Center label
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 12, QFont::Bold));
    p.drawText(QRect(cx - innerR, cy - 10, innerR * 2, 20), Qt::AlignCenter,
               QString::number(total));

    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 8));
    p.drawText(QRect(cx - innerR, cy + 6, innerR * 2, 14), Qt::AlignCenter, "total");

    // Legend below chart
    int legendY = cy + outerR + 10;
    int legendX = rect.x() + 5;
    idx = 0;
    p.setFont(QFont("Arial", 8));
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        if (legendY + 14 > rect.y() + rect.height()) break;
        QColor color = sliceColors[idx % sliceColors.size()];
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawRoundedRect(legendX, legendY, 8, 8, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.drawText(legendX + 12, legendY + 8, it.key() + " (" + QString::number(it.value()) + ")");

        legendY += 14;
        idx++;
    }
}

void PaperReadingSyncHub::drawStats(QPainter& p, const QRect& rect) {
    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 16, "Statistics");

    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Entries",   QString::number(entries_.size()),             QColor(59, 130, 246)},
        {"Avg Progress",    QString::number(avgProgress() * 100, 'f', 1) + "%", QColor(22, 163, 74)},
        {"Current Sessions",QString::number(currentCount()),             QColor(124, 58, 237)},
        {"Categories",      QString::number(categoryCounts().size()),     QColor(217, 119, 6)},
        {"Total Synced",    [&](){ int t=0; for(const auto& e: entries_) t += e.synced; return QString::number(t); }(),
                                                                      QColor(220, 38, 38)}
    };

    int contentY = rect.y() + 28;
    int boxH = qMin(52, (rect.height() - 30) / qMax(stats.size(), 1));
    if (boxH < 24) boxH = 24;

    for (int i = 0; i < stats.size(); ++i) {
        int y = contentY + i * (boxH + 5);
        if (y + boxH > rect.y() + rect.height()) break;

        // Box background
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        QPainterPath boxPath;
        boxPath.addRoundedRect(QRectF(rect.x(), y, rect.width(), boxH), 6, 6);
        p.drawPath(boxPath);

        // Left accent bar
        p.setBrush(stats[i].color);
        QPainterPath accentPath;
        accentPath.addRoundedRect(QRectF(rect.x(), y, 4, boxH), 2, 2);
        p.drawPath(accentPath);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(QRect(rect.x() + 14, y + 2, rect.width() - 24, boxH * 0.55),
                   Qt::AlignVCenter, stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(QRect(rect.x() + 14, y + boxH * 0.5, rect.width() - 24, boxH * 0.45),
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperReadingSyncHub::addEntry(const SyncHubEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<SyncHubEntry> PaperReadingSyncHub::entries() const {
    return entries_;
}

int PaperReadingSyncHub::currentCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.current) ++c;
    return c;
}

qreal PaperReadingSyncHub::avgProgress() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.progress;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingSyncHub::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperReadingSyncHub::onSync() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QMap<QString, QColor> deviceColors;
    deviceColors["Desktop"] = QColor(59, 130, 246);
    deviceColors["Mobile"]  = QColor(22, 163, 74);
    deviceColors["Tablet"]  = QColor(124, 58, 237);
    deviceColors["Web"]     = QColor(217, 119, 6);

    QStringList devices = {"Desktop", "Mobile", "Tablet", "Web"};

    int selectedIdx = categoryCombo_->currentIndex();
    QString device;
    if (selectedIdx == 0) {
        device = devices[QRandomGenerator::global()->bounded(devices.size())];
    } else {
        device = devices[qMin(selectedIdx - 1, devices.size() - 1)];
    }

    SyncHubEntry e;
    e.id = entries_.size() + 1;
    e.session = text;
    e.category = device;
    e.device = device;
    e.progress = QRandomGenerator::global()->bounded(101) / 100.0;
    e.synced = QRandomGenerator::global()->bounded(51);
    e.current = QRandomGenerator::global()->bounded(3) == 0;
    e.color = deviceColors.value(device, QColor(100, 116, 139));

    addEntry(e);
    emit sessionSynced(e.id, e.progress);

    inputField_->clear();
}

void PaperReadingSyncHub::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperReadingSyncHub::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Entries: 0 | Current: 0 | Avg Progress: 0.0%");
        return;
    }
    infoLabel_->setText(QString("Entries: %1 | Current: %2 | Avg Progress: %3%")
        .arg(entries_.size())
        .arg(currentCount())
        .arg(QString::number(avgProgress() * 100, 'f', 1)));
}

void PaperReadingSyncHub::loadSettings() {
    settings_.beginGroup("ReadingSyncHub");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        SyncHubEntry e;
        e.id       = settings_.value("id").toInt();
        e.session  = settings_.value("session").toString();
        e.category = settings_.value("category").toString();
        e.device   = settings_.value("device").toString();
        e.progress = settings_.value("progress").toDouble();
        e.synced   = settings_.value("synced").toInt();
        e.current  = settings_.value("current").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperReadingSyncHub::saveSettings() {
    settings_.beginGroup("ReadingSyncHub");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",       entries_[i].id);
        settings_.setValue("session",  entries_[i].session);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("device",   entries_[i].device);
        settings_.setValue("progress", entries_[i].progress);
        settings_.setValue("synced",   entries_[i].synced);
        settings_.setValue("current",  entries_[i].current);
        settings_.setValue("color",    entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
