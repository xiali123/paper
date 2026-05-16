#include "tools/PaperSideBySideCompare.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

PaperSideBySideCompare::PaperSideBySideCompare(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "SideBySideCompare")
{
    setupUI();
    loadSettings();
}

void PaperSideBySideCompare::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperSideBySideCompare::onAdd);
    toolbar->addWidget(addBtn_);

    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "A Wins", "B Wins", "Tie"});
    connect(filterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperSideBySideCompare::onFilterChanged);
    toolbar->addWidget(filterCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperSideBySideCompare::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Compare papers side by side");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperSideBySideCompare::addComparison(const ComparisonEntry& entry) {
    comparisons_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<ComparisonEntry> PaperSideBySideCompare::comparisons() const { return comparisons_; }

QMap<QString, int> PaperSideBySideCompare::verdictCounts() const {
    QMap<QString, int> counts;
    for (const auto& c : comparisons_) counts[c.verdict]++;
    return counts;
}

int PaperSideBySideCompare::aWins() const {
    int c = 0;
    for (const auto& comp : comparisons_) if (comp.verdict == "A wins") c++;
    return c;
}

int PaperSideBySideCompare::bWins() const {
    int c = 0;
    for (const auto& comp : comparisons_) if (comp.verdict == "B wins") c++;
    return c;
}

void PaperSideBySideCompare::onAdd() {
    bool ok;
    QString paperA = QInputDialog::getText(this, "Compare", "Paper A:", QLineEdit::Normal, "", &ok);
    if (!ok || paperA.isEmpty()) return;
    QString paperB = QInputDialog::getText(this, "Compare", "Paper B:", QLineEdit::Normal, "", &ok);
    if (!ok || paperB.isEmpty()) return;
    QString metric = QInputDialog::getText(this, "Compare", "Metric:", QLineEdit::Normal, "Score", &ok);
    if (!ok) return;

    ComparisonEntry e;
    e.id = comparisons_.size() + 1;
    e.metric = metric;
    e.paperA = paperA;
    e.paperB = paperB;
    e.valueA = QRandomGenerator::global()->bounded(100) / 10.0;
    e.valueB = QRandomGenerator::global()->bounded(100) / 10.0;
    e.category = "general";
    e.colorA = QColor(59,130,246);
    e.colorB = QColor(239,68,68);

    if (e.valueA > e.valueB) e.verdict = "A wins";
    else if (e.valueB > e.valueA) e.verdict = "B wins";
    else e.verdict = "tie";

    addComparison(e);
}

void PaperSideBySideCompare::onFilterChanged(int) { update(); }

void PaperSideBySideCompare::onClear() {
    comparisons_.clear();
    saveSettings();
    infoLabel_->setText("Compare papers side by side");
    update();
}

void PaperSideBySideCompare::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (comparisons_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Compare papers side by side");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Side-by-Side Compare");

    int w = width(), h = height();
    drawComparisonBars(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawVerdictChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperSideBySideCompare::drawComparisonBars(QPainter& p, const QRect& rect) {
    int filterIdx = filterCombo_->currentIndex();
    int show = 0;
    int maxShow = 10;
    int itemH = qMin(32, (rect.height() - 10) / maxShow);
    qreal maxVal = 1;
    for (const auto& c : comparisons_) maxVal = qMax(maxVal, qMax(c.valueA, c.valueB));

    for (int i = comparisons_.size() - 1; i >= 0 && show < maxShow; --i) {
        const auto& c = comparisons_[i];
        if (filterIdx == 1 && c.verdict != "A wins") continue;
        if (filterIdx == 2 && c.verdict != "B wins") continue;
        if (filterIdx == 3 && c.verdict != "tie") continue;

        int y = rect.y() + show * (itemH + 3);
        int halfW = (rect.width() - 40) / 2;
        int midX = rect.x() + halfW + 20;

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + itemH - 2, halfW, itemH, Qt::AlignVCenter | Qt::AlignRight, c.metric.left(10));

        int barWA = static_cast<int>((c.valueA / maxVal) * halfW);
        p.setPen(Qt::NoPen);
        p.setBrush(c.colorA);
        p.drawRoundedRect(midX - barWA, y + 4, barWA, itemH / 2 - 2, 2, 2);

        int barWB = static_cast<int>((c.valueB / maxVal) * halfW);
        p.setBrush(c.colorB);
        p.drawRoundedRect(midX + 2, y + 4, barWB, itemH / 2 - 2, 2, 2);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 6));
        p.drawText(midX - barWA - 3, y + itemH / 2, barWA + 6, itemH / 2, Qt::AlignCenter,
                   QString::number(c.valueA, 'f', 1));
        p.drawText(midX + barWB + 2, y + itemH / 2, barWB + 6, itemH / 2, Qt::AlignLeft,
                   QString::number(c.valueB, 'f', 1));
        show++;
    }
}

