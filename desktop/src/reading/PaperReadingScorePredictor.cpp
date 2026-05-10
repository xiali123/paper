#include "reading/PaperReadingScorePredictor.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReadingScorePredictor::PaperReadingScorePredictor(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingScorePredictor")
{
    setupUI();
    loadSettings();
}

void PaperReadingScorePredictor::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    predictBtn_ = new QPushButton("Predict");
    predictBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(predictBtn_, &QPushButton::clicked, this, &PaperReadingScorePredictor::onPredict);
    toolbar->addWidget(predictBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "ML", "NLP", "CV", "Systems"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingScorePredictor::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter paper title...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Predict reading comprehension scores");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperReadingScorePredictor::addEntry(const ScorePrediction& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit scorePredicted(entry.id, entry.predictedScore);
    update();
}

QList<ScorePrediction> PaperReadingScorePredictor::entries() const { return entries_; }

qreal PaperReadingScorePredictor::avgPredicted() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.predictedScore;
    return sum / entries_.size();
}

int PaperReadingScorePredictor::highConfidenceCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.highConfidence) c++;
    return c;
}

QMap<QString, int> PaperReadingScorePredictor::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperReadingScorePredictor::onPredict() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"ML", "NLP", "CV", "Systems"};
    QStringList factors = {"complexity", "length", "domain", "methodology"};
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        ScorePrediction e;
        e.id = entries_.size() + 1;
        e.paperTitle = text.left(12) + " paper" + QString::number(i);
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.predictedScore = 40 + QRandomGenerator::global()->bounded(60);
        e.confidence = 0.5 + QRandomGenerator::global()->bounded(50) / 100.0;
        e.factors = factors[QRandomGenerator::global()->bounded(factors.size())];
        e.citations = 5 + QRandomGenerator::global()->bounded(200);
        e.impactFactor = 0.5 + QRandomGenerator::global()->bounded(90) / 10.0;
        e.highConfidence = e.confidence >= 0.8;
        e.color = e.highConfidence ? QColor(16,185,129) : (e.confidence >= 0.6 ? QColor(59,130,246) : QColor(245,158,11));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperReadingScorePredictor::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Predict reading comprehension scores");
    update();
}

void PaperReadingScorePredictor::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Predict reading comprehension scores");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Score Predictor");
    int w = width(), h = height();
    drawPredictionList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReadingScorePredictor::drawPredictionList(QPainter& p, const QRect& rect) {
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
                   e.paperTitle.left(16) + (e.highConfidence ? " [HC]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + e.factors + " | IF " + QString::number(e.impactFactor, 'f', 1));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.predictedScore, 'f', 0) + " score");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.confidence * 100, 'f', 0) + "% conf | " + QString::number(e.citations) + " cit");
    }
}

void PaperReadingScorePredictor::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"ML", "NLP", "CV", "Systems"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 80));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x(), y + barH - 2, 30, barH, Qt::AlignRight | Qt::AlignVCenter, categories[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 35, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 38 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperReadingScorePredictor::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Papers", QString::number(entries_.size()), QColor(59,130,246)},
        {"High Conf", QString::number(highConfidenceCount()), QColor(16,185,129)},
        {"Avg Score", QString::number(avgPredicted(), 'f', 0), QColor(245,158,11)},
        {"Categories", QString::number(categoryCounts().size()), QColor(139,92,246)}
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

void PaperReadingScorePredictor::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Predict reading comprehension scores"); return; }
    infoLabel_->setText(QString("%1 papers | %2 high conf | %3 avg")
        .arg(entries_.size()).arg(highConfidenceCount()).arg(avgPredicted(), 0, 'f', 0));
}

void PaperReadingScorePredictor::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ScorePrediction e;
        e.id = settings_.value("id").toInt();
        e.paperTitle = settings_.value("paperTitle").toString();
        e.category = settings_.value("category").toString();
        e.predictedScore = settings_.value("predictedScore").toDouble();
        e.confidence = settings_.value("confidence").toDouble();
        e.factors = settings_.value("factors").toString();
        e.citations = settings_.value("citations").toInt();
        e.impactFactor = settings_.value("impactFactor").toDouble();
        e.highConfidence = settings_.value("highConfidence").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingScorePredictor::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("paperTitle", entries_[i].paperTitle);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("predictedScore", entries_[i].predictedScore);
        settings_.setValue("confidence", entries_[i].confidence);
        settings_.setValue("factors", entries_[i].factors);
        settings_.setValue("citations", entries_[i].citations);
        settings_.setValue("impactFactor", entries_[i].impactFactor);
        settings_.setValue("highConfidence", entries_[i].highConfidence);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
