#include "reading/PaperReadingOdyssey2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>
#include <QtMath>
#include <numeric>

PaperReadingOdyssey2::PaperReadingOdyssey2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingOdyssey2")
{
    setupUI();
    loadSettings();
}

void PaperReadingOdyssey2::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Intro", "Methods", "Results", "Discussion", "Conclusion"});
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Search chapters...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 2);

    exploreBtn_ = new QPushButton("Explore");
    exploreBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(exploreBtn_, &QPushButton::clicked, this, &PaperReadingOdyssey2::onExplore);
    toolbar->addWidget(exploreBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingOdyssey2::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    toolbar->addStretch();
    infoLabel_ = new QLabel("Begin your reading odyssey");
    infoLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    toolbar->addWidget(infoLabel_);

    setMinimumSize(640, 520);
}

void PaperReadingOdyssey2::addEntry(const ReadingOdyssey2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    if (entry.progress >= 1.0) {
        emit questCompleted(entry.id, entry.progress);
    }
    update();
}

QList<ReadingOdyssey2Entry> PaperReadingOdyssey2::entries() const { return entries_; }

int PaperReadingOdyssey2::epicCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.epic) c++;
    return c;
}

qreal PaperReadingOdyssey2::avgProgress() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.progress;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingOdyssey2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperReadingOdyssey2::onExplore() {
    static const QStringList categories = {"Intro", "Methods", "Results", "Discussion", "Conclusion"};
    static const QStringList quests = {
        "The Literature Review", "Methodology Mountains", "Results River", "Discussion Desert"
    };
    static const QColor colors[] = {
        QColor(59, 130, 246), QColor(22, 163, 74), QColor(217, 119, 6),
        QColor(220, 38, 38), QColor(124, 58, 237)
    };

    int cIdx = categoryCombo_->currentIndex();

    ReadingOdyssey2Entry e;
    e.id = entries_.size() + 1;
    e.chapter = inputField_->text().trimmed().isEmpty()
        ? QStringLiteral("Chapter %1").arg(e.id)
        : inputField_->text().trimmed().left(20);
    e.category = cIdx == 0
        ? categories[QRandomGenerator::global()->bounded(categories.size())]
        : categories[cIdx - 1];
    e.quest = quests[QRandomGenerator::global()->bounded(quests.size())];
    e.progress = QRandomGenerator::global()->bounded(101) / 100.0;
    e.pages = 5 + QRandomGenerator::global()->bounded(120);
    e.epic = QRandomGenerator::global()->bounded(100) < 30;
    e.color = colors[QRandomGenerator::global()->bounded(5)];

    addEntry(e);
    inputField_->clear();
}

void PaperReadingOdyssey2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Begin your reading odyssey");
    update();
}

void PaperReadingOdyssey2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Begin your reading odyssey");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Odyssey");

    int w = width(), h = height();
    drawOdysseyView(p, QRect(10, 50, static_cast<int>(w * 0.6) - 10, h - 80));
    drawCategoryChart(p, QRect(static_cast<int>(w * 0.6) + 10, 50,
                               static_cast<int>(w * 0.4) - 20, static_cast<int>(h * 0.75) - 50));
    drawStats(p, QRect(10, static_cast<int>(h * 0.75) + 10, w - 20,
                       static_cast<int>(h * 0.25) - 20));
}