void PaperSideBySideCompare::drawVerdictChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Verdicts");

    auto counts = verdictCounts();
    int total = comparisons_.size();
    if (total == 0) return;

    int pieW = qMin(rect.width(), rect.height() - 40);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 20 + pieW / 2;

    QStringList verdicts = {"A wins", "B wins", "tie"};
    QColor colors[] = {QColor(59,130,246), QColor(239,68,68), QColor(100,116,139)};

    qreal startAngle = 0;
    for (int i = 0; i < 3; ++i) {
        int count = counts.contains(verdicts[i]) ? counts[verdicts[i]] : 0;
        qreal span = (static_cast<qreal>(count) / total) * 360;
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawPie(cx - pieW / 2, cy - pieW / 2, pieW, pieW,
                  static_cast<int>(startAngle * 16), static_cast<int>(span * 16));
        startAngle += span;
    }

    p.setBrush(Qt::white);
    p.drawEllipse(cx - pieW / 4, cy - pieW / 4, pieW / 2, pieW / 2);

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(cx - 10, cy + 5, QString::number(total));
}

void PaperSideBySideCompare::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Comparisons", QString::number(comparisons_.size()), QColor(59,130,246)},
        {"A Wins", QString::number(aWins()), QColor(59,130,246)},
        {"B Wins", QString::number(bWins()), QColor(239,68,68)},
        {"Ties", QString::number(verdictCounts().value("tie", 0)), QColor(100,116,139)}
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

void PaperSideBySideCompare::updateInfo() {
    if (comparisons_.isEmpty()) { infoLabel_->setText("Compare papers side by side"); return; }
    infoLabel_->setText(QString("%1 comparisons | A: %2 | B: %3 | tie: %4")
        .arg(comparisons_.size()).arg(aWins()).arg(bWins())
        .arg(verdictCounts().value("tie", 0)));
}

void PaperSideBySideCompare::loadSettings() {
    int size = settings_.beginReadArray("comparisons");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ComparisonEntry e;
        e.id = settings_.value("id").toInt();
        e.metric = settings_.value("metric").toString();
        e.paperA = settings_.value("paperA").toString();
        e.paperB = settings_.value("paperB").toString();
        e.valueA = settings_.value("valueA").toDouble();
        e.valueB = settings_.value("valueB").toDouble();
        e.verdict = settings_.value("verdict").toString();
        e.category = settings_.value("category").toString();
        e.colorA = QColor(settings_.value("colorA").toString());
        e.colorB = QColor(settings_.value("colorB").toString());
        comparisons_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperSideBySideCompare::saveSettings() {
    settings_.beginWriteArray("comparisons");
    for (int i = 0; i < comparisons_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", comparisons_[i].id);
        settings_.setValue("metric", comparisons_[i].metric);
        settings_.setValue("paperA", comparisons_[i].paperA);
        settings_.setValue("paperB", comparisons_[i].paperB);
        settings_.setValue("valueA", comparisons_[i].valueA);
        settings_.setValue("valueB", comparisons_[i].valueB);
        settings_.setValue("verdict", comparisons_[i].verdict);
        settings_.setValue("category", comparisons_[i].category);
        settings_.setValue("colorA", comparisons_[i].colorA.name());
        settings_.setValue("colorB", comparisons_[i].colorB.name());
    }
    settings_.endArray();
}
