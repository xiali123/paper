#include "reading/PaperReadingNexus2.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperReadingNexus2::PaperReadingNexus2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingNexus2")
{
    setupUI();
    loadSettings();
    if (entries_.isEmpty()) {
        QStringList papers = {
            "Attention Is All You Need", "Deep Residual Learning",
            "BERT Pre-training", "GPT-4 Technical Report",
            "ResNet Architecture", "Transformer Survey",
            "Federated Learning Systems", "Neural Architecture Search"
        };
        QStringList categories = {"Science", "Humanities", "Engineering", "Medicine", "Social Science"};
        QStringList techniques = {"SQ3R", "Pomodoro", "Feynman", "Spaced Repetition", "Active Recall"};
        QColor colors[] = {
            QColor(59,130,246), QColor(22,163,74), QColor(217,119,6),
            QColor(220,38,38), QColor(124,58,237)
        };
        for (int i = 0; i < 8; ++i) {
            ReadingNexus2Entry e;
            e.id = i + 1;
            e.paper = papers[i];
            e.category = categories[i % 5];
            e.technique = techniques[i % 5];
            e.comprehension = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
            e.sessions = 1 + QRandomGenerator::global()->bounded(15);
            e.mastered = e.comprehension >= 0.8;
            e.color = colors[i % 5];
            entries_.append(e);
        }
        saveSettings();
        updateInfo();
        update();
    }
}

void PaperReadingNexus2::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Science", "Humanities", "Engineering", "Medicine", "Social Science"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; min-width: 120px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Search papers...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    updateBtn_ = new QPushButton("Update");
    updateBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(updateBtn_, &QPushButton::clicked, this, &PaperReadingNexus2::onUpdate);
    toolbar->addWidget(updateBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingNexus2::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Reading Nexus 2 ready");
    infoLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(680, 540);
}

void PaperReadingNexus2::addEntry(const ReadingNexus2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit nexusUpdated(entry.id, entry.comprehension);
    update();
}

QList<ReadingNexus2Entry> PaperReadingNexus2::entries() const { return entries_; }

int PaperReadingNexus2::masteredCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.mastered) c++;
    return c;
}

qreal PaperReadingNexus2::avgComprehension() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.comprehension;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingNexus2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperReadingNexus2::onUpdate() {
    QStringList categories = {"Science", "Humanities", "Engineering", "Medicine", "Social Science"};
    QStringList techniques = {"SQ3R", "Pomodoro", "Feynman", "Spaced Repetition", "Active Recall"};
    QColor colors[] = {
        QColor(59,130,246), QColor(22,163,74), QColor(217,119,6),
        QColor(220,38,38), QColor(124,58,237)
    };

    int cIdx = categoryCombo_->currentIndex();
    ReadingNexus2Entry e;
    e.id = entries_.size() + 1;
    e.paper = inputField_->text().trimmed().isEmpty()
        ? "Paper " + QString::number(e.id)
        : inputField_->text().trimmed();
    e.category = cIdx == 0
        ? categories[QRandomGenerator::global()->bounded(categories.size())]
        : categories[cIdx - 1];
    e.technique = techniques[QRandomGenerator::global()->bounded(techniques.size())];
    e.comprehension = 0.2 + QRandomGenerator::global()->bounded(80) / 100.0;
    e.sessions = 1 + QRandomGenerator::global()->bounded(12);
    e.mastered = e.comprehension >= 0.8;
    e.color = colors[QRandomGenerator::global()->bounded(5)];
    addEntry(e);
    inputField_->clear();
}

void PaperReadingNexus2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Reading Nexus 2 ready");
    update();
}

void PaperReadingNexus2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No reading data - click Update to add entries");
        return;
    }

    int w = width(), h = height();

    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Nexus 2");

    int contentTop = 45;
    int bottomH = static_cast<int>(h * 0.25);
    int topH = h - contentTop - bottomH - 10;
    int leftW = static_cast<int>(w * 0.6);

    drawNexusView(p, QRect(10, contentTop, leftW - 10, topH));
    drawCategoryChart(p, QRect(leftW + 10, contentTop, w - leftW - 20, topH));
    drawStats(p, QRect(10, h - bottomH, w - 20, bottomH - 10));
}

