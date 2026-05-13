#include "reading/PaperReadingHorizon2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>
#include <QtMath>

PaperReadingHorizon2::PaperReadingHorizon2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingHorizon2")
{
    setupUI();
    loadSettings();
}

void PaperReadingHorizon2::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Science", "Technology", "Engineering", "Arts", "Math"});
    toolbar->addWidget(categoryCombo_, 1);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter horizon label...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 2);
    navigateBtn_ = new QPushButton("Navigate");
    navigateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(navigateBtn_, &QPushButton::clicked, this, &PaperReadingHorizon2::onNavigate);
    toolbar->addWidget(navigateBtn_);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingHorizon2::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    infoLabel_ = new QLabel("Navigate your reading horizons");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(640, 520);
}

void PaperReadingHorizon2::addEntry(const ReadingHorizon2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    if (entry.reached) {
        emit horizonReached(entry.id, entry.distance);
    }
    update();
}

QList<ReadingHorizon2Entry> PaperReadingHorizon2::entries() const { return entries_; }

int PaperReadingHorizon2::reachedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.reached) c++;
    return c;
}

qreal PaperReadingHorizon2::avgDistance() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.distance;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingHorizon2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperReadingHorizon2::onNavigate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList horizons = {"Data Science Frontier", "AI Frontier", "Quantum Frontier"};
    QStringList directions = {"North", "South", "East", "West"};
    QStringList categories = {"Science", "Technology", "Engineering", "Arts", "Math"};
    QList<QColor> palette = {
        QColor(59, 130, 246),   // #3b82f6
        QColor(22, 163, 74),    // #16a34a
        QColor(217, 119, 6),    // #d97706
        QColor(220, 38, 38),    // #dc2626
        QColor(124, 58, 237)    // #7c3aed
    };

    int cIdx = categoryCombo_->currentIndex();
    int count = 2 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        ReadingHorizon2Entry e;
        e.id = entries_.size() + 1;
        e.horizon = horizons[QRandomGenerator::global()->bounded(horizons.size())];
        e.direction = directions[QRandomGenerator::global()->bounded(directions.size())];
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())]
                                : categories[cIdx - 1];
        e.distance = QRandomGenerator::global()->bounded(10001) / 100.0;
        e.waypoints = QRandomGenerator::global()->bounded(12) + 1;
        e.reached = e.distance >= 90.0;
        e.color = palette[QRandomGenerator::global()->bounded(palette.size())];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperReadingHorizon2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Navigate your reading horizons");
    update();
}

