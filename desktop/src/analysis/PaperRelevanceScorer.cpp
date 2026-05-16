#include "analysis/PaperRelevanceScorer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperRelevanceScorer::PaperRelevanceScorer(QWidget* parent) : QWidget(parent) { setupUI(); loadSettings(); }

void PaperRelevanceScorer::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Content", "Method", "Novelty", "Impact", "Timeliness"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Paper title...");
    scoreBtn_ = new QPushButton("Score", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Scores: 0 | Top Tier: 0 | Avg Score: 0.00", this);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(scoreBtn_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);
    connect(scoreBtn_, &QPushButton::clicked, this, &PaperRelevanceScorer::onScore);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperRelevanceScorer::onClear);
}

void PaperRelevanceScorer::addEntry(const ScoreEntry& entry) { entries_.append(entry); updateInfo(); update(); }
QList<ScoreEntry> PaperRelevanceScorer::entries() const { return entries_; }

int PaperRelevanceScorer::topTierCount() const { int c = 0; for (const auto& e : entries_) if (e.topTier) c++; return c; }

qreal PaperRelevanceScorer::avgScore() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0; for (const auto& e : entries_) sum += e.score; return sum / entries_.size();
}

QMap<QString, int> PaperRelevanceScorer::categoryCounts() const { QMap<QString, int> m; for (const auto& e : entries_) m[e.category]++; return m; }

void PaperRelevanceScorer::onScore() {
    ScoreEntry e;
    e.id = entries_.size() + 1;
    e.paper = inputField_->text().trimmed();
    if (e.paper.isEmpty()) e.paper = QString("Paper_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    e.metric = categoryCombo_->currentText();
    e.score = QRandomGenerator::global()->bounded(0.0, 1.0);
    e.weight = QRandomGenerator::global()->bounded(0.5, 2.0);
    e.weighted = e.score * e.weight;
    e.topTier = e.weighted > 1.5;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    updateInfo(); saveSettings();
    emit scoreComputed(e.id, e.score);
    update();
}

void PaperRelevanceScorer::onClear() { entries_.clear(); updateInfo(); saveSettings(); update(); }

void PaperRelevanceScorer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawScoreList(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryChart(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperRelevanceScorer::drawScoreList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155)); p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Relevance Scores:");
    int y = rect.top() + 20;
    for (int i = 0; i < qMin(entries_.size(), 8); ++i) {
        const auto& e = entries_[i];
        p.setPen(e.color); p.setBrush(e.color);
        p.drawRoundedRect(rect.left(), y, 8, 8, 2, 2);
        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + 14, y + 9, QString("%1 | %2 | Score: %3 | W: %4 | %5")
            .arg(e.paper.left(12), e.metric)
            .arg(QString::number(e.score, 'f', 2))
            .arg(QString::number(e.weighted, 'f', 2))
            .arg(e.topTier ? "Top" : ""));
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperRelevanceScorer::drawCategoryChart(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts();
    int y = rect.top() + 5; p.setPen(QColor(0x334155)); p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Metric:"); y += 18;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    int ci = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        p.setBrush(colors[ci++ % colors.size()]);
        p.drawRoundedRect(rect.left(), y, qMin(it.value() * 20, rect.width()), 14, 3, 3);
        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + 4, y + 12, QString("%1: %2").arg(it.key()).arg(it.value()));
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperRelevanceScorer::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155)); p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total: %1").arg(entries_.size())); y += 16;
    p.drawText(rect.left(), y, QString("Top Tier: %1").arg(topTierCount())); y += 16;
    p.drawText(rect.left(), y, QString("Avg Score: %1").arg(QString::number(avgScore(), 'f', 3)));
}

void PaperRelevanceScorer::updateInfo() {
    infoLabel_->setText(QString("Scores: %1 | Top Tier: %2 | Avg Score: %3")
        .arg(entries_.size()).arg(topTierCount()).arg(QString::number(avgScore(), 'f', 2)));
}

void PaperRelevanceScorer::loadSettings() {
    settings_.beginGroup("RelevanceScorer");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        ScoreEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.paper = settings_.value(QString("paper_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.metric = settings_.value(QString("metric_%1").arg(i)).toString();
        e.score = settings_.value(QString("score_%1").arg(i)).toDouble();
        e.weight = settings_.value(QString("weight_%1").arg(i)).toDouble();
        e.weighted = settings_.value(QString("weighted_%1").arg(i)).toDouble();
        e.topTier = settings_.value(QString("topTier_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup(); updateInfo();
}

void PaperRelevanceScorer::saveSettings() {
    settings_.beginGroup("RelevanceScorer"); settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("paper_%1").arg(i), e.paper);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("metric_%1").arg(i), e.metric);
        settings_.setValue(QString("score_%1").arg(i), e.score);
        settings_.setValue(QString("weight_%1").arg(i), e.weight);
        settings_.setValue(QString("weighted_%1").arg(i), e.weighted);
        settings_.setValue(QString("topTier_%1").arg(i), e.topTier);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
