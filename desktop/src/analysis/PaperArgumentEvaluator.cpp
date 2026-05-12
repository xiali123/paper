#include "analysis/PaperArgumentEvaluator.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>

PaperArgumentEvaluator::PaperArgumentEvaluator(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ArgumentEvaluator")
{
    setupUI();
    loadSettings();
}

void PaperArgumentEvaluator::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Empirical", "Logical", "Statistical", "Analogical", "Authority", "Causal"});
    toolbar->addWidget(categoryCombo_);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter claim...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);
    evaluateBtn_ = new QPushButton("Evaluate");
    evaluateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(evaluateBtn_, &QPushButton::clicked, this, &PaperArgumentEvaluator::onEvaluate);
    toolbar->addWidget(evaluateBtn_);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperArgumentEvaluator::onClear);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();
    layout->addLayout(toolbar);
    infoLabel_ = new QLabel("Evaluate argument strength");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(640, 520);
}

void PaperArgumentEvaluator::addEntry(const ArgumentEvalEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit argumentEvaluated(entry.id, entry.strength);
    update();
}

QList<ArgumentEvalEntry> PaperArgumentEvaluator::entries() const { return entries_; }

int PaperArgumentEvaluator::convincingCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.convincing) c++;
    return c;
}

qreal PaperArgumentEvaluator::avgStrength() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.strength;
    return sum / entries_.size();
}

QMap<QString, int> PaperArgumentEvaluator::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperArgumentEvaluator::onEvaluate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    ArgumentEvalEntry e;
    e.id = entries_.size() + 1;
    e.claim = text;
    e.category = categoryCombo_->currentText() == "All"
        ? QStringList{"Empirical","Logical","Statistical","Analogical","Authority","Causal"}[QRandomGenerator::global()->bounded(6)]
        : categoryCombo_->currentText();
    e.strength = QRandomGenerator::global()->bounded(1000) / 1000.0;
    e.evidence = 1 + QRandomGenerator::global()->bounded(20);
    e.convincing = e.strength > 0.7;
    QStringList stances = {"pro", "con", "neutral"};
    e.stance = stances[QRandomGenerator::global()->bounded(3)];
    e.color = e.convincing ? QColor(22, 163, 74) : QColor(220, 38, 38);
    addEntry(e);
    inputField_->clear();
}

void PaperArgumentEvaluator::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Evaluate argument strength");
    update();
}

void PaperArgumentEvaluator::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Evaluate argument strength");
        return;
    }
    int w = width(), h = height();
    int colW = w / 3;
    drawArgumentMatrix(p, QRect(10, 10, colW - 15, h - 20));
    drawCategoryChart(p, QRect(colW + 5, 10, colW - 15, h - 20));
    drawStats(p, QRect(2 * colW + 5, 10, colW - 15, h - 20));
}

void PaperArgumentEvaluator::drawArgumentMatrix(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 18, "Argument Evaluator");
    int show = qMin(10, entries_.size());
    int itemH = qMin(38, (rect.height() - 40) / qMax(show, 1));
    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + 30 + i * (itemH + 4);
        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);
        // Left accent bar
        p.setBrush(e.convincing ? QColor(22, 163, 74) : QColor(220, 38, 38));
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);
        // Strength bar
        int barW = static_cast<int>(e.strength * (rect.width() - 90));
        p.setBrush(e.convincing ? QColor(22, 163, 74) : QColor(220, 38, 38));
        p.setOpacity(0.3);
        p.drawRoundedRect(rect.x() + 8, y + itemH - 10, barW, 6, 3, 3);
        p.setOpacity(1.0);
        // Claim text
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 2, rect.width() - 70, 16, Qt::AlignVCenter,
                   e.claim.left(20) + " [" + e.stance + "]");
        // Strength and evidence
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + rect.width() - 80, y + 2, 75, 16, Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.strength * 100, 'f', 0) + "% | Ev:" + QString::number(e.evidence));
    }
}

void PaperArgumentEvaluator::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 18, "Categories");
    auto counts = categoryCounts();
    if (counts.isEmpty()) return;
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    QList<QString> keys = counts.keys();
    int barH = qMin(24, (rect.height() - 40) / qMax(keys.size(), 1));
    QColor palette[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6),
                        QColor(220,38,38), QColor(124,58,237), QColor(6,182,212)};
    for (int i = 0; i < keys.size(); ++i) {
        int y = rect.y() + 30 + i * (barH + 4);
        int count = counts[keys[i]];
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 65, barH, Qt::AlignRight | Qt::AlignVCenter, keys[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(palette[i % 6]);
        p.drawRoundedRect(rect.x() + 70, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 73 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperArgumentEvaluator::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 18, "Statistics");
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Arguments", QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Convincing", QString::number(convincingCount()), QColor(22, 163, 74)},
        {"Avg Strength", QString::number(avgStrength(), 'f', 2), QColor(217, 119, 6)}
    };
    int boxH = qMin(50, (rect.height() - 40) / qMax(stats.size(), 1));
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + 30 + i * (boxH + 8);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 16, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 26, Qt::AlignVCenter, stats[i].value);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 30, rect.width() - 20, 16, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperArgumentEvaluator::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Evaluate argument strength"); return; }
    infoLabel_->setText(QString("Arguments: %1 | Convincing: %2 | Avg Strength: %3")
        .arg(entries_.size())
        .arg(convincingCount())
        .arg(avgStrength(), 0, 'f', 2));
}

void PaperArgumentEvaluator::loadSettings() {
    settings_.beginGroup("ArgumentEvaluator");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ArgumentEvalEntry e;
        e.id = settings_.value("id").toInt();
        e.claim = settings_.value("claim").toString();
        e.category = settings_.value("category").toString();
        e.stance = settings_.value("stance").toString();
        e.strength = settings_.value("strength").toDouble();
        e.evidence = settings_.value("evidence").toInt();
        e.convincing = settings_.value("convincing").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperArgumentEvaluator::saveSettings() {
    settings_.beginGroup("ArgumentEvaluator");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("claim", entries_[i].claim);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("stance", entries_[i].stance);
        settings_.setValue("strength", entries_[i].strength);
        settings_.setValue("evidence", entries_[i].evidence);
        settings_.setValue("convincing", entries_[i].convincing);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
