#include "visualization/PaperKeywordEvolution.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperKeywordEvolution::PaperKeywordEvolution(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "KeywordEvolution")
{
    setupUI();
    loadSettings();
}

void PaperKeywordEvolution::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperKeywordEvolution::onGenerate);
    toolbar->addWidget(generateBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "ML", "NLP", "Vision", "Security", "Theory"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperKeywordEvolution::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter keyword to track evolution...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Track keyword evolution");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperKeywordEvolution::addEntry(const KeywordEvolutionEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit evolutionGenerated(entry.id, entry.growth);
    update();
}

QList<KeywordEvolutionEntry> PaperKeywordEvolution::entries() const { return entries_; }

qreal PaperKeywordEvolution::avgGrowth() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.growth;
    return sum / entries_.size();
}

int PaperKeywordEvolution::risingCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.trend == "rising") c++;
    return c;
}

QMap<QString, int> PaperKeywordEvolution::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperKeywordEvolution::onGenerate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"ML", "NLP", "Vision", "Security", "Theory"};
    QStringList trends = {"rising", "stable", "declining"};
    QStringList relateds = {"neural", "attention", "transformer", "embedding", "generative", "contrastive"};

    int catIdx = categoryCombo_->currentIndex();
    if (catIdx == 0) catIdx = 1 + QRandomGenerator::global()->bounded(categories.size());

    int count = 5 + QRandomGenerator::global()->bounded(6);
    for (int i = 0; i < count; ++i) {
        KeywordEvolutionEntry e;
        e.id = entries_.size() + 1;
        e.keyword = text.left(8) + "-" + QString::number(i);
        e.year = 2018 + i % 8;
        e.frequency = 5 + QRandomGenerator::global()->bounded(200);
        e.trend = trends[QRandomGenerator::global()->bounded(trends.size())];
        e.rank = 1 + QRandomGenerator::global()->bounded(50);
        e.category = categories[qMin(catIdx - 1, categories.size() - 1)];
        e.growth = -0.3 + QRandomGenerator::global()->bounded(130) / 100.0;
        e.relatedTerm = relateds[QRandomGenerator::global()->bounded(relateds.size())];
        e.color = e.trend == "rising" ? QColor(16,185,129) : (e.trend == "stable" ? QColor(245,158,11) : QColor(239,68,68));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperKeywordEvolution::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Track keyword evolution");
    update();
}

void PaperKeywordEvolution::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Track keyword evolution");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Keyword Evolution");

    int w = width(), h = height();
    drawTrendList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperKeywordEvolution::drawTrendList(QPainter& p, const QRect& rect) {
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
                   e.keyword.left(14) + " [" + QString::number(e.year) + "]");

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   "#" + QString::number(e.rank) + " | " + e.relatedTerm);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.frequency) + " freq");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   (e.growth >= 0 ? "+" : "") + QString::number(e.growth * 100, 'f', 0) + "% " + e.trend);
    }
}

void PaperKeywordEvolution::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"ML", "NLP", "Vision", "Security", "Theory"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68), QColor(139,92,246)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 2);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 100));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 55, barH, Qt::AlignRight | Qt::AlignVCenter, cats[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 60, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 63 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperKeywordEvolution::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Keywords", QString::number(entries_.size()), QColor(59,130,246)},
        {"Rising", QString::number(risingCount()), QColor(16,185,129)},
        {"Avg Growth", QString::number(avgGrowth() * 100, 'f', 0) + "%", QColor(245,158,11)},
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

void PaperKeywordEvolution::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Track keyword evolution"); return; }
    infoLabel_->setText(QString("%1 keywords | %2 rising | %3% avg growth")
        .arg(entries_.size()).arg(risingCount()).arg(avgGrowth() * 100, 0, 'f', 0));
}

void PaperKeywordEvolution::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        KeywordEvolutionEntry e;
        e.id = settings_.value("id").toInt();
        e.keyword = settings_.value("keyword").toString();
        e.year = settings_.value("year").toInt();
        e.frequency = settings_.value("frequency").toInt();
        e.trend = settings_.value("trend").toString();
        e.rank = settings_.value("rank").toInt();
        e.category = settings_.value("category").toString();
        e.growth = settings_.value("growth").toDouble();
        e.relatedTerm = settings_.value("relatedTerm").toString();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperKeywordEvolution::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("keyword", entries_[i].keyword);
        settings_.setValue("year", entries_[i].year);
        settings_.setValue("frequency", entries_[i].frequency);
        settings_.setValue("trend", entries_[i].trend);
        settings_.setValue("rank", entries_[i].rank);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("growth", entries_[i].growth);
        settings_.setValue("relatedTerm", entries_[i].relatedTerm);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
