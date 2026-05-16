#include "analysis/PaperPatternMiner.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperPatternMiner::PaperPatternMiner(QWidget* parent)
    : QWidget(parent) {
    setupUI();
    loadSettings();
}

void PaperPatternMiner::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Sequential", "Co-occurrence", "Periodic", "Trend", "Anomaly"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Pattern name...");
    mineBtn_ = new QPushButton("Mine", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Patterns: 0 | Frequent: 0 | Avg Confidence: 0.00", this);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(mineBtn_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);
    connect(mineBtn_, &QPushButton::clicked, this, &PaperPatternMiner::onMine);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperPatternMiner::onClear);
}

void PaperPatternMiner::addEntry(const PatternEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<PatternEntry> PaperPatternMiner::entries() const { return entries_; }

int PaperPatternMiner::frequentCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.frequent) c++;
    return c;
}

qreal PaperPatternMiner::avgConfidence() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.confidence;
    return sum / entries_.size();
}

QMap<QString, int> PaperPatternMiner::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperPatternMiner::onMine() {
    PatternEntry e;
    e.id = entries_.size() + 1;
    e.pattern = inputField_->text().trimmed();
    if (e.pattern.isEmpty()) e.pattern = QString("Pattern_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    QStringList freqs = {"Daily", "Weekly", "Monthly", "Rare", "Constant"};
    e.frequency = freqs[QRandomGenerator::global()->bounded(freqs.size())];
    e.support = QRandomGenerator::global()->bounded(0.0, 1.0);
    e.confidence = QRandomGenerator::global()->bounded(0.0, 1.0);
    e.occurrences = QRandomGenerator::global()->bounded(1, 1000);
    e.frequent = e.support > 0.5 && e.confidence > 0.7;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    updateInfo();
    saveSettings();
    emit patternMined(e.id, e.confidence);
    update();
}

void PaperPatternMiner::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperPatternMiner::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawPatternList(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryChart(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperPatternMiner::drawPatternList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Discovered Patterns:");
    int y = rect.top() + 20;
    for (int i = 0; i < qMin(entries_.size(), 8); ++i) {
        const auto& e = entries_[i];
        p.setPen(e.color);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.left(), y, 8, 8, 2, 2);
        p.setPen(QColor(0x334155));
        QString text = QString("%1 | %2 | Sup: %3 | Conf: %4 | %5x")
            .arg(e.pattern, e.category)
            .arg(QString::number(e.support, 'f', 2))
            .arg(QString::number(e.confidence, 'f', 2))
            .arg(e.occurrences);
        p.drawText(rect.left() + 14, y + 9, text);
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperPatternMiner::drawCategoryChart(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts();
    int y = rect.top() + 5;
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Type:");
    y += 18;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    int ci = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        p.setBrush(colors[ci++ % colors.size()]);
        p.drawRoundedRect(rect.left(), y, qMin(it.value() * 20, rect.width()), 14, 3, 3);
        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + 4, y + 12, QString("%1: %2").arg(it.key()).arg(it.value()));
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperPatternMiner::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Frequent: %1").arg(frequentCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Avg Confidence: %1").arg(QString::number(avgConfidence(), 'f', 3)));
}

void PaperPatternMiner::updateInfo() {
    infoLabel_->setText(QString("Patterns: %1 | Frequent: %2 | Avg Confidence: %3")
        .arg(entries_.size()).arg(frequentCount())
        .arg(QString::number(avgConfidence(), 'f', 2)));
}

void PaperPatternMiner::loadSettings() {
    settings_.beginGroup("PatternMiner");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        PatternEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.pattern = settings_.value(QString("pattern_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.frequency = settings_.value(QString("frequency_%1").arg(i)).toString();
        e.support = settings_.value(QString("support_%1").arg(i)).toDouble();
        e.confidence = settings_.value(QString("confidence_%1").arg(i)).toDouble();
        e.occurrences = settings_.value(QString("occurrences_%1").arg(i)).toInt();
        e.frequent = settings_.value(QString("frequent_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperPatternMiner::saveSettings() {
    settings_.beginGroup("PatternMiner");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("pattern_%1").arg(i), e.pattern);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("frequency_%1").arg(i), e.frequency);
        settings_.setValue(QString("support_%1").arg(i), e.support);
        settings_.setValue(QString("confidence_%1").arg(i), e.confidence);
        settings_.setValue(QString("occurrences_%1").arg(i), e.occurrences);
        settings_.setValue(QString("frequent_%1").arg(i), e.frequent);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
