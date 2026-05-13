#include "analysis/PaperStrengthAssessor2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperStrengthAssessor2::PaperStrengthAssessor2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "StrengthAssessor2")
{
    setupUI();
    loadSettings();
}

void PaperStrengthAssessor2::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    assessBtn_ = new QPushButton("Assess");
    assessBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(assessBtn_, &QPushButton::clicked, this, &PaperStrengthAssessor2::onAssess);
    toolbar->addWidget(assessBtn_);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Medicine", "Psychology", "Economics", "Education", "Engineering"});
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter claim to assess...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperStrengthAssessor2::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Assess claim strength");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(680, 560);
}

void PaperStrengthAssessor2::addEntry(const StrengthAssessor2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit claimAssessed(entry.id, entry.strength);
    update();
}

QList<StrengthAssessor2Entry> PaperStrengthAssessor2::entries() const {
    return entries_;
}

int PaperStrengthAssessor2::robustCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.robust) c++;
    return c;
}

qreal PaperStrengthAssessor2::avgStrength() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.strength;
    return sum / entries_.size();
}

QMap<QString, int> PaperStrengthAssessor2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperStrengthAssessor2::onAssess() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    static const QStringList categories = {"Medicine", "Psychology", "Economics", "Education", "Engineering"};
    static const QStringList evidenceTypes = {"Experimental", "Observational", "Meta-analysis", "Theoretical", "Survey"};
    static const QColor palette[] = {
        QColor(59, 130, 246), QColor(22, 163, 74), QColor(217, 119, 6),
        QColor(220, 38, 38), QColor(124, 58, 237)
    };

    int count = 4 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        StrengthAssessor2Entry e;
        e.id = entries_.size() + 1;
        e.claim = text.left(20) + " claim " + QString::number(e.id);
        int cIdx = QRandomGenerator::global()->bounded(categories.size());
        e.category = categories[cIdx];
        int eIdx = QRandomGenerator::global()->bounded(evidenceTypes.size());
        e.evidence = evidenceTypes[eIdx];
        e.strength = 0.15 + QRandomGenerator::global()->bounded(85) / 100.0;
        e.sources = 1 + QRandomGenerator::global()->bounded(30);
        e.robust = e.strength >= 0.7 && e.sources >= 5;
        e.color = palette[cIdx];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperStrengthAssessor2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Assess claim strength");
    update();
}

