#include "reading/PaperReviewExchange2.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperReviewExchange2::PaperReviewExchange2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReviewExchange2")
{
    setupUI();
    loadSettings();
    if (entries_.isEmpty()) {
        QStringList papers = {
            "Attention Mechanisms in Deep Learning",
            "BERT: Pre-training of Transformers",
            "YOLOv8 Object Detection Framework",
            "Robot Motion Planning with RL",
            "Computational Complexity of Graph Problems",
            "GPT-4 Technical Report Analysis",
            "Semantic Segmentation Survey",
            "Multi-Agent Reinforcement Learning"
        };
        QStringList categories = {
            "Machine Learning", "NLP", "Vision", "Robotics", "Theory"
        };
        QStringList reviewers = {"Dr. Alpha", "Prof. Beta", "Dr. Gamma"};
        QColor colors[] = {
            QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706),
            QColor(0xdc2626), QColor(0x7c3aed)
        };
        for (int i = 0; i < 8; ++i) {
            ReviewExchange2Entry e;
            e.id = i + 1;
            e.paper = papers[i];
            e.category = categories[i % 5];
            e.reviewer = reviewers[i % 3];
            e.quality = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
            e.comments = 1 + QRandomGenerator::global()->bounded(20);
            e.constructive = e.quality >= 0.6;
            e.color = colors[i % 5];
            entries_.append(e);
        }
        saveSettings();
        updateInfo();
        update();
    }
}

void PaperReviewExchange2::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Machine Learning", "NLP", "Vision", "Robotics", "Theory"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; min-width: 130px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Paper title...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    reviewBtn_ = new QPushButton("Review", this);
    reviewBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(reviewBtn_, &QPushButton::clicked, this, &PaperReviewExchange2::onReview);
    toolbar->addWidget(reviewBtn_);

    clearBtn_ = new QPushButton("Clear", this);
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReviewExchange2::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Review Exchange 2 ready", this);
    infoLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(700, 560);
}

void PaperReviewExchange2::addEntry(const ReviewExchange2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit reviewCompleted(entry.id, entry.quality);
    update();
}

QList<ReviewExchange2Entry> PaperReviewExchange2::entries() const {
    return entries_;
}

int PaperReviewExchange2::constructiveCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.constructive) c++;
    return c;
}

qreal PaperReviewExchange2::avgQuality() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.quality;
    return sum / entries_.size();
}

QMap<QString, int> PaperReviewExchange2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperReviewExchange2::onReview() {
    QStringList categories = {"Machine Learning", "NLP", "Vision", "Robotics", "Theory"};
    QStringList reviewers = {"Dr. Alpha", "Prof. Beta", "Dr. Gamma"};
    QColor colors[] = {
        QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706),
        QColor(0xdc2626), QColor(0x7c3aed)
    };

    int cIdx = categoryCombo_->currentIndex();
    ReviewExchange2Entry e;
    e.id = entries_.size() + 1;
    e.paper = inputField_->text().trimmed().isEmpty()
        ? "Paper " + QString::number(e.id)
        : inputField_->text().trimmed();
    e.category = cIdx == 0
        ? categories[QRandomGenerator::global()->bounded(categories.size())]
        : categories[cIdx - 1];
    e.reviewer = reviewers[QRandomGenerator::global()->bounded(reviewers.size())];
    e.quality = 0.2 + QRandomGenerator::global()->bounded(80) / 100.0;
    e.comments = 1 + QRandomGenerator::global()->bounded(25);
    e.constructive = e.quality >= 0.6;
    e.color = colors[QRandomGenerator::global()->bounded(5)];
    addEntry(e);
    inputField_->clear();
}

void PaperReviewExchange2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Review Exchange 2 ready");
    update();
}

void PaperReviewExchange2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No review data - click Review to add entries");
        return;
    }

    int w = width(), h = height();

    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Review Exchange 2");

    int contentTop = 45;
    int bottomH = static_cast<int>(h * 0.22);
    int topH = h - contentTop - bottomH - 10;
    int leftW = static_cast<int>(w * 0.6);

    drawReviewView(p, QRect(10, contentTop, leftW - 10, topH));
    drawCategoryChart(p, QRect(leftW + 10, contentTop, w - leftW - 20, topH));
    drawStats(p, QRect(10, h - bottomH, w - 20, bottomH - 10));
}

