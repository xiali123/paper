#include "workspace/PaperPeerReview.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperPeerReview::PaperPeerReview(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "PeerReview")
{
    setupUI();
    loadSettings();
}

void PaperPeerReview::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    reviewBtn_ = new QPushButton("Review");
    reviewBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(reviewBtn_, &QPushButton::clicked, this, &PaperPeerReview::onReview);
    toolbar->addWidget(reviewBtn_);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Novelty", "Methodology", "Clarity", "Significance", "Reproducibility"});
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Paper title...");
    toolbar->addWidget(inputField_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperPeerReview::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Generate peer reviews");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 450);
}

void PaperPeerReview::addEntry(const ReviewEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit reviewDone(entry.id, entry.score);
    update();
}

QList<ReviewEntry> PaperPeerReview::entries() const { return entries_; }

int PaperPeerReview::acceptedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.accepted) c++;
    return c;
}

qreal PaperPeerReview::avgScore() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.score;
    return sum / entries_.size();
}

QMap<QString, int> PaperPeerReview::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperPeerReview::onReview() {
    QString paper = inputField_->text().trimmed().isEmpty()
        ? QString("Paper %1").arg(entries_.size() + 1)
        : inputField_->text().trimmed();

    QStringList categories = {"Novelty", "Methodology", "Clarity", "Significance", "Reproducibility"};
    QStringList reviewers = {"Reviewer A", "Reviewer B", "Reviewer C", "Reviewer D", "Reviewer E"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};

    int count = 3 + QRandomGenerator::global()->bounded(4);

    for (int i = 0; i < count; ++i) {
        ReviewEntry e;
        e.id = entries_.size() + 1;
        e.paper = paper;
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.reviewer = reviewers[QRandomGenerator::global()->bounded(reviewers.size())];
        e.score = QRandomGenerator::global()->bounded(100) / 10.0;
        e.comments = QRandomGenerator::global()->bounded(20);
        e.accepted = e.score >= 5.0;
        e.color = colors[QRandomGenerator::global()->bounded(5)];
        addEntry(e);
    }

    inputField_->clear();
}

void PaperPeerReview::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Generate peer reviews");
    update();
}

void PaperPeerReview::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Generate peer reviews");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Peer Review Board");

    int w = width(), h = height();
    drawReviewBoard(p, QRect(20, 50, w - 40, h * 2 / 3 - 40));
    drawCategoryChart(p, QRect(20, h * 2 / 3 + 10, (w - 50) / 2, h / 3 - 30));
    drawStats(p, QRect(30 + (w - 50) / 2, h * 2 / 3 + 10, (w - 50) / 2, h / 3 - 30));
}

void PaperPeerReview::drawReviewBoard(QPainter& p, const QRect& rect) {
    int filterIdx = categoryCombo_->currentIndex();
    QStringList categories = {"Novelty", "Methodology", "Clarity", "Significance", "Reproducibility"};

    int row = 0;
    int rowH = 36;
    for (const auto& e : entries_) {
        if (filterIdx > 0 && e.category != categories[filterIdx - 1]) continue;

        int y = rect.y() + row * rowH;
        if (y + rowH > rect.bottom()) break;

        p.setPen(Qt::NoPen);
        p.setBrush(row % 2 == 0 ? QColor(248, 250, 252) : Qt::white);
        p.drawRoundedRect(rect.x(), y, rect.width(), rowH - 2, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawEllipse(rect.x() + 8, y + 10, 12, 12);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 26, y + 5, rect.width() / 4, 16, Qt::AlignVCenter, e.paper.left(20));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 26 + rect.width() / 4, y + 5, rect.width() / 6, 16, Qt::AlignVCenter, e.category);

        p.drawText(rect.x() + 26 + rect.width() * 5 / 12, y + 5, rect.width() / 6, 16, Qt::AlignVCenter, e.reviewer);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 26 + rect.width() * 7 / 12, y + 5, rect.width() / 10, 16, Qt::AlignVCenter, QString::number(e.score, 'f', 1));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 26 + rect.width() * 9 / 12, y + 5, rect.width() / 10, 16, Qt::AlignVCenter, QString("%1 cmt").arg(e.comments));

        p.setPen(Qt::NoPen);
        p.setBrush(e.accepted ? QColor(34, 197, 94) : QColor(239, 68, 68));
        p.drawRoundedRect(rect.x() + 26 + rect.width() * 11 / 12, y + 6, 50, 18, 4, 4);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(rect.x() + 26 + rect.width() * 11 / 12, y + 6, 50, 18, Qt::AlignCenter, e.accepted ? "Accepted" : "Rejected");

        row++;
    }
}

