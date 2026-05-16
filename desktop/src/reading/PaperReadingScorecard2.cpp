#include "reading/PaperReadingScorecard2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReadingScorecard2::PaperReadingScorecard2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingScorecard2")
{
    setupUI();
    loadSettings();
    if (entries_.isEmpty()) {
        QColor palette[] = {QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"), QColor("#dc2626"), QColor("#7c3aed")};
        QStringList categories = {"Comprehension", "Speed", "Retention", "Analysis", "Synthesis"};
        QStringList metrics[] = {
            {"Main Idea Recall", "Detail Accuracy", "Inference Quality"},
            {"Words Per Minute", "Scan Efficiency", "Skim Accuracy"},
            {"Next-Day Recall", "Weekly Retention", "Concept Linking"},
            {"Argument Mapping", "Evidence Evaluation", "Bias Detection"},
            {"Cross-Paper Linking", "Summary Coherence", "Gap Identification"}
        };
        int id = 1;
        QStringList papers = {
            "Attention Is All You Need", "BERT: Pre-training of Deep Bidirectional Transformers",
            "GPT-4 Technical Report", "ResNet: Deep Residual Learning", "GANs: Generative Adversarial Networks",
            "Transformers in Vision", "Reinforcement Learning Survey", "Diffusion Models Review"
        };
        for (int i = 0; i < 8; ++i) {
            ReadingScorecard2Entry e;
            e.id = id++;
            e.paper = papers[i];
            int catIdx = i % 5;
            e.category = categories[catIdx];
            e.metric = metrics[catIdx][i % 3];
            e.score = 0.4 + QRandomGenerator::global()->bounded(60) / 100.0;
            e.attempts = 1 + QRandomGenerator::global()->bounded(5);
            e.mastered = e.score >= 0.85;
            e.color = palette[catIdx];
            entries_.append(e);
        }
        saveSettings();
        updateInfo();
        update();
    }
}

void PaperReadingScorecard2::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"Comprehension", "Speed", "Retention", "Analysis", "Synthesis"});
    categoryCombo_->setStyleSheet("QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(new QLabel("Category:"));
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter paper title...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    scoreBtn_ = new QPushButton("Score");
    scoreBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(scoreBtn_, &QPushButton::clicked, this, &PaperReadingScorecard2::onScore);
    toolbar->addWidget(scoreBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingScorecard2::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Reading scorecard");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(620, 500);
}

void PaperReadingScorecard2::addEntry(const ReadingScorecard2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit scoreUpdated(entry.id, entry.score);
    update();
}

QList<ReadingScorecard2Entry> PaperReadingScorecard2::entries() const {
    return entries_;
}

int PaperReadingScorecard2::masteredCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.mastered) c++;
    return c;
}

qreal PaperReadingScorecard2::avgScore() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.score;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingScorecard2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperReadingScorecard2::onScore() {
    QString paper = inputField_->text().trimmed();
    if (paper.isEmpty()) return;

    QColor palette[] = {QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"), QColor("#dc2626"), QColor("#7c3aed")};
    QStringList categories = {"Comprehension", "Speed", "Retention", "Analysis", "Synthesis"};
    QStringList metrics[] = {
        {"Main Idea Recall", "Detail Accuracy", "Inference Quality"},
        {"Words Per Minute", "Scan Efficiency", "Skim Accuracy"},
        {"Next-Day Recall", "Weekly Retention", "Concept Linking"},
        {"Argument Mapping", "Evidence Evaluation", "Bias Detection"},
        {"Cross-Paper Linking", "Summary Coherence", "Gap Identification"}
    };

    int catIdx = categoryCombo_->currentIndex();
    int nextId = entries_.isEmpty() ? 1 : entries_.last().id + 1;

    ReadingScorecard2Entry e;
    e.id = nextId;
    e.paper = paper;
    e.category = categories[catIdx];
    e.metric = metrics[catIdx][nextId % 3];
    e.score = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
    e.attempts = 1;
    e.mastered = e.score >= 0.85;
    e.color = palette[catIdx];
    addEntry(e);

    inputField_->clear();
}

void PaperReadingScorecard2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Reading scorecard");
    update();
}

void PaperReadingScorecard2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Reading scorecard");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Scorecard");

    int w = width(), h = height();
    drawScorecardView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReadingScorecard2::drawScorecardView(QPainter& p, const QRect& rect) {
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
                   e.paper.left(18) + " [" + e.category.left(5) + "]");
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.metric + " | x" + QString::number(e.attempts));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.score * 100, 'f', 0) + "%");
        QString status = e.mastered ? "Mastered" : "In progress";
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight, status);
    }
}

void PaperReadingScorecard2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList categories = {"Comprehension", "Speed", "Retention", "Analysis", "Synthesis"};
    QColor palette[] = {QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"), QColor("#dc2626"), QColor("#7c3aed")};

    int maxVal = 1;
    for (const auto& cat : categories)
        maxVal = qMax(maxVal, counts.contains(cat) ? counts[cat] : 0);

    int barH = qMin(20, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 4);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x(), y + barH - 2, 75, barH, Qt::AlignRight | Qt::AlignVCenter, categories[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(palette[i]);
        p.drawRoundedRect(rect.x() + 80, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 83 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperReadingScorecard2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries", QString::number(entries_.size()), QColor("#3b82f6")},
        {"Mastered", QString::number(masteredCount()), QColor("#16a34a")},
        {"Avg Score", QString::number(avgScore() * 100, 'f', 0) + "%", QColor("#d97706")},
        {"Categories", QString::number(categoryCounts().size()), QColor("#7c3aed")}
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

void PaperReadingScorecard2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Reading scorecard");
        return;
    }
    infoLabel_->setText(QString("%1 entries | %2 mastered | %3% avg")
        .arg(entries_.size()).arg(masteredCount()).arg(avgScore() * 100, 0, 'f', 0));
}

void PaperReadingScorecard2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ReadingScorecard2Entry e;
        e.id = settings_.value("id").toInt();
        e.paper = settings_.value("paper").toString();
        e.category = settings_.value("category").toString();
        e.metric = settings_.value("metric").toString();
        e.score = settings_.value("score").toDouble();
        e.attempts = settings_.value("attempts").toInt();
        e.mastered = settings_.value("mastered").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingScorecard2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("paper", entries_[i].paper);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("metric", entries_[i].metric);
        settings_.setValue("score", entries_[i].score);
        settings_.setValue("attempts", entries_[i].attempts);
        settings_.setValue("mastered", entries_[i].mastered);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