void PaperReviewExchange2::drawReviewView(QPainter& p, const QRect& rect) {
    // Section header
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.x(), rect.y() + 12, "Review Cards");

    int show = qMin(8, entries_.size());
    int cardH = qMin(54, (rect.height() - 24) / qMax(show, 1));
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
        p.drawText(rect.x() + 12, y + 3, cardW - 160, 16, Qt::AlignVCenter,
                   e.paper.left(30));

        // Reviewer badge
        QFontMetrics fm(QFont("Arial", 7));
        int badgeW = fm.horizontalAdvance(e.reviewer) + 12;
        int badgeX = rect.x() + 12;
        int badgeY = y + 20;
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(160));
        p.drawRoundedRect(badgeX, badgeY, badgeW, 14, 3, 3);
        p.setPen(e.color);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(badgeX, badgeY, badgeW, 14, Qt::AlignCenter, e.reviewer);

        // Quality score ring (circular gauge)
        int ringSize = qMin(cardH - 10, 34);
        int ringX = rect.x() + cardW - 100;
        int ringY = y + (cardH - ringSize) / 2;
        int ringCX = ringX + ringSize / 2;
        int ringCY = ringY + ringSize / 2;
        int ringR = ringSize / 2 - 2;

        // Background ring
        p.setPen(QPen(QColor(226, 232, 240), 3));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(ringCX - ringR, ringCY - ringR, ringR * 2, ringR * 2);

        // Progress arc
        int spanAngle = static_cast<int>(e.quality * 360 * 16);
        p.setPen(QPen(e.color, 3, Qt::SolidLine, Qt::RoundCap));
        p.drawArc(ringCX - ringR, ringCY - ringR, ringR * 2, ringR * 2,
                  90 * 16, -spanAngle);

        // Score text inside ring
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(QRect(ringX, ringY, ringSize, ringSize), Qt::AlignCenter,
                   QString::number(static_cast<int>(e.quality * 100)));

        // Comment count icon
        int commentX = rect.x() + cardW - 55;
        int commentY = y + (cardH - 14) / 2;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        p.drawRoundedRect(commentX, commentY, 20, 14, 3, 3);
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 7));
        p.drawText(QRect(commentX, commentY, 20, 14), Qt::AlignCenter,
                   QString::number(e.comments));

        // Constructive indicator
        int indX = rect.x() + cardW - 28;
        int indY = y + (cardH - 16) / 2;
        if (e.constructive) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(22, 163, 74));
            p.drawEllipse(indX, indY, 16, 16);
            p.setPen(Qt::white);
            p.setFont(QFont("Arial", 10, QFont::Bold));
            p.drawText(QRect(indX, indY, 16, 16), Qt::AlignCenter,
                       QString::fromUtf8("✓"));
        } else {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(220, 38, 38));
            p.drawEllipse(indX, indY, 16, 16);
            p.setPen(Qt::white);
            p.setFont(QFont("Arial", 10, QFont::Bold));
            p.drawText(QRect(indX, indY, 16, 16), Qt::AlignCenter,
                       QString::fromUtf8("✗"));
        }
    }
}

void PaperReviewExchange2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.x(), rect.y() + 12, "Reviewer Distribution");

    // Count reviews per reviewer per category
    QStringList reviewers = {"Dr. Alpha", "Prof. Beta", "Dr. Gamma"};
    QColor reviewerColors[] = {
        QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706)
    };

    QMap<QString, int> reviewerCounts;
    for (const auto& e : entries_) reviewerCounts[e.reviewer]++;

    int total = entries_.size();
    if (total == 0) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.adjusted(0, 20, 0, 0), Qt::AlignLeft | Qt::AlignTop, "No data yet");
        return;
    }

    // Donut chart
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 24 + (rect.height() - 80) / 2;
    int outerR = qMin(rect.width(), rect.height() - 80) / 2 - 10;
    int innerR = static_cast<int>(outerR * 0.55);

    int startAngle = 90 * 16;
    for (int i = 0; i < reviewers.size(); ++i) {
        int count = reviewerCounts.value(reviewers[i], 0);
        if (count == 0) continue;
        int spanAngle = static_cast<int>((static_cast<qreal>(count) / total) * 360 * 16);

        p.setPen(Qt::NoPen);
        p.setBrush(reviewerColors[i]);
        p.drawPie(cx - outerR, cy - outerR, outerR * 2, outerR * 2,
                  startAngle, spanAngle);
        startAngle += spanAngle;
    }

    // Donut hole
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
               "reviews");

    // Legend
    int legendY = rect.y() + rect.height() - 20;
    int legendX = rect.x() + 4;
    for (int i = 0; i < reviewers.size(); ++i) {
        int count = reviewerCounts.value(reviewers[i], 0);
        p.setPen(Qt::NoPen);
        p.setBrush(reviewerColors[i]);
        p.drawRoundedRect(legendX, legendY, 8, 8, 2, 2);
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 7));
        p.drawText(legendX + 11, legendY + 8,
                   reviewers[i].left(3) + " " + QString::number(count));
        legendX += 55;
    }
}

void PaperReviewExchange2::drawStats(QPainter& p, const QRect& rect) {
    int constructive = constructiveCount();
    qreal avgQ = avgQuality();
    int totalComments = 0;
    for (const auto& e : entries_) totalComments += e.comments;

    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Reviews",      QString::number(entries_.size()),    QColor(0x3b82f6)},
        {"Constructive Count",  QString::number(constructive),      QColor(0x16a34a)},
        {"Avg Quality",         QString::number(avgQ, 'f', 2),      QColor(0xd97706)},
        {"Total Comments",      QString::number(totalComments),     QColor(0x7c3aed)}
    };

    int boxCount = stats.size();
    int gap = 10;
    int boxW = (rect.width() - gap * (boxCount - 1)) / boxCount;
    int boxH = rect.height() - 4;

    for (int i = 0; i < boxCount; ++i) {
        int x = rect.x() + i * (boxW + gap);
        int y = rect.y();

        // Box background
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(x, y, boxW, boxH, 8, 8);

        // Top accent bar
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

void PaperReviewExchange2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Review Exchange 2 ready");
        return;
    }
    infoLabel_->setText(
        QString("%1 reviews | %2 constructive | quality %3")
            .arg(entries_.size())
            .arg(constructiveCount())
            .arg(avgQuality(), 0, 'f', 2));
}

void PaperReviewExchange2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ReviewExchange2Entry e;
        e.id = settings_.value("id").toInt();
        e.paper = settings_.value("paper").toString();
        e.category = settings_.value("category").toString();
        e.reviewer = settings_.value("reviewer").toString();
        e.quality = settings_.value("quality").toDouble();
        e.comments = settings_.value("comments").toInt();
        e.constructive = settings_.value("constructive").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReviewExchange2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("paper", entries_[i].paper);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("reviewer", entries_[i].reviewer);
        settings_.setValue("quality", entries_[i].quality);
        settings_.setValue("comments", entries_[i].comments);
        settings_.setValue("constructive", entries_[i].constructive);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
