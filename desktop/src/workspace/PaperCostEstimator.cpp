#include "workspace/PaperCostEstimator.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperCostEstimator::PaperCostEstimator(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CostEstimator")
{
    setupUI();
    loadSettings();
}

void PaperCostEstimator::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    estimateBtn_ = new QPushButton("Estimate");
    estimateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(estimateBtn_, &QPushButton::clicked, this, &PaperCostEstimator::onEstimate);
    toolbar->addWidget(estimateBtn_);
    toolbar->addWidget(new QLabel("Period:"));
    periodCombo_ = new QComboBox();
    periodCombo_->addItems({"Monthly", "Quarterly", "Annual"});
    toolbar->addWidget(periodCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCostEstimator::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter cost item name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Estimate project costs");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperCostEstimator::addEntry(const CostEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit costEstimated(entry.id, entry.estimatedCost);
    update();
}

QList<CostEntry> PaperCostEstimator::entries() const { return entries_; }

qreal PaperCostEstimator::totalEstimated() const {
    qreal t = 0;
    for (const auto& e : entries_) t += e.estimatedCost;
    return t;
}

int PaperCostEstimator::overBudgetCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.overBudget) c++;
    return c;
}

QMap<QString, int> PaperCostEstimator::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperCostEstimator::onEstimate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"licensing", "compute", "storage", "travel", "publishing"};
    QStringList currencies = {"USD", "EUR", "GBP"};
    QStringList periods = {"monthly", "quarterly", "annual"};
    int pIdx = periodCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        CostEntry e;
        e.id = entries_.size() + 1;
        e.itemName = text.left(10) + " item" + QString::number(i);
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.estimatedCost = 100 + QRandomGenerator::global()->bounded(10000);
        e.actualCost = e.estimatedCost * (0.7 + QRandomGenerator::global()->bounded(60) / 100.0);
        e.currency = currencies[QRandomGenerator::global()->bounded(currencies.size())];
        e.period = periods[pIdx];
        e.variance = (e.actualCost - e.estimatedCost) / e.estimatedCost;
        e.overBudget = e.actualCost > e.estimatedCost;
        e.color = e.overBudget ? QColor(239,68,68) : (e.variance < -0.1 ? QColor(16,185,129) : QColor(59,130,246));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperCostEstimator::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Estimate project costs");
    update();
}

void PaperCostEstimator::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Estimate project costs");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Cost Estimator");
    int w = width(), h = height();
    drawCostList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperCostEstimator::drawCostList(QPainter& p, const QRect& rect) {
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
                   e.itemName.left(14) + (e.overBudget ? " [OVER]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + e.period + " | " + e.currency);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.estimatedCost, 'f', 0) + " est");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.actualCost, 'f', 0) + " act | " + QString::number(e.variance * 100, 'f', 0) + "%");
    }
}

void PaperCostEstimator::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList cats = {"licensing", "compute", "storage", "travel", "publishing"};
    QString labels[] = {"License", "Compute", "Storage", "Travel", "Publish"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246), QColor(239,68,68)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(20, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
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

void PaperCostEstimator::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Items", QString::number(entries_.size()), QColor(59,130,246)},
        {"Over Budget", QString::number(overBudgetCount()), QColor(239,68,68)},
        {"Total Est", QString::number(totalEstimated(), 'f', 0), QColor(245,158,11)},
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

void PaperCostEstimator::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Estimate project costs"); return; }
    infoLabel_->setText(QString("%1 items | %2 over | %3 est")
        .arg(entries_.size()).arg(overBudgetCount()).arg(totalEstimated(), 0, 'f', 0));
}

void PaperCostEstimator::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        CostEntry e;
        e.id = settings_.value("id").toInt();
        e.itemName = settings_.value("itemName").toString();
        e.category = settings_.value("category").toString();
        e.estimatedCost = settings_.value("estimatedCost").toDouble();
        e.actualCost = settings_.value("actualCost").toDouble();
        e.currency = settings_.value("currency").toString();
        e.period = settings_.value("period").toString();
        e.variance = settings_.value("variance").toDouble();
        e.overBudget = settings_.value("overBudget").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperCostEstimator::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("itemName", entries_[i].itemName);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("estimatedCost", entries_[i].estimatedCost);
        settings_.setValue("actualCost", entries_[i].actualCost);
        settings_.setValue("currency", entries_[i].currency);
        settings_.setValue("period", entries_[i].period);
        settings_.setValue("variance", entries_[i].variance);
        settings_.setValue("overBudget", entries_[i].overBudget);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