void PaperReadingOdyssey2::drawOdysseyView(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    if (show == 0) return;

    int cardH = qMin(48, (rect.height() - 20) / qMax(show, 1));
    int cardW = rect.width() - 10;
    int startX = rect.x() + 5;
    int pathX = startX + 8;

    // winding path background line
    p.setPen(QPen(QColor(226, 232, 240), 2, Qt::DashLine));
    QPainterPath pathLine;
    pathLine.moveTo(pathX, rect.y());
    for (int i = 0; i < show; ++i) {
        int cy = rect.y() + 10 + i * (cardH + 6) + cardH / 2;
        int offset = (i % 2 == 0) ? 4 : -4;
        pathLine.lineTo(pathX + offset, cy);
    }
    p.drawPath(pathLine);

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + 10 + i * (cardH + 6);
        int indent = (i % 2) * 10;

        // card background
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(startX + indent, y, cardW - indent, cardH, 6, 6);

        // left accent bar
        p.setBrush(e.color);
        p.drawRoundedRect(startX + indent, y, 4, cardH, 2, 2);

        // chapter name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(startX + indent + 10, y + 3, cardW / 2 - 10, 18, Qt::AlignVCenter,
                   e.chapter.left(18));

        // quest badge
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(140));
        int badgeW = qMin(static_cast<int>(e.quest.length()) * 6 + 12, cardW / 3);
        p.drawRoundedRect(startX + indent + 10, y + 22, badgeW, 16, 3, 3);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(startX + indent + 10, y + 22, badgeW, 16, Qt::AlignCenter,
                   e.quest.left(20));

        // winding progress bar
        int barX = startX + indent + cardW / 2 + 10;
        int barW = cardW / 2 - 30;
        int barY1 = y + 8;
        int barY2 = y + 28;
        // background track (winding S-shape)
        p.setPen(QPen(QColor(226, 232, 240), 4, Qt::SolidLine, Qt::RoundCap));
        QPainterPath trackPath;
        trackPath.moveTo(barX, barY1);
        trackPath.cubicTo(barX + barW * 0.3, barY1 + 12,
                          barX + barW * 0.7, barY2 - 12,
                          barX + barW, barY2);
        p.drawPath(trackPath);
        // progress fill along the path
        int progW = static_cast<int>(e.progress * barW);
        p.setPen(QPen(e.color, 4, Qt::SolidLine, Qt::RoundCap));
        QPainterPath fillPath;
        fillPath.moveTo(barX, barY1);
        fillPath.cubicTo(barX + progW * 0.3, barY1 + 12 * (qreal(progW) / qMax(barW, 1)),
                         barX + progW * 0.7, barY2 - 12 * (qreal(progW) / qMax(barW, 1)),
                         barX + progW, barY1 + (barY2 - barY1) * (qreal(progW) / qMax(barW, 1)));
        p.drawPath(fillPath);

        // progress percentage
        p.setPen(e.color.darker(110));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(barX + barW + 4, y + 20,
                   QString::number(e.progress * 100, 'f', 0) + "%");

        // page count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(startX + indent + cardW - 60, y + 3, 55, 14, Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(e.pages) + " pages");

        // epic star indicator
        if (e.epic) {
            int starX = startX + indent + cardW - 18;
            int starY = y + 6;
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(245, 158, 11));
            QPainterPath star;
            qreal cx = starX + 5.0, cy = starY + 5.0, or_ = 6.0, ir = 2.5;
            for (int j = 0; j < 5; ++j) {
                qreal aOuter = (j * 72.0 - 90.0) * M_PI / 180.0;
                qreal aInner = ((j * 72.0) + 36.0 - 90.0) * M_PI / 180.0;
                star.moveTo(cx + or_ * qCos(aOuter), cy + or_ * qSin(aOuter));
                star.lineTo(cx + ir * qCos(aInner), cy + ir * qSin(aInner));
            }
            star.closeSubpath();
            p.drawPath(star);
        }

        // path node dot
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawEllipse(pathX - 4, y + cardH / 2 - 4, 8, 8);
    }
}

void PaperReadingOdyssey2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Category Distribution");

    auto counts = categoryCounts();
    static const QStringList categories = {"Intro", "Methods", "Results", "Discussion", "Conclusion"};
    static const QMap<QString, QColor> catColors = {
        {"Intro",      QColor(59, 130, 246)},
        {"Methods",    QColor(22, 163, 74)},
        {"Results",    QColor(217, 119, 6)},
        {"Discussion", QColor(220, 38, 38)},
        {"Conclusion", QColor(124, 58, 237)}
    };

    int total = 0;
    for (const auto& v : counts) total += v;
    if (total == 0) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.adjusted(0, 20, 0, 0), Qt::AlignCenter, "No data");
        return;
    }

    // donut chart
    int donutSize = qMin(rect.width(), rect.height() - 50);
    donutSize = qMax(donutSize, 60);
    int donutX = rect.x() + (rect.width() - donutSize) / 2;
    int donutY = rect.y() + 22;
    int outerR = donutSize / 2;
    int innerR = outerR * 55 / 100;
    QRectF outerRect(donutX, donutY, donutSize, donutSize);
    QRectF innerRect(donutX + (outerR - innerR), donutY + (outerR - innerR),
                     innerR * 2, innerR * 2);

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
    p.drawText(labelRect, Qt::AlignCenter, "quests");
}

