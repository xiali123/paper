#include "analysis/PaperEvidenceGrader.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>
#include <QtMath>

PaperEvidenceGrader::PaperEvidenceGrader(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "EvidenceGrader")
{
    setupUI();
    loadSettings();
}

void PaperEvidenceGrader::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Experimental", "Statistical", "Anecdotal", "Review"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; "
        "min-width: 120px; }"
        "QComboBox::drop-down { border: none; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter claim...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    gradeBtn_ = new QPushButton("Grade");
    gradeBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 14px; "
        "border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(gradeBtn_, &QPushButton::clicked, this, &PaperEvidenceGrader::onGrade);
    toolbar->addWidget(gradeBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperEvidenceGrader::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Grade evidence claims");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(640, 520);
}

void PaperEvidenceGrader::addEntry(const EvidenceGraderEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit evidenceGraded(entry.id, entry.grade);
    update();
}

QList<EvidenceGraderEntry> PaperEvidenceGrader::entries() const { return entries_; }

int PaperEvidenceGrader::strongCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.strong) ++c;
    return c;
}

qreal PaperEvidenceGrader::avgGrade() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.grade;
    return sum / entries_.size();
}

QMap<QString, int> PaperEvidenceGrader::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperEvidenceGrader::onGrade() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"experimental", "statistical", "anecdotal", "review"};
    QStringList sources = {"paper-A", "paper-B", "dataset-C", "survey-D", "meta-analysis-E"};
    QColor catColors[] = {
        QColor(0x3b, 0x82, 0xf6),  // #3b82f6 experimental
        QColor(0x16, 0xa3, 0x4a),  // #16a34a statistical
        QColor(0x7c, 0x3a, 0xed),  // #7c3aed anecdotal
        QColor(0xd9, 0x77, 0x06)   // #d97706 review
    };

    int comboIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(5);

    for (int i = 0; i < count; ++i) {
        EvidenceGraderEntry e;
        e.id = entries_.size() + 1;
        e.claim = text.left(12) + " claim" + QString::number(i);
        e.source = sources[QRandomGenerator::global()->bounded(sources.size())];

        if (comboIdx == 0) {
            int catIdx = QRandomGenerator::global()->bounded(categories.size());
            e.category = categories[catIdx];
            e.color = catColors[catIdx];
        } else {
            e.category = categories[comboIdx - 1];
            e.color = catColors[comboIdx - 1];
        }

        // Grade: 4.0 = A, 3.0 = B, 2.0 = C, 1.0 = D, 0.0 = F
        e.grade = QRandomGenerator::global()->bounded(5) * 1.0;
        // Add fractional variation
        if (e.grade > 0) {
            e.grade += QRandomGenerator::global()->bounded(30) / 100.0;
            if (e.grade > 4.0) e.grade = 4.0;
        }

        e.citations = QRandomGenerator::global()->bounded(50) + 1;
        e.strong = e.grade >= 3.0 && e.citations >= 10;

        addEntry(e);
    }
    inputField_->clear();
}

void PaperEvidenceGrader::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperEvidenceGrader::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Grade evidence claims");
        return;
    }

    int w = width(), h = height();

    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 28, "Evidence Grader");

    // Layout: grade cards on top half, pie chart bottom-left, stats bottom-right
    int topH = h * 55 / 100 - 30;
    drawGradeView(p, QRect(20, 42, w - 40, topH));

    int botY = 42 + topH + 12;
    int botH = h - botY - 10;
    int halfW = (w - 50) / 2;
    drawCategoryChart(p, QRect(20, botY, halfW, botH));
    drawStats(p, QRect(30 + halfW, botY, halfW, botH));
}

QString gradeToLetter(qreal g) {
    if (g >= 3.7) return "A";
    if (g >= 3.0) return "B";
    if (g >= 2.0) return "C";
    if (g >= 1.0) return "D";
    return "F";
}