void PaperPeerReview::drawCategoryChart(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts();
    QStringList categories = {"Novelty", "Methodology", "Clarity", "Significance", "Reproducibility"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};

    int maxCount = 1;
    for (const auto& cat : categories) maxCount = qMax(maxCount, counts.value(cat, 0));

    int barW = qMin(30, (rect.width() - 20) / 5 - 8);
    int groupW = barW + 8;
    int startX = rect.x() + (rect.width() - 5 * groupW) / 2;

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 9, QFont::Bold));
    p.drawText(rect.x(), rect.y(), rect.width(), 20, Qt::AlignCenter, "By Category");

    for (int i = 0; i < 5; ++i) {
        int count = counts.value(categories[i], 0);
        int barH = static_cast<int>(static_cast<qreal>(count) / maxCount * (rect.height() - 50));
        int x = startX + i * groupW;
        int y = rect.y() + rect.height() - 20 - barH;

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(x, y, barW, barH, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(x - 4, rect.y() + rect.height() - 8, barW + 8, 14, Qt::AlignCenter, categories[i].left(3));

        if (count > 0) {
            p.setPen(QColor(15, 23, 42));
            p.setFont(QFont("Arial", 7, QFont::Bold));
            p.drawText(x, y - 14, barW, 14, Qt::AlignCenter, QString::number(count));
        }
    }
}

void PaperPeerReview::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 9, QFont::Bold));
    p.drawText(rect.x(), rect.y(), rect.width(), 20, Qt::AlignCenter, "Statistics");

    int total = entries_.size();
    int accepted = acceptedCount();
    qreal avg = avgScore();
    int rejected = total - accepted;

    QStringList labels = {"Total", "Accepted", "Rejected", "Avg Score"};
    int values[] = {total, accepted, rejected, qRound(avg * 10)};
    QColor colors[] = {QColor(59,130,246), QColor(34,197,94), QColor(239,68,68), QColor(217,119,6)};

    int boxW = qMin(80, (rect.width() - 30) / 4);
    int startX = rect.x() + (rect.width() - 4 * (boxW + 8)) / 2;

    for (int i = 0; i < 4; ++i) {
        int x = startX + i * (boxW + 8);
        int y = rect.y() + 24;

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i].lighter(190));
        p.drawRoundedRect(x, y, boxW, 45, 6, 6);

        p.setPen(colors[i]);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(x + 4, y + 5, boxW - 8, 22, Qt::AlignCenter,
                   i == 3 ? QString::number(avg, 'f', 1) : QString::number(values[i]));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(x + 4, y + 28, boxW - 8, 14, Qt::AlignCenter, labels[i]);
    }
}

void PaperPeerReview::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Generate peer reviews"); return; }
    infoLabel_->setText(QString("%1 reviews | %2 accepted | avg %3")
        .arg(entries_.size()).arg(acceptedCount()).arg(QString::number(avgScore(), 'f', 1)));
}

void PaperPeerReview::loadSettings() {
    int size = settings_.beginReadArray("reviews");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ReviewEntry e;
        e.id = settings_.value("id").toInt();
        e.paper = settings_.value("paper").toString();
        e.category = settings_.value("category").toString();
        e.reviewer = settings_.value("reviewer").toString();
        e.score = settings_.value("score").toDouble();
        e.comments = settings_.value("comments").toInt();
        e.accepted = settings_.value("accepted").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperPeerReview::saveSettings() {
    settings_.beginWriteArray("reviews");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("paper", entries_[i].paper);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("reviewer", entries_[i].reviewer);
        settings_.setValue("score", entries_[i].score);
        settings_.setValue("comments", entries_[i].comments);
        settings_.setValue("accepted", entries_[i].accepted);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
