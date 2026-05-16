#include "workspace/PaperReviewExchange.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReviewExchange::PaperReviewExchange(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReviewExchange")
{
    setupUI();
    loadSettings();
}

void PaperReviewExchange::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    exchangeBtn_ = new QPushButton("Exchange");
    exchangeBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(exchangeBtn_, &QPushButton::clicked, this, &PaperReviewExchange::onExchange);
    toolbar->addWidget(exchangeBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Peer Review", "Technical", "Methodology", "Writing", "Novelty"});
    toolbar->addWidget(categoryCombo_, 1);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter paper title...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReviewExchange::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    infoLabel_ = new QLabel("Exchange reviews");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperReviewExchange::onExchange() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"peer review", "technical", "methodology", "writing", "novelty"};
    QStringList reviewers = {"Dr. Smith", "Prof. Lee", "Dr. Chen", "Prof. Kim", "Dr. Wang"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,106), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        ExchangeEntry e;
        e.id = entries_.size() + 1;
        e.paper = text.left(8) + " rev" + QString::number(i);
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.reviewer = reviewers[QRandomGenerator::global()->bounded(reviewers.size())];
        e.quality = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
        e.rounds = 1 + QRandomGenerator::global()->bounded(5);
        e.consensus = QRandomGenerator::global()->bounded(2) == 0;
        e.color = colors[QRandomGenerator::global()->bounded(5)];
        entries_.append(e);
        saveSettings();
        updateInfo();
        update();
    }
    inputField_->clear();
}

void PaperReviewExchange::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Exchange reviews");
    update();
}

void PaperReviewExchange::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Exchange reviews");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Review Exchange");
    int w = width(), h = height();
    drawExchangeBoard(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReviewExchange::drawExchangeBoard(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(34, (rect.height() - 10) / qMax(show, 1));
    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 3);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.paper.left(14) + (e.consensus ? " [OK]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.reviewer + " | R" + QString::number(e.rounds));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.quality * 100, 'f', 0) + "%");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.category);
    }
}

void PaperReviewExchange::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    QStringList categories = {"peer review", "technical", "methodology", "writing", "novelty"};
    QString labels[] = {"Peer", "Technical", "Method", "Writing", "Novelty"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,106), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(22, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 65, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 70, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 73 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperReviewExchange::drawStats(QPainter& p, const QRect& rect) {
    int consensusCount = 0;
    qreal totalQuality = 0;
    int totalRounds = 0;
    for (const auto& e : entries_) {
        if (e.consensus) consensusCount++;
        totalQuality += e.quality;
        totalRounds += e.rounds;
    }
    qreal avgQuality = entries_.isEmpty() ? 0 : totalQuality / entries_.size();
    qreal avgRounds = entries_.isEmpty() ? 0 : static_cast<qreal>(totalRounds) / entries_.size();
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Reviews", QString::number(entries_.size()), QColor(59,130,246)},
        {"Consensus", QString::number(consensusCount), QColor(22,163,106)},
        {"Avg Quality", QString::number(avgQuality * 100, 'f', 0) + "%", QColor(217,119,6)},
        {"Avg Rounds", QString::number(avgRounds, 'f', 1), QColor(124,58,237)}
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

void PaperReviewExchange::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Exchange reviews"); return; }
    int consensusCount = 0;
    qreal totalQuality = 0;
    for (const auto& e : entries_) {
        if (e.consensus) consensusCount++;
        totalQuality += e.quality;
    }
    qreal avgQuality = totalQuality / entries_.size();
    infoLabel_->setText(QString("%1 reviews | %2 consensus | %3% quality")
        .arg(entries_.size()).arg(consensusCount).arg(avgQuality * 100, 0, 'f', 0));
}

void PaperReviewExchange::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ExchangeEntry e;
        e.id = settings_.value("id").toInt();
        e.paper = settings_.value("paper").toString();
        e.category = settings_.value("category").toString();
        e.reviewer = settings_.value("reviewer").toString();
        e.quality = settings_.value("quality").toDouble();
        e.rounds = settings_.value("rounds").toInt();
        e.consensus = settings_.value("consensus").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReviewExchange::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("paper", entries_[i].paper);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("reviewer", entries_[i].reviewer);
        settings_.setValue("quality", entries_[i].quality);
        settings_.setValue("rounds", entries_[i].rounds);
        settings_.setValue("consensus", entries_[i].consensus);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
