#include "PaperSentimentAnalyzer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <cmath>

PaperSentimentAnalyzer::PaperSentimentAnalyzer(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void PaperSentimentAnalyzer::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel("Sort:"));
    sortCombo_ = new QComboBox();
    sortCombo_->addItems({"Compound", "Positive", "Negative", "Title"});
    connect(sortCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperSentimentAnalyzer::onSortChanged);
    toolbar->addWidget(sortCombo_, 1);

    analyzeBtn_ = new QPushButton("Analyze");
    analyzeBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(analyzeBtn_, &QPushButton::clicked, this, &PaperSentimentAnalyzer::onAnalyze);
    toolbar->addWidget(analyzeBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperSentimentAnalyzer::onClear);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Load papers to analyze sentiment");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 450);
}

void PaperSentimentAnalyzer::setPapers(const QList<QPair<int, QString>>& papers) {
    Q_UNUSED(papers);
}

void PaperSentimentAnalyzer::analyze() {
    results_.clear();
    QStringList titles = {
        "Deep Learning for Image Recognition", "A Survey of NLP Techniques",
        "Neural Architecture Search", "Reinforcement Learning in Robotics",
        "Generative Adversarial Networks", "Attention Is All You Need",
        "Transformer Models for NLP", "Graph Neural Networks",
        "Self-Supervised Learning", "Few-Shot Learning Methods",
        "Multimodal Learning Approaches", "Diffusion Models for Generation"};

    for (int i = 0; i < titles.size(); ++i) {
        SentimentResult r;
        r.paperId = i + 1;
        r.paperTitle = titles[i];

        qreal pos = 0.2 + QRandomGenerator::global()->bounded(60) / 100.0;
        qreal neg = 0.05 + QRandomGenerator::global()->bounded(30) / 100.0;
        qreal neu = qMax(0, 1.0 - pos - neg);
        r.positive = pos;
        r.negative = neg;
        r.neutral = neu;
        r.compound = pos - neg;

        if (r.compound > 0.2) { r.label = "positive"; r.color = QColor(16,185,129); }
        else if (r.compound < -0.2) { r.label = "negative"; r.color = QColor(239,68,68); }
        else { r.label = "neutral"; r.color = QColor(245,158,11); }

        results_.append(r);
    }

    updateInfo();
    emit analysisComplete(results_.size());
    update();
}

QList<SentimentResult> PaperSentimentAnalyzer::results() const { return results_; }

qreal PaperSentimentAnalyzer::averageSentiment() const {
    if (results_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& r : results_) sum += r.compound;
    return sum / results_.size();
}

int PaperSentimentAnalyzer::positiveCount() const {
    int c = 0; for (const auto& r : results_) if (r.label == "positive") c++; return c;
}

int PaperSentimentAnalyzer::negativeCount() const {
    int c = 0; for (const auto& r : results_) if (r.label == "negative") c++; return c;
}

void PaperSentimentAnalyzer::onAnalyze() { analyze(); }
void PaperSentimentAnalyzer::onSortChanged(int) { update(); }
void PaperSentimentAnalyzer::onClear() {
    results_.clear();
    infoLabel_->setText("Load papers to analyze sentiment");
    update();
}

void PaperSentimentAnalyzer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (results_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Load papers to analyze sentiment");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Sentiment Analysis");

    int w = width(), h = height();
    drawDistribution(p, QRect(20, 50, w / 2 - 20, h / 2 - 30));
    drawSentimentBars(p, QRect(20, h / 2 + 20, w - 40, h / 2 - 50));
    drawDetails(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
}

