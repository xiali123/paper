#include "analysis/PaperArgumentChain.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperArgumentChain::PaperArgumentChain(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ArgumentChain")
{
    setupUI();
    loadSettings();
}

void PaperArgumentChain::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    analyzeBtn_ = new QPushButton("Analyze");
    analyzeBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(analyzeBtn_, &QPushButton::clicked, this, &PaperArgumentChain::onAnalyze);
    toolbar->addWidget(analyzeBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Deductive", "Inductive", "Abductive", "Analogical"});
    toolbar->addWidget(categoryCombo_, 1);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter argument chain text...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 2);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperArgumentChain::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    infoLabel_ = new QLabel("Analyze argument chains");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperArgumentChain::addEntry(const ChainEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit chainComplete(entry.id, entry.strength);
    update();
}

QList<ChainEntry> PaperArgumentChain::entries() const { return entries_; }

int PaperArgumentChain::validCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.valid) c++;
    return c;
}

qreal PaperArgumentChain::avgStrength() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.strength;
    return sum / entries_.size();
}

QMap<QString, int> PaperArgumentChain::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperArgumentChain::onAnalyze() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"Deductive", "Inductive", "Abductive", "Analogical"};
    QStringList links = {"therefore", "because", "hence", "implies", "suggests"};
    int cIdx = categoryCombo_->currentIndex();
    int count = 4 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        ChainEntry e;
        e.id = entries_.size() + 1;
        e.premise = text.left(8) + " prem" + QString::number(i);
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.link = links[QRandomGenerator::global()->bounded(links.size())];
        e.strength = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
        e.steps = 2 + QRandomGenerator::global()->bounded(6);
        e.valid = e.strength >= 0.6 && e.steps >= 3;
        QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
        e.color = colors[QRandomGenerator::global()->bounded(5)];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperArgumentChain::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Analyze argument chains");
    update();
}

void PaperArgumentChain::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Analyze argument chains");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Argument Chain");
    int w = width(), h = height();
    drawChainDiagram(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperArgumentChain::drawChainDiagram(QPainter& p, const QRect& rect) {
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
                   e.premise.left(10) + " -> " + e.link.left(6) + (e.valid ? " [V]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + QString::number(e.steps) + " steps");
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.strength * 100, 'f', 0) + "%");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.valid ? "valid" : "invalid");
    }
}

void PaperArgumentChain::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"Deductive", "Inductive", "Abductive", "Analogical"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 120));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 75, barH, Qt::AlignRight | Qt::AlignVCenter, categories[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 80, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 83 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperArgumentChain::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Chains", QString::number(entries_.size()), QColor(59,130,246)},
        {"Valid", QString::number(validCount()), QColor(22,163,74)},
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

void PaperArgumentChain::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Analyze argument chains"); return; }
    infoLabel_->setText(QString("%1 chains | %2 valid | %3% strength")
        .arg(entries_.size()).arg(validCount()).arg(avgStrength() * 100, 0, 'f', 0));
}

void PaperArgumentChain::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ChainEntry e;
        e.id = settings_.value("id").toInt();
        e.premise = settings_.value("premise").toString();
        e.category = settings_.value("category").toString();
        e.link = settings_.value("link").toString();
        e.strength = settings_.value("strength").toDouble();
        e.steps = settings_.value("steps").toInt();
        e.valid = settings_.value("valid").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperArgumentChain::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("premise", entries_[i].premise);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("link", entries_[i].link);
        settings_.setValue("strength", entries_[i].strength);
        settings_.setValue("steps", entries_[i].steps);
        settings_.setValue("valid", entries_[i].valid);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
