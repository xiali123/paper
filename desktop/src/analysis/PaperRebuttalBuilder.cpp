#include "analysis/PaperRebuttalBuilder.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperRebuttalBuilder::PaperRebuttalBuilder(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "RebuttalBuilder")
{
    setupUI();
    loadSettings();
}

void PaperRebuttalBuilder::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    buildBtn_ = new QPushButton("Build");
    buildBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(buildBtn_, &QPushButton::clicked, this, &PaperRebuttalBuilder::onBuild);
    toolbar->addWidget(buildBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Methodology", "Evidence", "Logic", "Data", "Interpretation"});
    toolbar->addWidget(categoryCombo_, 1);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter rebuttal topic...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperRebuttalBuilder::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    infoLabel_ = new QLabel("Build rebuttals");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperRebuttalBuilder::addEntry(const RebuttalEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit rebuttalBuilt(entry.id, entry.strength);
    update();
}

QList<RebuttalEntry> PaperRebuttalBuilder::entries() const { return entries_; }

int PaperRebuttalBuilder::convincingCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.convincing) c++;
    return c;
}

qreal PaperRebuttalBuilder::avgStrength() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.strength;
    return sum / entries_.size();
}

QMap<QString, int> PaperRebuttalBuilder::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperRebuttalBuilder::onBuild() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"Methodology", "Evidence", "Logic", "Data", "Interpretation"};
    QStringList points = {"Weak sample size", "Missing controls", "Correlation != causation",
                          "Cherry-picked data", "Unsupported claims", "Confounding variables",
                          "Overgeneralization", "Selection bias"};
    QStringList counters = {"Replicate with larger cohort", "Add control group",
                            "Perform causal inference", "Use full dataset",
                            "Cite supporting evidence", "Adjust for confounders",
                            "Narrow scope of claims", "Randomize selection"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6),
                       QColor(220,38,38), QColor(124,58,237)};
    int cIdx = categoryCombo_->currentIndex();
    int count = 4 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        RebuttalEntry e;
        e.id = entries_.size() + 1;
        e.point = points[QRandomGenerator::global()->bounded(points.size())];
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.counter = counters[QRandomGenerator::global()->bounded(counters.size())];
        e.strength = 0.2 + QRandomGenerator::global()->bounded(80) / 100.0;
        e.references = QRandomGenerator::global()->bounded(12) + 1;
        e.convincing = e.strength >= 0.7;
        e.color = colors[QRandomGenerator::global()->bounded(5)];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperRebuttalBuilder::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Build rebuttals");
    update();
}

void PaperRebuttalBuilder::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Build rebuttals");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Rebuttal Builder");
    int w = width(), h = height();
    drawRebuttalView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperRebuttalBuilder::drawRebuttalView(QPainter& p, const QRect& rect) {
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
                   e.point + (e.convincing ? " [OK]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.counter + " | " + e.category);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.strength * 100, 'f', 0) + "% str");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.references) + " refs");
    }
}

void PaperRebuttalBuilder::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"Methodology", "Evidence", "Logic", "Data", "Interpretation"};
    QString labels[] = {"Method", "Evidence", "Logic", "Data", "Interpret"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6),
                       QColor(220,38,38), QColor(124,58,237)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(20, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
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

void PaperRebuttalBuilder::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Rebuttals", QString::number(entries_.size()), QColor(59,130,246)},
        {"Convincing", QString::number(convincingCount()), QColor(22,163,74)},
        {"Avg Str", QString::number(avgStrength() * 100, 'f', 0) + "%", QColor(217,119,6)},
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

void PaperRebuttalBuilder::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Build rebuttals"); return; }
    infoLabel_->setText(QString("%1 rebuttals | %2 convincing | %3% avg str")
        .arg(entries_.size()).arg(convincingCount()).arg(avgStrength() * 100, 0, 'f', 0));
}

void PaperRebuttalBuilder::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        RebuttalEntry e;
        e.id = settings_.value("id").toInt();
        e.point = settings_.value("point").toString();
        e.category = settings_.value("category").toString();
        e.counter = settings_.value("counter").toString();
        e.strength = settings_.value("strength").toDouble();
        e.references = settings_.value("references").toInt();
        e.convincing = settings_.value("convincing").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperRebuttalBuilder::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("point", entries_[i].point);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("counter", entries_[i].counter);
        settings_.setValue("strength", entries_[i].strength);
        settings_.setValue("references", entries_[i].references);
        settings_.setValue("convincing", entries_[i].convincing);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