void PaperReadingNexus2::drawNexusView(QPainter& p, const QRect& rect) {
    // Section header
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.x(), rect.y() + 12, "Paper Cards");

    int show = qMin(8, entries_.size());
    int cardH = qMin(52, (rect.height() - 24) / qMax(show, 1));
    int cardW = rect.width() - 4;

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + 20 + i * (cardH + 3);

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(248, 250, 252));
        p.drawRoundedRect(rect.x(), y, cardW, cardH, 6, 6);

        // Left color bar
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, cardH, 2, 2);

        // Paper name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 12, y + 3, cardW - 120, 16, Qt::AlignVCenter,
                   e.paper.left(24));

        // Technique badge
        int badgeX = rect.x() + 12;
        int badgeY = y + 20;
        QFontMetrics fm(QFont("Arial", 7));
        int badgeW = fm.horizontalAdvance(e.technique) + 10;
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(160));
        p.drawRoundedRect(badgeX, badgeY, badgeW, 14, 3, 3);
        p.setPen(e.color);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(badgeX, badgeY, badgeW, 14, Qt::AlignCenter, e.technique);

        // Comprehension ring (circular progress)
        int ringSize = qMin(cardH - 8, 32);
        int ringX = rect.x() + cardW - 85;
        int ringY = y + (cardH - ringSize) / 2;
        int ringCenterX = ringX + ringSize / 2;
        int ringCenterY = ringY + ringSize / 2;
        int ringRadius = ringSize / 2 - 2;

        // Background ring
        p.setPen(QPen(QColor(226, 232, 240), 3));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(ringCenterX - ringRadius, ringCenterY - ringRadius,
                      ringRadius * 2, ringRadius * 2);

        // Progress arc
        int spanAngle = static_cast<int>(e.comprehension * 360 * 16);
        p.setPen(QPen(e.color, 3, Qt::SolidLine, Qt::RoundCap));
        p.drawArc(ringCenterX - ringRadius, ringCenterY - ringRadius,
                  ringRadius * 2, ringRadius * 2, 90 * 16, -spanAngle);

        // Percentage text inside ring
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(QRect(ringX, ringY, ringSize, ringSize), Qt::AlignCenter,
                   QString::number(static_cast<int>(e.comprehension * 100)) + "%");

        // Mastered checkmark
        if (e.mastered) {
            int checkX = rect.x() + cardW - 45;
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(22, 163, 74));
            p.drawEllipse(checkX, y + (cardH - 16) / 2, 16, 16);
            p.setPen(Qt::white);
            p.setFont(QFont("Arial", 10, QFont::Bold));
            p.drawText(QRect(checkX, y + (cardH - 16) / 2, 16, 16), Qt::AlignCenter, QString::fromUtf8("✓"));
        }

        // Sessions count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        int sessX = rect.x() + cardW - 25;
        p.drawText(sessX, y + (cardH - 12) / 2, 24, 12, Qt::AlignCenter,
                   QString::number(e.sessions) + "s");
    }
}