QColor gradeToGaugeColor(qreal g) {
    if (g >= 3.7) return QColor(0x16, 0xa3, 0x4a); // green
    if (g >= 3.0) return QColor(0x22, 0xc5, 0x5e); // light green
    if (g >= 2.0) return QColor(0xeab, 0x30, 0x08); // yellow
    if (g >= 1.0) return QColor(0xf9, 0x73, 0x16); // orange
    return QColor(0xdc, 0x26, 0x26); // red
}

void PaperEvidenceGrader::drawGradeView(QPainter& p, const QRect& r) {
    int show = qMin(8, entries_.size());
    if (show == 0) return;

    int cardH = qMin(62, (r.height() - 10) / show);
    int cardW = r.width();

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = r.y() + i * (cardH + 4);

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(192));
        p.drawRoundedRect(r.x(), y, cardW, cardH, 6, 6);

        // Left color bar
        p.setBrush(e.color);
        QPainterPath bar;
        bar.addRoundedRect(QRectF(r.x(), y, 5, cardH), 2.5, 2.5);
        p.drawPath(bar);

        // Claim text
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(r.x() + 14, y + 4, cardW / 2 - 20, 18, Qt::AlignVCenter,
                   e.claim.left(28));

        // Source badge
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(160));
        QPainterPath badge;
        qreal badgeX = r.x() + 14;
        qreal badgeY = y + 24;
        badge.addRoundedRect(QRectF(badgeX, badgeY, 80, 16), 8, 8);
        p.drawPath(badge);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(QRectF(badgeX, badgeY, 80, 16), Qt::AlignCenter, e.source);

        // Category label
        p.setFont(QFont("Arial", 7));
        p.setPen(QColor(100, 116, 139));
        p.drawText(r.x() + 100, y + 24, 120, 16, Qt::AlignVCenter,
                   "[" + e.category + "]");

        // Grade gauge (semicircle)
        int gaugeSize = qMin(40, cardH - 8);
        int gaugeX = r.x() + cardW - gaugeSize - 80;
        int gaugeY = y + (cardH - gaugeSize) / 2;
        QRectF gaugeRect(gaugeX, gaugeY, gaugeSize, gaugeSize);

        // Background arc
        p.setPen(QPen(QColor(226, 232, 240), 3));
        p.setBrush(Qt::NoBrush);
        p.drawArc(gaugeRect.toRect(), 30 * 16, 120 * 16);

        // Filled arc proportional to grade (0-4.0 mapped over 120 degrees)
        QColor gaugeCol = gradeToGaugeColor(e.grade);
        int spanAngle = static_cast<int>((e.grade / 4.0) * 120) * 16;
        p.setPen(QPen(gaugeCol, 3));
        p.drawArc(gaugeRect.toRect(), 150 * 16, -spanAngle);

        // Grade letter
        p.setPen(gaugeCol);
        p.setFont(QFont("Arial", 10, QFont::Bold));
        p.drawText(gaugeRect, Qt::AlignCenter, gradeToLetter(e.grade));

        // Citations count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        int citX = r.x() + cardW - 68;
        p.drawText(citX, y + 4, 60, 16, Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.citations) + " cites");

        // Strong indicator
        if (e.strong) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(0x16, 0xa3, 0x4a));
            QPainterPath strongDot;
            strongDot.addEllipse(QRectF(citX + 42, y + 6, 8, 8));
            p.drawPath(strongDot);
        }

        // Grade numeric
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 7));
        p.drawText(citX, y + 20, 60, 16, Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.grade, 'f', 1) + "/4.0");
    }
}

