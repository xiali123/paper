#include "analysis/PaperHypothesisTester.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperHypothesisTester::PaperHypothesisTester(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "HypothesisTester")
{
    setupUI();
    loadSettings();
}

void PaperHypothesisTester::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    testBtn_ = new QPushButton("Test");
    testBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(testBtn_, &QPushButton::clicked, this, &PaperHypothesisTester::onTest);
    toolbar->addWidget(testBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "T-test", "Chi-square", "ANOVA", "Regression"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperHypothesisTester::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter hypothesis...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Test hypotheses");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperHypothesisTester::addEntry(const HypothesisEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit hypothesisTested(entry.id, entry.pValue);
    update();
}

QList<HypothesisEntry> PaperHypothesisTester::entries() const { return entries_; }

int PaperHypothesisTester::significantCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.significant) c++;
    return c;
}

qreal PaperHypothesisTester::avgEffect() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.effectSize;
    return sum / entries_.size();
}

QMap<QString, int> PaperHypothesisTester::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperHypothesisTester::onTest() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"t-test", "chi-square", "anova", "regression"};
    QStringList hypotheses = {"H0: no difference", "H0: independent", "H0: equal means", "H0: no effect",
                             "H0: normal dist", "H0: no correlation"};
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        HypothesisEntry e;
        e.id = entries_.size() + 1;
        e.hypothesis = hypotheses[QRandomGenerator::global()->bounded(hypotheses.size())];
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.testType = e.category;
        e.pValue = QRandomGenerator::global()->bounded(100) / 100.0;
        e.effectSize = QRandomGenerator::global()->bounded(100) / 100.0;
        e.significant = e.pValue < 0.05;
        e.color = e.significant ? QColor(16,185,129) : QColor(59,130,246);
        addEntry(e);
    }
    inputField_->clear();
}

void PaperHypothesisTester::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Test hypotheses");
    update();
}

void PaperHypothesisTester::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Test hypotheses");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Hypothesis Tester");
    int w = width(), h = height();
    drawHypothesisList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperHypothesisTester::drawHypothesisList(QPainter& p, const QRect& rect) {
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
                   e.hypothesis + (e.significant ? " [*]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.testType + " | " + e.category);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "p=" + QString::number(e.pValue, 'f', 3));
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "eff=" + QString::number(e.effectSize, 'f', 2));
    }
}

void PaperHypothesisTester::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"t-test", "chi-square", "anova", "regression"};
    QString labels[] = {"T-test", "Chi-sq", "ANOVA", "Regress"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 100));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 55, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 60, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 63 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperHypothesisTester::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries", QString::number(entries_.size()), QColor(59,130,246)},
        {"Significant", QString::number(significantCount()), QColor(16,185,129)},
        {"Avg Effect", QString::number(avgEffect(), 'f', 2), QColor(245,158,11)},
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

void PaperHypothesisTester::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Test hypotheses"); return; }
    infoLabel_->setText(QString("%1 tests | %2 significant | %3 avg effect")
        .arg(entries_.size()).arg(significantCount()).arg(avgEffect(), 0, 'f', 2));
}

void PaperHypothesisTester::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        HypothesisEntry e;
        e.id = settings_.value("id").toInt();
        e.hypothesis = settings_.value("hypothesis").toString();
        e.category = settings_.value("category").toString();
        e.testType = settings_.value("testType").toString();
        e.pValue = settings_.value("pValue").toDouble();
        e.effectSize = settings_.value("effectSize").toDouble();
        e.significant = settings_.value("significant").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperHypothesisTester::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("hypothesis", entries_[i].hypothesis);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("testType", entries_[i].testType);
        settings_.setValue("pValue", entries_[i].pValue);
        settings_.setValue("effectSize", entries_[i].effectSize);
        settings_.setValue("significant", entries_[i].significant);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
