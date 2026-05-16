#include "analysis/PaperAssumptionMap.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperAssumptionMap::PaperAssumptionMap(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "AssumptionMap")
{
    setupUI();
    loadSettings();
}

void PaperAssumptionMap::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    mapBtn_ = new QPushButton("Map");
    mapBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(mapBtn_, &QPushButton::clicked, this, &PaperAssumptionMap::onMap);
    toolbar->addWidget(mapBtn_);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Methodological", "Theoretical", "Statistical", "Empirical"});
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter assumption text to map...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperAssumptionMap::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Map paper assumptions");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperAssumptionMap::addEntry(const AssumptionEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit assumptionChecked(entry.id, entry.risk);
    update();
}

QList<AssumptionEntry> PaperAssumptionMap::entries() const { return entries_; }

int PaperAssumptionMap::validatedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.validated) c++;
    return c;
}

qreal PaperAssumptionMap::avgRisk() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.risk;
    return sum / entries_.size();
}

QMap<QString, int> PaperAssumptionMap::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperAssumptionMap::onMap() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"Methodological", "Theoretical", "Statistical", "Empirical"};
    QStringList impacts = {"critical", "moderate", "low", "negligible"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};

    int count = 4 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        AssumptionEntry e;
        e.id = entries_.size() + 1;
        e.assumption = text.left(12) + " asm" + QString::number(i);
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.impact = impacts[QRandomGenerator::global()->bounded(impacts.size())];
        e.risk = 0.1 + QRandomGenerator::global()->bounded(90) / 100.0;
        e.dependencies = QRandomGenerator::global()->bounded(12);
        e.validated = e.risk <= 0.4;
        e.color = colors[QRandomGenerator::global()->bounded(5)];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperAssumptionMap::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Map paper assumptions");
    update();
}

void PaperAssumptionMap::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Map paper assumptions");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Assumption Map");

    int w = width(), h = height();
    drawAssumptionGraph(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperAssumptionMap::drawAssumptionGraph(QPainter& p, const QRect& rect) {
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
                   e.assumption.left(14) + (e.validated ? " [OK]" : ""));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + QString::number(e.dependencies) + " deps");
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.risk * 100, 'f', 0) + "% risk");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.impact.left(14));
    }
}

void PaperAssumptionMap::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList categories = {"Methodological", "Theoretical", "Statistical", "Empirical"};
    QString labels[] = {"Methodological", "Theoretical", "Statistical", "Empirical"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(124,58,237)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 120));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 75, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 80, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 83 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperAssumptionMap::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Assumptions", QString::number(entries_.size()), QColor(59,130,246)},
        {"Validated", QString::number(validatedCount()), QColor(22,163,74)},
        {"Avg Risk", QString::number(avgRisk() * 100, 'f', 0) + "%", QColor(217,119,6)},
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

void PaperAssumptionMap::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Map paper assumptions"); return; }
    infoLabel_->setText(QString("%1 assumptions | %2 validated | %3% avg risk")
        .arg(entries_.size()).arg(validatedCount()).arg(avgRisk() * 100, 0, 'f', 0));
}

void PaperAssumptionMap::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        AssumptionEntry e;
        e.id = settings_.value("id").toInt();
        e.assumption = settings_.value("assumption").toString();
        e.category = settings_.value("category").toString();
        e.impact = settings_.value("impact").toString();
        e.risk = settings_.value("risk").toDouble();
        e.dependencies = settings_.value("dependencies").toInt();
        e.validated = settings_.value("validated").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperAssumptionMap::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("assumption", entries_[i].assumption);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("impact", entries_[i].impact);
        settings_.setValue("risk", entries_[i].risk);
        settings_.setValue("dependencies", entries_[i].dependencies);
        settings_.setValue("validated", entries_[i].validated);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