void PaperStrengthAssessor2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Assess claim strength");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Strength Assessor 2");

    int w = width(), h = height();
    drawAssessorView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperStrengthAssessor2::drawAssessorView(QPainter& p, const QRect& rect) {
    int maxShow = 8;
    int itemH = qMin(52, (rect.height() - 10) / maxShow);
    int shown = 0;

    static const QColor palette[] = {
        QColor(59, 130, 246), QColor(22, 163, 74), QColor(217, 119, 6),
        QColor(220, 38, 38), QColor(124, 58, 237)
    };
    static const QStringList evidenceTypes = {"Experimental", "Observational", "Meta-analysis", "Theoretical", "Survey"};

    for (int i = entries_.size() - 1; i >= 0 && shown < maxShow; --i) {
        const auto& e = entries_[i];
        int y = rect.y() + shown * (itemH + 4);
        int rw = rect.width();

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rw, itemH, 6, 6);

        // Left color accent
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 5, itemH, 2, 2);

        // Evidence type badge
        int eIdx = evidenceTypes.indexOf(e.evidence);
        if (eIdx < 0) eIdx = 0;
        QColor badgeColor = palette[eIdx];
        QString badgeText = e.evidence;
        p.setFont(QFont("Arial", 7, QFont::Bold));
        QFontMetrics fm(p.font());
        int badgeW = fm.horizontalAdvance(badgeText) + 10;
        int badgeX = rect.x() + 10;
        int badgeY = y + 3;
        p.setPen(Qt::NoPen);
        p.setBrush(badgeColor);
        p.drawRoundedRect(badgeX, badgeY, badgeW, 14, 3, 3);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(badgeX, badgeY, badgeW, 14, Qt::AlignCenter, badgeText);

        // Claim text
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 18, rw - 90, 14, Qt::AlignVCenter, e.claim.left(24));

        // Strength gauge (colored arc)
        int gaugeCx = rect.x() + rw - 38;
        int gaugeCy = y + itemH / 2;
        int gaugeR = 16;
        QRectF arcRect(gaugeCx - gaugeR, gaugeCy - gaugeR, gaugeR * 2, gaugeR * 2);

        // Background arc
        p.setPen(QPen(QColor(226, 232, 240), 3));
        p.setBrush(Qt::NoBrush);
        p.drawArc(arcRect, 0, 360 * 16);

        // Strength arc
        QColor gaugeColor;
        if (e.strength >= 0.7) gaugeColor = QColor(22, 163, 74);
        else if (e.strength >= 0.4) gaugeColor = QColor(217, 119, 6);
        else gaugeColor = QColor(220, 38, 38);

        int spanAngle = static_cast<int>(e.strength * 360 * 16);
        p.setPen(QPen(gaugeColor, 3, Qt::SolidLine, Qt::RoundCap));
        p.drawArc(arcRect, 90 * 16, -spanAngle);

        // Strength percentage in center of gauge
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(arcRect.toRect(), Qt::AlignCenter,
                   QString::number(e.strength * 100, 'f', 0) + "%");

        // Source count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 33, rw / 2 - 10, 14, Qt::AlignVCenter,
                   QString::number(e.sources) + " sources");

        // Robust checkmark
        if (e.robust) {
            p.setPen(QColor(22, 163, 74));
            p.setFont(QFont("Arial", 10, QFont::Bold));
            p.drawText(rect.x() + rw / 2, y + 33, 40, 14, Qt::AlignVCenter,
                       QChar(0x2713) + " robust");
        }

        // Category label
        p.setPen(e.color);
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + rw / 2, y + 18, rw / 2 - 50, 14,
                   Qt::AlignVCenter, e.category);

        shown++;
    }
}

void PaperStrengthAssessor2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Category Distribution");

    auto counts = categoryCounts();
    static const QStringList categories = {"Medicine", "Psychology", "Economics", "Education", "Engineering"};
    static const QColor colors[] = {
        QColor(59, 130, 246), QColor(22, 163, 74), QColor(217, 119, 6),
        QColor(220, 38, 38), QColor(124, 58, 237)
    };

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 4);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 120));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 75, barH, Qt::AlignRight | Qt::AlignVCenter, categories[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 80, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 83 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperStrengthAssessor2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Claims",       QString::number(entries_.size()),                QColor(59, 130, 246)},
        {"Robust",       QString::number(robustCount()),                  QColor(22, 163, 74)},
        {"Avg Strength", QString::number(avgStrength() * 100, 'f', 0) + "%", QColor(217, 119, 6)},
        {"Categories",   QString::number(categoryCounts().size()),        QColor(124, 58, 237)}
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

void PaperStrengthAssessor2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Assess claim strength");
        return;
    }
    infoLabel_->setText(QString("%1 claims | %2 robust | %3% avg")
        .arg(entries_.size())
        .arg(robustCount())
        .arg(avgStrength() * 100, 0, 'f', 0));
}

void PaperStrengthAssessor2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        StrengthAssessor2Entry e;
        e.id       = settings_.value("id").toInt();
        e.claim    = settings_.value("claim").toString();
        e.category = settings_.value("category").toString();
        e.evidence = settings_.value("evidence").toString();
        e.strength = settings_.value("strength").toDouble();
        e.sources  = settings_.value("sources").toInt();
        e.robust   = settings_.value("robust").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperStrengthAssessor2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",       entries_[i].id);
        settings_.setValue("claim",    entries_[i].claim);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("evidence", entries_[i].evidence);
        settings_.setValue("strength", entries_[i].strength);
        settings_.setValue("sources",  entries_[i].sources);
        settings_.setValue("robust",   entries_[i].robust);
        settings_.setValue("color",    entries_[i].color.name());
    }
    settings_.endArray();
}