void PaperReadingHorizon2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Navigate your reading horizons");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Horizon");

    int w = width(), h = height();
    drawHorizonView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReadingHorizon2::drawHorizonView(QPainter& p, const QRect& rect) {
    int show = qMin(8, entries_.size());
    if (show == 0) return;

    // --- gradient sky ---
    QLinearGradient skyGrad(rect.topLeft(), rect.bottomLeft());
    skyGrad.setColorAt(0.0, QColor(135, 206, 250));   // light sky blue
    skyGrad.setColorAt(0.5, QColor(200, 225, 255));   // pale horizon
    skyGrad.setColorAt(1.0, QColor(240, 248, 255));   // snow white bottom
    p.setPen(Qt::NoPen);
    p.setBrush(skyGrad);
    p.drawRoundedRect(rect, 8, 8);

    // --- vanishing point ---
    qreal vanishX = rect.x() + rect.width() / 2.0;
    qreal vanishY = rect.y() + 30;

    // --- ground / horizon line ---
    qreal groundY = rect.y() + rect.height() - 16;
    QLinearGradient groundGrad(rect.x(), groundY - 30, rect.x(), groundY);
    groundGrad.setColorAt(0.0, QColor(210, 230, 210));
    groundGrad.setColorAt(1.0, QColor(170, 200, 170));
    p.setBrush(groundGrad);
    p.setPen(Qt::NoPen);
    p.drawRect(rect.x(), static_cast<int>(groundY) - 30, rect.width(), 30);
    p.setPen(QPen(QColor(120, 160, 120), 1));
    p.drawLine(rect.x(), static_cast<int>(groundY), rect.x() + rect.width(), static_cast<int>(groundY));

    // --- perspective guide lines (subtle) ---
    p.setPen(QPen(QColor(180, 200, 220, 80), 0.5, Qt::DashLine));
    p.drawLine(QPointF(vanishX, vanishY), QPointF(rect.x(), groundY));
    p.drawLine(QPointF(vanishX, vanishY), QPointF(rect.x() + rect.width(), groundY));

    // --- horizon cards ---
    qreal slotW = (rect.width() - 40.0) / show;
    qreal maxDist = 100.0;

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        qreal centerX = rect.x() + 20 + i * slotW + slotW / 2.0;

        // perspective scaling: farther = closer to vanishing point
        qreal t = e.distance / maxDist;  // 0..1 where 1 = farthest
        qreal scale = 1.0 - t * 0.55;   // near=1.0, far=0.45
        qreal cardW = slotW * 0.8 * scale;
        qreal cardH = 56 * scale;

        // y position interpolated between ground and vanishing point
        qreal cardY = vanishY + (groundY - vanishY - cardH) * (1.0 - t * 0.65);

        QRectF cardRect(centerX - cardW / 2.0, cardY, cardW, cardH);

        // card shadow
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 25));
        p.drawRoundedRect(cardRect.translated(2, 2), 5, 5);

        // card body gradient
        QLinearGradient cardGrad(cardRect.topLeft(), cardRect.bottomLeft());
        cardGrad.setColorAt(0.0, e.color.lighter(140));
        cardGrad.setColorAt(1.0, e.color);
        p.setBrush(cardGrad);
        p.setPen(QPen(e.color.darker(130), 1));
        p.drawRoundedRect(cardRect, 5, 5);

        // horizon name inside card
        p.setPen(Qt::white);
        QFont cardFont("Arial", qMax(6, static_cast<int>(8 * scale)), QFont::Bold);
        p.setFont(cardFont);
        QString name = e.horizon;
        if (cardW < 60) name = name.left(8);
        p.drawText(cardRect.adjusted(3, 2, -3, -cardH * 0.35), Qt::AlignCenter, name);

        // direction label
        p.setFont(QFont("Arial", qMax(5, static_cast<int>(6 * scale))));
        p.setPen(QColor(255, 255, 255, 200));
        p.drawText(cardRect.adjusted(3, cardH * 0.45, -3, 0), Qt::AlignCenter, e.direction);

        // --- distance progress bar beneath card ---
        qreal barY = cardY + cardH + 3;
        qreal barW = cardW;
        qreal barH = 4 * scale;
        QRectF barBg(centerX - barW / 2.0, barY, barW, barH);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(220, 230, 240));
        p.drawRoundedRect(barBg, 2, 2);

        qreal filledW = barW * qBound(0.0, e.distance / maxDist, 1.0);
        QRectF barFill(centerX - barW / 2.0, barY, filledW, barH);
        p.setBrush(e.reached ? QColor(16, 185, 129) : e.color);
        p.drawRoundedRect(barFill, 2, 2);

        // --- waypoint markers along the bar ---
        if (e.waypoints > 0 && barW > 20) {
            for (int wp = 0; wp <= e.waypoints; ++wp) {
                qreal wpFrac = static_cast<qreal>(wp) / e.waypoints;
                qreal wpX = centerX - barW / 2.0 + barW * wpFrac;
                qreal wpY2 = barY + barH / 2.0;
                qreal dotR = 2.0 * scale;
                p.setBrush(wpFrac <= e.distance / maxDist ? QColor(255, 255, 255) : QColor(180, 195, 210));
                p.setPen(Qt::NoPen);
                p.drawEllipse(QPointF(wpX, wpY2), dotR, dotR);
            }
        }

        // --- reached flag ---
        if (e.reached) {
            qreal flagX = centerX;
            qreal flagBase = cardY - 2;
            qreal flagTop = flagBase - 16 * scale;
            p.setPen(QPen(e.color.darker(140), 1.5));
            p.drawLine(QPointF(flagX, flagBase), QPointF(flagX, flagTop));
            QPainterPath flag;
            flag.moveTo(flagX, flagTop);
            flag.lineTo(flagX + 10 * scale, flagTop + 5 * scale);
            flag.lineTo(flagX, flagTop + 10 * scale);
            flag.closeSubpath();
            p.setBrush(QColor(16, 185, 129));
            p.setPen(Qt::NoPen);
            p.drawPath(flag);
        }

        // --- distance text below bar ---
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", qMax(5, static_cast<int>(7 * scale))));
        QString distLabel = QString::number(e.distance, 'f', 0) + "km";
        p.drawText(QRectF(centerX - slotW / 2.0, barY + barH + 1, slotW, 12),
                   Qt::AlignCenter, distLabel);
    }
}