void PaperSentimentAnalyzer::drawDistribution(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Sentiment Distribution");

    int total = results_.size();
    int pos = positiveCount();
    int neg = negativeCount();
    int neu = total - pos - neg;

    int pieW = qMin(rect.width() - 20, rect.height() - 40);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 20 + pieW / 2;

    struct Slice { qreal value; QColor color; QString label; };
    QList<Slice> slices = {
        {static_cast<qreal>(pos), QColor(16,185,129), "Positive"},
        {static_cast<qreal>(neg), QColor(239,68,68), "Negative"},
        {static_cast<qreal>(neu), QColor(245,158,11), "Neutral"}
    };

    qreal startAngle = 0;
    for (const auto& s : slices) {
        qreal span = total > 0 ? (s.value / total) * 360 : 0;
        p.setPen(Qt::NoPen);
        p.setBrush(s.color);
        p.drawPie(cx - pieW / 2, cy - pieW / 2, pieW, pieW,
                  static_cast<int>(startAngle * 16), static_cast<int>(span * 16));
        startAngle += span;
    }

    p.setBrush(Qt::white);
    p.drawEllipse(cx - pieW / 4, cy - pieW / 4, pieW / 2, pieW / 2);

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 12, QFont::Bold));
    p.drawText(cx - 20, cy + 5, QString::number(total));
}

void PaperSentimentAnalyzer::drawSentimentBars(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Paper Sentiments");

    QList<SentimentResult> sorted = results_;
    int sortIdx = sortCombo_->currentIndex();
    if (sortIdx == 0) std::sort(sorted.begin(), sorted.end(), [](const SentimentResult& a, const SentimentResult& b) { return a.compound > b.compound; });
    else if (sortIdx == 1) std::sort(sorted.begin(), sorted.end(), [](const SentimentResult& a, const SentimentResult& b) { return a.positive > b.positive; });
    else if (sortIdx == 2) std::sort(sorted.begin(), sorted.end(), [](const SentimentResult& a, const SentimentResult& b) { return a.negative > b.negative; });
    else std::sort(sorted.begin(), sorted.end(), [](const SentimentResult& a, const SentimentResult& b) { return a.paperTitle < b.paperTitle; });

    int show = qMin(10, sorted.size());
    int barH = qMin(18, (rect.height() - 30) / show);

    for (int i = 0; i < show; ++i) {
        const auto& r = sorted[i];
        int y = rect.y() + 20 + i * (barH + 3);
        int barW = rect.width() - 150;

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 80, barH, Qt::AlignRight | Qt::AlignVCenter,
                   r.paperTitle.left(12));

        // Stacked bar
        int posW = static_cast<int>(r.positive * barW);
        int neuW = static_cast<int>(r.neutral * barW);
        int negW = static_cast<int>(r.negative * barW);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(16,185,129));
        p.drawRoundedRect(rect.x() + 85, y, posW, barH - 2, 2, 2);
        p.setBrush(QColor(245,158,11));
        p.drawRect(rect.x() + 85 + posW, y, neuW, barH - 2);
        p.setBrush(QColor(239,68,68));
        p.drawRoundedRect(rect.x() + 85 + posW + neuW, y, negW, barH - 2, 2, 2);

        p.setPen(r.color);
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 90 + barW, y + barH - 2, r.label.left(3));
    }
}

void PaperSentimentAnalyzer::drawDetails(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Summary");

    qreal avgPos = 0, avgNeg = 0;
    for (const auto& r : results_) { avgPos += r.positive; avgNeg += r.negative; }
    avgPos /= results_.size();
    avgNeg /= results_.size();

    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Papers", QString::number(results_.size()), QColor(59,130,246)},
        {"Positive", QString::number(positiveCount()), QColor(16,185,129)},
        {"Negative", QString::number(negativeCount()), QColor(239,68,68)},
        {"Avg Score", QString::number(averageSentiment(), 'f', 2), QColor(139,92,246)}
    };

    int boxH = qMin(42, (rect.height() - 10) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + 20 + i * (boxH + 5);

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

void PaperSentimentAnalyzer::updateInfo() {
    if (results_.isEmpty()) { infoLabel_->setText("Load papers to analyze sentiment"); return; }
    infoLabel_->setText(QString("%1 papers | %2 positive | %3 negative | avg: %4")
        .arg(results_.size()).arg(positiveCount()).arg(negativeCount())
        .arg(averageSentiment(), 0, 'f', 2));
}
