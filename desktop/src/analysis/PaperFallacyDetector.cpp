#include "analysis/PaperFallacyDetector.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperFallacyDetector::PaperFallacyDetector(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "FallacyDetector")
{
    setupUI();
    loadSettings();
}

void PaperFallacyDetector::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    detectBtn_ = new QPushButton("Detect");
    detectBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(detectBtn_, &QPushButton::clicked, this, &PaperFallacyDetector::onDetect);
    toolbar->addWidget(detectBtn_);
    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Critical", "Warning", "Minor"});
    toolbar->addWidget(filterCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperFallacyDetector::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter text to detect fallacies...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Detect logical fallacies");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperFallacyDetector::addEntry(const FallacyEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit fallacyDetected(entry.id, entry.severity);
    update();
}

QList<FallacyEntry> PaperFallacyDetector::entries() const { return entries_; }

qreal PaperFallacyDetector::avgSeverity() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.severity;
    return sum / entries_.size();
}

int PaperFallacyDetector::criticalCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.critical) c++;
    return c;
}

QMap<QString, int> PaperFallacyDetector::fallacyTypeCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.fallacyType]++;
    return counts;
}

void PaperFallacyDetector::onDetect() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList types = {"ad hominem", "straw man", "false cause", "hasty generalization", "appeal to authority", "circular reasoning"};
    QStringList contexts = {"introduction", "methodology", "results", "discussion", "conclusion"};
    QStringList suggestions = {"provide evidence", "use counter-examples", "verify sources", "check logic chain", "seek peer review"};
    QStringList categories = {"formal", "informal", "statistical", "rhetorical"};
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        FallacyEntry e;
        e.id = entries_.size() + 1;
        e.text = text.left(12) + " seg" + QString::number(i);
        e.fallacyType = types[QRandomGenerator::global()->bounded(types.size())];
        e.severity = 0.2 + QRandomGenerator::global()->bounded(80) / 100.0;
        e.context = contexts[QRandomGenerator::global()->bounded(contexts.size())];
        e.occurrence = 1 + QRandomGenerator::global()->bounded(5);
        e.suggestion = suggestions[QRandomGenerator::global()->bounded(suggestions.size())];
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.critical = e.severity >= 0.7;
        e.color = e.critical ? QColor(239,68,68) : (e.severity >= 0.4 ? QColor(245,158,11) : QColor(59,130,246));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperFallacyDetector::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Detect logical fallacies");
    update();
}

void PaperFallacyDetector::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Detect logical fallacies");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Fallacy Detector");
    int w = width(), h = height();
    drawFallacyList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawTypeChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperFallacyDetector::drawFallacyList(QPainter& p, const QRect& rect) {
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
                   e.fallacyType.left(16) + (e.critical ? " [!]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.context + " | x" + QString::number(e.occurrence));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.severity * 100, 'f', 0) + "% sev");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.suggestion.left(16));
    }
}

void PaperFallacyDetector::drawTypeChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = fallacyTypeCounts();
    QStringList cats = {"formal", "informal", "statistical", "rhetorical"};
    QString labels[] = {"Formal", "Informal", "Statistical", "Rhetorical"};
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

void PaperFallacyDetector::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Fallacies", QString::number(entries_.size()), QColor(59,130,246)},
        {"Critical", QString::number(criticalCount()), QColor(239,68,68)},
        {"Avg Severity", QString::number(avgSeverity() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Types", QString::number(fallacyTypeCounts().size()), QColor(139,92,246)}
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

void PaperFallacyDetector::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Detect logical fallacies"); return; }
    infoLabel_->setText(QString("%1 fallacies | %2 critical | %3% sev")
        .arg(entries_.size()).arg(criticalCount()).arg(avgSeverity() * 100, 0, 'f', 0));
}

void PaperFallacyDetector::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        FallacyEntry e;
        e.id = settings_.value("id").toInt();
        e.text = settings_.value("text").toString();
        e.fallacyType = settings_.value("fallacyType").toString();
        e.severity = settings_.value("severity").toDouble();
        e.context = settings_.value("context").toString();
        e.occurrence = settings_.value("occurrence").toInt();
        e.suggestion = settings_.value("suggestion").toString();
        e.category = settings_.value("category").toString();
        e.critical = settings_.value("critical").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperFallacyDetector::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("text", entries_[i].text);
        settings_.setValue("fallacyType", entries_[i].fallacyType);
        settings_.setValue("severity", entries_[i].severity);
        settings_.setValue("context", entries_[i].context);
        settings_.setValue("occurrence", entries_[i].occurrence);
        settings_.setValue("suggestion", entries_[i].suggestion);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("critical", entries_[i].critical);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
