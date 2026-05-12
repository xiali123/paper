#include "analysis/PaperContrastAnalyzer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperContrastAnalyzer::PaperContrastAnalyzer(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ContrastAnalyzer")
{
    setupUI();
    loadSettings();
}

void PaperContrastAnalyzer::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    analyzeBtn_ = new QPushButton("Analyze");
    analyzeBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(analyzeBtn_, &QPushButton::clicked, this, &PaperContrastAnalyzer::onAnalyze);
    toolbar->addWidget(analyzeBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Methodology", "Results", "Claims", "Approach", "Data"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperContrastAnalyzer::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter contrast pair (format: paperA vs paperB)...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Analyze paper contrasts");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperContrastAnalyzer::addEntry(const ContrastEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit contrastAnalyzed(entry.id, entry.difference);
    update();
}

QList<ContrastEntry> PaperContrastAnalyzer::entries() const { return entries_; }

int PaperContrastAnalyzer::divergentCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.divergent) c++;
    return c;
}

qreal PaperContrastAnalyzer::avgDifference() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.difference;
    return sum / entries_.size();
}

QMap<QString, int> PaperContrastAnalyzer::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperContrastAnalyzer::onAnalyze() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QString paperA, paperB;
    int vsPos = text.indexOf(" vs ");
    if (vsPos >= 0) {
        paperA = text.left(vsPos).trimmed();
        paperB = text.mid(vsPos + 4).trimmed();
    } else {
        paperA = text.left(text.size() / 2).trimmed();
        paperB = text.mid(text.size() / 2).trimmed();
    }
    if (paperA.isEmpty()) paperA = "Paper-A";
    if (paperB.isEmpty()) paperB = "Paper-B";

    QStringList categories = {"Methodology", "Results", "Claims", "Approach", "Data"};
    QStringList dimensions = {"scope", "sample size", "method", "metric", "significance", "effect size"};

    QColor palette[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(5);

    for (int i = 0; i < count; ++i) {
        ContrastEntry e;
        e.id = entries_.size() + 1;
        e.paperA = paperA;
        e.paperB = paperB;
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.dimension = dimensions[QRandomGenerator::global()->bounded(dimensions.size())];
        e.similarity = QRandomGenerator::global()->bounded(100) / 100.0;
        e.difference = QRandomGenerator::global()->bounded(100) / 100.0;
        e.significance = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
        e.divergent = e.difference > 0.5;
        e.color = palette[i % 5];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperContrastAnalyzer::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Analyze paper contrasts");
    update();
}

void PaperContrastAnalyzer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Analyze paper contrasts");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Contrast Analyzer");
    int w = width(), h = height();
    drawContrastList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperContrastAnalyzer::drawContrastList(QPainter& p, const QRect& rect) {
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
                   e.paperA.left(8) + " vs " + e.paperB.left(8) + (e.divergent ? " [!]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + e.dimension);
        // similarity bar
        int barX = rect.x() + rect.width() / 2;
        int barW = (rect.width() / 2 - 30) / 2;
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(barX, y + 4, barW, 16, Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.similarity * 100, 'f', 0) + "% sim");
        p.drawText(barX + barW + 4, y + 4, barW, 16, Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.difference * 100, 'f', 0) + "% diff");
        // mini bar for difference
        p.setPen(Qt::NoPen);
        p.setBrush(e.divergent ? QColor(220, 38, 38) : QColor(22, 163, 74));
        int diffBarW = static_cast<int>(e.difference * barW);
        p.drawRoundedRect(barX + barW + 4, y + 20, diffBarW, 10, 2, 2);
    }
}

void PaperContrastAnalyzer::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"Methodology", "Results", "Claims", "Approach", "Data"};
    QString labels[] = {"Method", "Results", "Claims", "Approach", "Data"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(20, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 4);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 65, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 70, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 73 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperContrastAnalyzer::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Contrasts", QString::number(entries_.size()), QColor(59,130,246)},
        {"Divergent", QString::number(divergentCount()), QColor(220,38,38)},
        {"Avg Diff", QString::number(avgDifference() * 100, 'f', 0) + "%", QColor(217,119,6)},
        {"Categories", QString::number(categoryCounts().size()), QColor(124,58,237)}
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

void PaperContrastAnalyzer::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Analyze paper contrasts"); return; }
    infoLabel_->setText(QString("%1 contrasts | %2 divergent | %3% avg diff")
        .arg(entries_.size()).arg(divergentCount()).arg(avgDifference() * 100, 0, 'f', 0));
}

void PaperContrastAnalyzer::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ContrastEntry e;
        e.id = settings_.value("id").toInt();
        e.paperA = settings_.value("paperA").toString();
        e.paperB = settings_.value("paperB").toString();
        e.category = settings_.value("category").toString();
        e.dimension = settings_.value("dimension").toString();
        e.similarity = settings_.value("similarity").toDouble();
        e.difference = settings_.value("difference").toDouble();
        e.significance = settings_.value("significance").toDouble();
        e.divergent = settings_.value("divergent").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperContrastAnalyzer::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("paperA", entries_[i].paperA);
        settings_.setValue("paperB", entries_[i].paperB);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("dimension", entries_[i].dimension);
        settings_.setValue("similarity", entries_[i].similarity);
        settings_.setValue("difference", entries_[i].difference);
        settings_.setValue("significance", entries_[i].significance);
        settings_.setValue("divergent", entries_[i].divergent);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
