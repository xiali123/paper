#include "analysis/PaperArgumentStrength.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperArgumentStrength::PaperArgumentStrength(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ArgumentStrength")
{
    setupUI();
    loadSettings();
}

void PaperArgumentStrength::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    evaluateBtn_ = new QPushButton("Evaluate");
    evaluateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(evaluateBtn_, &QPushButton::clicked, this, &PaperArgumentStrength::onEvaluate);
    toolbar->addWidget(evaluateBtn_);
    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Strong", "Moderate", "Weak"});
    toolbar->addWidget(filterCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperArgumentStrength::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter claim to evaluate...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Evaluate argument strength");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperArgumentStrength::addEntry(const ArgumentEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit argumentEvaluated(entry.id, entry.strength);
    update();
}

QList<ArgumentEntry> PaperArgumentStrength::entries() const { return entries_; }

qreal PaperArgumentStrength::avgStrength() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.strength;
    return sum / entries_.size();
}

int PaperArgumentStrength::strongCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.strong) c++;
    return c;
}

QMap<QString, int> PaperArgumentStrength::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperArgumentStrength::onEvaluate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList evidenceTypes = {"empirical", "statistical", "anecdotal", "theoretical", "experimental"};
    QStringList sections = {"introduction", "methodology", "results", "discussion", "conclusion"};
    QStringList categories = {"causal", "correlational", "predictive", "descriptive"};
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        ArgumentEntry e;
        e.id = entries_.size() + 1;
        e.claim = text.left(14) + " claim" + QString::number(i);
        e.strength = 0.1 + QRandomGenerator::global()->bounded(90) / 100.0;
        e.evidenceType = evidenceTypes[QRandomGenerator::global()->bounded(evidenceTypes.size())];
        e.sourceCount = 1 + QRandomGenerator::global()->bounded(20);
        e.consistency = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
        e.section = sections[QRandomGenerator::global()->bounded(sections.size())];
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.strong = e.strength >= 0.7;
        e.color = e.strong ? QColor(16,185,129) : (e.strength >= 0.4 ? QColor(245,158,11) : QColor(239,68,68));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperArgumentStrength::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Evaluate argument strength");
    update();
}

void PaperArgumentStrength::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Evaluate argument strength");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Argument Strength");
    int w = width(), h = height();
    drawArgumentList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawStrengthChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperArgumentStrength::drawArgumentList(QPainter& p, const QRect& rect) {
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
                   e.claim.left(16) + (e.strong ? " [S]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.evidenceType + " | " + QString::number(e.sourceCount) + " src | " + e.section);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.strength * 100, 'f', 0) + "% str");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "cons:" + QString::number(e.consistency, 'f', 2));
    }
}

void PaperArgumentStrength::drawStrengthChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList cats = {"causal", "correlational", "predictive", "descriptive"};
    QString labels[] = {"Causal", "Correl.", "Predict", "Descrip"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 65, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 70, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 73 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperArgumentStrength::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Arguments", QString::number(entries_.size()), QColor(59,130,246)},
        {"Strong", QString::number(strongCount()), QColor(16,185,129)},
        {"Avg Strength", QString::number(avgStrength() * 100, 'f', 0) + "%", QColor(245,158,11)},
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

void PaperArgumentStrength::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Evaluate argument strength"); return; }
    infoLabel_->setText(QString("%1 args | %2 strong | %3% avg")
        .arg(entries_.size()).arg(strongCount()).arg(avgStrength() * 100, 0, 'f', 0));
}

void PaperArgumentStrength::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ArgumentEntry e;
        e.id = settings_.value("id").toInt();
        e.claim = settings_.value("claim").toString();
        e.strength = settings_.value("strength").toDouble();
        e.evidenceType = settings_.value("evidenceType").toString();
        e.sourceCount = settings_.value("sourceCount").toInt();
        e.consistency = settings_.value("consistency").toDouble();
        e.section = settings_.value("section").toString();
        e.category = settings_.value("category").toString();
        e.strong = settings_.value("strong").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperArgumentStrength::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("claim", entries_[i].claim);
        settings_.setValue("strength", entries_[i].strength);
        settings_.setValue("evidenceType", entries_[i].evidenceType);
        settings_.setValue("sourceCount", entries_[i].sourceCount);
        settings_.setValue("consistency", entries_[i].consistency);
        settings_.setValue("section", entries_[i].section);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("strong", entries_[i].strong);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