void PaperReadingOdyssey2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Quests", QString::number(entries_.size()),              QColor(59, 130, 246)},
        {"Epic Count",   QString::number(epicCount()),                  QColor(220, 38, 38)},
        {"Avg Progress", QString::number(avgProgress() * 100, 'f', 0) + "%", QColor(217, 119, 6)},
        {"Total Pages",  QString::number(std::accumulate(entries_.begin(), entries_.end(), 0,
                               [](int s, const ReadingOdyssey2Entry& e) { return s + e.pages; })),
                                                                       QColor(22, 163, 74)}
    };

    int boxCount = stats.size();
    int gap = 8;
    int boxW = (rect.width() - gap * (boxCount - 1)) / boxCount;
    int boxH = rect.height() - 4;

    for (int i = 0; i < boxCount; ++i) {
        int x = rect.x() + i * (boxW + gap);
        int y = rect.y() + 2;

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(x, y, boxW, boxH, 6, 6);

        // top accent
        p.setBrush(stats[i].color);
        p.drawRoundedRect(x, y, boxW, 3, 2, 2);

        // value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", qMax(10, qMin(16, boxH / 3)), QFont::Bold));
        p.drawText(x + 8, y + 6, boxW - 16, boxH / 2 - 4, Qt::AlignVCenter, stats[i].value);

        // label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", qMax(7, qMin(10, boxH / 5))));
        p.drawText(x + 8, y + boxH / 2 + 4, boxW - 16, boxH / 2 - 8, Qt::AlignVCenter,
                   stats[i].label);
    }
}

void PaperReadingOdyssey2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Begin your reading odyssey");
        return;
    }
    infoLabel_->setText(QString("%1 quests | %2 epic | %3% done")
        .arg(entries_.size())
        .arg(epicCount())
        .arg(avgProgress() * 100, 0, 'f', 0));
}

void PaperReadingOdyssey2::loadSettings() {
    entries_.clear();
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ReadingOdyssey2Entry e;
        e.id       = settings_.value("id").toInt();
        e.chapter  = settings_.value("chapter").toString();
        e.category = settings_.value("category").toString();
        e.quest    = settings_.value("quest").toString();
        e.progress = settings_.value("progress").toDouble();
        e.pages    = settings_.value("pages").toInt();
        e.epic     = settings_.value("epic").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();

    // seed 8 demo entries if first launch
    if (entries_.isEmpty()) {
        static const QStringList chapters = {
            "Opening Passage", "Theory Foundations", "Data Expedition",
            "Analysis Caverns", "Insight Summit", "Debate Arena",
            "Synthesis Bridge", "Final Horizon"
        };
        static const QStringList categories = {"Intro", "Methods", "Results", "Discussion", "Conclusion"};
        static const QStringList quests = {
            "The Literature Review", "Methodology Mountains", "Results River", "Discussion Desert"
        };
        static const QColor colors[] = {
            QColor(59, 130, 246), QColor(22, 163, 74), QColor(217, 119, 6),
            QColor(220, 38, 38), QColor(124, 58, 237)
        };

        for (int i = 0; i < 8; ++i) {
            ReadingOdyssey2Entry e;
            e.id       = i + 1;
            e.chapter  = chapters[i];
            e.category = categories[i % categories.size()];
            e.quest    = quests[i % quests.size()];
            e.progress = (30 + QRandomGenerator::global()->bounded(71)) / 100.0;
            e.pages    = 10 + QRandomGenerator::global()->bounded(110);
            e.epic     = i == 2 || i == 5 || i == 7;
            e.color    = colors[i % 5];
            entries_.append(e);
        }
        saveSettings();
    }

    updateInfo();
}

void PaperReadingOdyssey2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",       entries_[i].id);
        settings_.setValue("chapter",  entries_[i].chapter);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("quest",    entries_[i].quest);
        settings_.setValue("progress", entries_[i].progress);
        settings_.setValue("pages",    entries_[i].pages);
        settings_.setValue("epic",     entries_[i].epic);
        settings_.setValue("color",    entries_[i].color.name());
    }
    settings_.endArray();
}