void PaperReadingHorizon2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList categories = {"Science", "Technology", "Engineering", "Arts", "Math"};
    QMap<QString, QColor> catColors = {
        {"Science",     QColor(59, 130, 246)},
        {"Technology",  QColor(22, 163, 74)},
        {"Engineering", QColor(217, 119, 6)},
        {"Arts",        QColor(220, 38, 38)},
        {"Math",        QColor(124, 58, 237)}
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

    qreal startAngle = 90.0; // start from top
    for (int i = 0; i < categories.size(); ++i) {
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        if (count == 0) continue;
        qreal sweep = (static_cast<qreal>(count) / total) * 360.0;

        // draw arc segment using QPainterPath
        QPainterPath slice;
        slice.moveTo(outerRect.center());
        slice.arcTo(outerRect, startAngle, sweep);
        slice.closeSubpath();
        p.setPen(QPen(Qt::white, 2));
        p.setBrush(catColors.value(categories[i], QColor(148, 163, 184)));
        p.drawPath(slice);

        // label on the slice
        qreal midAngleDeg = startAngle + sweep / 2.0;
        qreal midAngleRad = (90.0 - midAngleDeg) * M_PI / 180.0;
        int labelR = outerR - 14;
        qreal lx = outerRect.center().x() + labelR * qCos(midAngleRad);
        qreal ly = outerRect.center().y() - labelR * qSin(midAngleRad);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(QRectF(lx - 22, ly - 8, 44, 16), Qt::AlignCenter,
                   categories[i].left(3) + " " + QString::number(count));

        startAngle += sweep;
    }

    // donut hole
    p.setBrush(Qt::white);
    p.setPen(Qt::NoPen);
    p.drawEllipse(innerRect);

    // center label
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 14, QFont::Bold));
    p.drawText(innerRect, Qt::AlignCenter, QString::number(total));
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 7));
    QRectF labelRect = innerRect.adjusted(0, 14, 0, 14);
    p.drawText(labelRect, Qt::AlignCenter, "horizons");
}

void PaperReadingHorizon2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Horizons",   QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Reached",          QString::number(reachedCount()),  QColor(22, 163, 74)},
        {"Avg Distance",     QString::number(avgDistance(), 'f', 0) + "km", QColor(217, 119, 6)},
        {"Waypoints Total",  QString::number([&]{
            int wp = 0;
            for (const auto& e : entries_) wp += e.waypoints;
            return wp;
        }()), QColor(220, 38, 38)}
    };
    int boxH = qMin(42, (rect.height() - 10) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);
        // background
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);
        // left accent bar
        p.setBrush(stats[i].color);
        p.drawRoundedRect(rect.x(), y, 4, boxH, 2, 2);
        // value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 12, y + 4, rect.width() - 20, 22, Qt::AlignVCenter, stats[i].value);
        // label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 12, y + 26, rect.width() - 20, 14, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperReadingHorizon2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Navigate your reading horizons");
        return;
    }
    infoLabel_->setText(QString("%1 horizons | %2 reached | avg %3km")
        .arg(entries_.size())
        .arg(reachedCount())
        .arg(avgDistance(), 0, 'f', 0));
}

void PaperReadingHorizon2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ReadingHorizon2Entry e;
        e.id = settings_.value("id").toInt();
        e.horizon = settings_.value("horizon").toString();
        e.category = settings_.value("category").toString();
        e.direction = settings_.value("direction").toString();
        e.distance = settings_.value("distance").toDouble();
        e.waypoints = settings_.value("waypoints").toInt();
        e.reached = settings_.value("reached").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingHorizon2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("horizon", entries_[i].horizon);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("direction", entries_[i].direction);
        settings_.setValue("distance", entries_[i].distance);
        settings_.setValue("waypoints", entries_[i].waypoints);
        settings_.setValue("reached", entries_[i].reached);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
