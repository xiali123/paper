#include "analysis/PaperBiasChecker.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperBiasChecker::PaperBiasChecker(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "BiasChecker")
{
    setupUI();
    loadSettings();
}

void PaperBiasChecker::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    checkBtn_ = new QPushButton("Check");
    checkBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(checkBtn_, &QPushButton::clicked, this, &PaperBiasChecker::onCheck);
    toolbar->addWidget(checkBtn_);
    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "High", "Medium", "Low"});
    toolbar->addWidget(filterCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperBiasChecker::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter text to check bias...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Check paper bias");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperBiasChecker::addEntry(const BiasEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit biasDetected(entry.id, entry.score);
    update();
}

QList<BiasEntry> PaperBiasChecker::entries() const { return entries_; }

qreal PaperBiasChecker::avgScore() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.score;
    return sum / entries_.size();
}

int PaperBiasChecker::flaggedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.flagged) c++;
    return c;
}

QMap<QString, int> PaperBiasChecker::biasTypeCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperBiasChecker::onCheck() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList types = {"selection bias", "confirmation bias", "publication bias", "sampling bias", "reporting bias", "attrition bias"};
    QStringList contexts = {"abstract", "methodology", "data collection", "analysis", "conclusion"};
    QStringList suggestions = {"randomize samples", "blind review", "increase sample size", "pre-register study", "use control group"};
    QStringList categories = {"methodological", "cognitive", "statistical", "reporting"};
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        BiasEntry e;
        e.id = entries_.size() + 1;
        e.text = text.left(12) + " seg" + QString::number(i);
        e.biasType = types[QRandomGenerator::global()->bounded(types.size())];
        e.score = 0.1 + QRandomGenerator::global()->bounded(90) / 100.0;
        e.context = contexts[QRandomGenerator::global()->bounded(contexts.size())];
        e.occurrence = 1 + QRandomGenerator::global()->bounded(5);
        e.suggestion = suggestions[QRandomGenerator::global()->bounded(suggestions.size())];
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.flagged = e.score >= 0.6;
        e.color = e.flagged ? QColor(239,68,68) : (e.score >= 0.3 ? QColor(245,158,11) : QColor(59,130,246));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperBiasChecker::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Check paper bias");
    update();
}

void PaperBiasChecker::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Check paper bias");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Bias Checker");
    int w = width(), h = height();
    drawBiasList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawTypeChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperBiasChecker::drawBiasList(QPainter& p, const QRect& rect) {
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
                   e.biasType.left(16) + (e.flagged ? " [!]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.context + " | x" + QString::number(e.occurrence));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.score * 100, 'f', 0) + "% bias");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.suggestion.left(16));
    }
}

void PaperBiasChecker::drawTypeChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = biasTypeCounts();
    QStringList cats = {"methodological", "cognitive", "statistical", "reporting"};
    QString labels[] = {"Method", "Cognitive", "Statistical", "Report"};
    QColor colors[] = {QColor(239,68,68), QColor(245,158,11), QColor(59,130,246), QColor(139,92,246)};
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

void PaperBiasChecker::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Biases", QString::number(entries_.size()), QColor(59,130,246)},
        {"Flagged", QString::number(flaggedCount()), QColor(239,68,68)},
        {"Avg Score", QString::number(avgScore() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Categories", QString::number(biasTypeCounts().size()), QColor(139,92,246)}
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

void PaperBiasChecker::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Check paper bias"); return; }
    infoLabel_->setText(QString("%1 biases | %2 flagged | %3% avg")
        .arg(entries_.size()).arg(flaggedCount()).arg(avgScore() * 100, 0, 'f', 0));
}

void PaperBiasChecker::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        BiasEntry e;
        e.id = settings_.value("id").toInt();
        e.text = settings_.value("text").toString();
        e.biasType = settings_.value("biasType").toString();
        e.score = settings_.value("score").toDouble();
        e.context = settings_.value("context").toString();
        e.occurrence = settings_.value("occurrence").toInt();
        e.suggestion = settings_.value("suggestion").toString();
        e.category = settings_.value("category").toString();
        e.flagged = settings_.value("flagged").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperBiasChecker::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("text", entries_[i].text);
        settings_.setValue("biasType", entries_[i].biasType);
        settings_.setValue("score", entries_[i].score);
        settings_.setValue("context", entries_[i].context);
        settings_.setValue("occurrence", entries_[i].occurrence);
        settings_.setValue("suggestion", entries_[i].suggestion);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("flagged", entries_[i].flagged);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