void PaperEvidenceGrader::drawCategoryChart(QPainter& p, const QRect& r) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(r.topLeft(), "Category Distribution");

    auto counts = categoryCounts();
    if (counts.isEmpty()) return;

    QStringList catKeys = {"experimental", "statistical", "anecdotal", "review"};
    QString catLabels[] = {"Experimental", "Statistical", "Anecdotal", "Review"};
    QColor catColors[] = {
        QColor(0x3b, 0x82, 0xf6),
        QColor(0x16, 0xa3, 0x4a),
        QColor(0x7c, 0x3a, 0xed),
        QColor(0xd9, 0x77, 0x06)
    };

    int total = 0;
    QList<int> values;
    for (const auto& key : catKeys) {
        int v = counts.contains(key) ? counts[key] : 0;
        values.append(v);
        total += v;
    }

    if (total == 0) return;

    int side = qMin(r.width() - 20, r.height() - 40);
    int cx = r.x() + (r.width() - side) / 2;
    int cy = r.y() + 25;
    QRectF pieRect(cx, cy, side, side);

    qreal startAngle = 0.0;
    for (int i = 0; i < 4; ++i) {
        if (values[i] == 0) continue;
        qreal span = (static_cast<qreal>(values[i]) / total) * 360.0;

        QPainterPath slice;
        slice.moveTo(pieRect.center());
        slice.arcTo(pieRect, startAngle, span);
        slice.closeSubpath();

        p.setPen(Qt::NoPen);
        p.setBrush(catColors[i]);
        p.drawPath(slice);

        // Label on slice
        qreal midAngle = qDegreesToRadians(startAngle + span / 2.0);
        qreal labelR = side * 0.35;
        qreal lx = pieRect.center().x() + labelR * qCos(midAngle);
        qreal ly = pieRect.center().y() - labelR * qSin(midAngle);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(QRectF(lx - 20, ly - 8, 40, 16), Qt::AlignCenter,
                   catLabels[i].left(3) + " " + QString::number(values[i]));

        startAngle += span;
    }

    // Legend below pie
    int legY = cy + side + 6;
    int legX = r.x() + 4;
    for (int i = 0; i < 4; ++i) {
        p.setPen(Qt::NoPen);
        p.setBrush(catColors[i]);
        p.drawRoundedRect(legX, legY, 10, 10, 2, 2);
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 7));
        p.drawText(legX + 14, legY + 10, catLabels[i]);
        legX += 70;
    }
}

void PaperEvidenceGrader::drawStats(QPainter& p, const QRect& r) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(r.topLeft(), "Statistics");

    QString avgLetter = gradeToLetter(avgGrade());

    struct Stat {
        QString label;
        QString value;
        QColor color;
    };
    QList<Stat> stats = {
        {"Total Claims", QString::number(entries_.size()), QColor(0x3b, 0x82, 0xf6)},
        {"Avg Grade", QString::number(avgGrade(), 'f', 1) + " (" + avgLetter + ")",
         QColor(0x16, 0xa3, 0x4a)},
        {"Strong Evidence", QString::number(strongCount()),
         QColor(0x7c, 0x3a, 0xed)},
        {"Categories", QString::number(categoryCounts().size()),
         QColor(0xd9, 0x77, 0x06)}
    };

    int boxH = qMin(44, (r.height() - 30) / qMax(stats.size(), 1));
    for (int i = 0; i < stats.size(); ++i) {
        int y = r.y() + 22 + i * (boxH + 5);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        QPainterPath box;
        box.addRoundedRect(QRectF(r.x(), y, r.width(), boxH), 6, 6);
        p.drawPath(box);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(r.x() + 12, y + 4, r.width() - 24, 24, Qt::AlignVCenter,
                   stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(r.x() + 12, y + 28, r.width() - 24, 14, Qt::AlignVCenter,
                   stats[i].label);
    }
}

void PaperEvidenceGrader::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Grade evidence claims");
        return;
    }
    infoLabel_->setText(
        QString("%1 entries | %2 strong | avg %3 (%4)")
            .arg(entries_.size())
            .arg(strongCount())
            .arg(avgGrade(), 0, 'f', 1)
            .arg(gradeToLetter(avgGrade())));
}

void PaperEvidenceGrader::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        EvidenceGraderEntry e;
        e.id = settings_.value("id").toInt();
        e.claim = settings_.value("claim").toString();
        e.category = settings_.value("category").toString();
        e.source = settings_.value("source").toString();
        e.grade = settings_.value("grade").toDouble();
        e.citations = settings_.value("citations").toInt();
        e.strong = settings_.value("strong").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperEvidenceGrader::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("claim", entries_[i].claim);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("source", entries_[i].source);
        settings_.setValue("grade", entries_[i].grade);
        settings_.setValue("citations", entries_[i].citations);
        settings_.setValue("strong", entries_[i].strong);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