void PaperReadingNexus2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.x(), rect.y() + 12, "Category Distribution");

    auto counts = categoryCounts();
    QStringList allCats = {"Science", "Humanities", "Engineering", "Medicine", "Social Science"};
    QColor colors[] = {
        QColor(59,130,246), QColor(22,163,74), QColor(217,119,6),
        QColor(220,38,38), QColor(124,58,237)
    };

    int total = 0;
    for (const auto& c : allCats) total += counts.contains(c) ? counts[c] : 0;
    if (total == 0) return;

    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 24 + (rect.height() - 64) / 2;
    int outerR = qMin(rect.width(), rect.height() - 64) / 2 - 10;
    int innerR = static_cast<int>(outerR * 0.55);

    int startAngle = 90 * 16;
    for (int i = 0; i < 5; ++i) {
        int count = counts.contains(allCats[i]) ? counts[allCats[i]] : 0;
        if (count == 0) continue;
        int spanAngle = static_cast<int>((static_cast<qreal>(count) / total) * 360 * 16);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawPie(cx - outerR, cy - outerR, outerR * 2, outerR * 2,
                  startAngle, spanAngle);

        startAngle += spanAngle;
    }

    // Inner circle (donut hole)
    p.setBrush(Qt::white);
    p.drawEllipse(cx - innerR, cy - innerR, innerR * 2, innerR * 2);

    // Center label
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 14, QFont::Bold));
    p.drawText(QRect(cx - innerR, cy - 12, innerR * 2, 20), Qt::AlignCenter,
               QString::number(total));
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 7));
    p.drawText(QRect(cx - innerR, cy + 4, innerR * 2, 14), Qt::AlignCenter,
               "papers");

    // Legend
    int legendY = rect.y() + rect.height() - 20;
    int legendX = rect.x() + 4;
    for (int i = 0; i < 5; ++i) {
        int count = counts.contains(allCats[i]) ? counts[allCats[i]] : 0;
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(legendX, legendY, 8, 8, 2, 2);
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 7));
        p.drawText(legendX + 11, legendY + 8, allCats[i].left(3) + " " + QString::number(count));
        legendX += 50;
    }
}

void PaperReadingNexus2::drawStats(QPainter& p, const QRect& rect) {
    int mastered = masteredCount();
    qreal masteredRate = entries_.isEmpty() ? 0 : static_cast<qreal>(mastered) / entries_.size();
    qreal avgComp = avgComprehension();
    int totalSessions = 0;
    for (const auto& e : entries_) totalSessions += e.sessions;

    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Papers",  QString::number(entries_.size()), QColor(59,130,246)},
        {"Mastered Rate", QString::number(static_cast<int>(masteredRate * 100)) + "%", QColor(22,163,74)},
        {"Avg Comprehension", QString::number(avgComp, 'f', 2), QColor(217,119,6)},
        {"Total Sessions", QString::number(totalSessions), QColor(124,58,237)}
    };

    int boxCount = stats.size();
    int gap = 10;
    int boxW = (rect.width() - gap * (boxCount - 1)) / boxCount;
    int boxH = rect.height() - 4;

    for (int i = 0; i < boxCount; ++i) {
        int x = rect.x() + i * (boxW + gap);
        int y = rect.y();

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(x, y, boxW, boxH, 8, 8);

        // Top color accent bar
        p.setBrush(stats[i].color);
        p.drawRoundedRect(x, y, boxW, 4, 2, 2);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 16, QFont::Bold));
        p.drawText(x + 8, y + 8, boxW - 16, boxH / 2, Qt::AlignVCenter,
                   stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(x + 8, y + boxH / 2, boxW - 16, boxH / 2 - 4, Qt::AlignVCenter,
                   stats[i].label);
    }
}

void PaperReadingNexus2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Reading Nexus 2 ready");
        return;
    }
    infoLabel_->setText(
        QString("%1 papers | %2 mastered | comprehension %3")
            .arg(entries_.size())
            .arg(masteredCount())
            .arg(avgComprehension(), 0, 'f', 2));
}

void PaperReadingNexus2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ReadingNexus2Entry e;
        e.id = settings_.value("id").toInt();
        e.paper = settings_.value("paper").toString();
        e.category = settings_.value("category").toString();
        e.technique = settings_.value("technique").toString();
        e.comprehension = settings_.value("comprehension").toDouble();
        e.sessions = settings_.value("sessions").toInt();
        e.mastered = settings_.value("mastered").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingNexus2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("paper", entries_[i].paper);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("technique", entries_[i].technique);
        settings_.setValue("comprehension", entries_[i].comprehension);
        settings_.setValue("sessions", entries_[i].sessions);
        settings_.setValue("mastered", entries_[i].mastered);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
