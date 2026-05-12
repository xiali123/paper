#include "analysis/PaperCausalFinder.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperCausalFinder::PaperCausalFinder(QWidget* parent)
    : QWidget(parent) {
    setupUI();
    loadSettings();
}

void PaperCausalFinder::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Granger", "Pearl", "Rubin", "Structural", "Counterfactual"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Cause variable...");
    findBtn_ = new QPushButton("Find", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Links: 0 | Significant: 0 | Avg Strength: 0.00", this);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(findBtn_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);
    connect(findBtn_, &QPushButton::clicked, this, &PaperCausalFinder::onFind);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCausalFinder::onClear);
}

void PaperCausalFinder::addEntry(const CausalEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<CausalEntry> PaperCausalFinder::entries() const { return entries_; }

int PaperCausalFinder::significantCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.significant) c++;
    return c;
}

qreal PaperCausalFinder::avgStrength() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.strength;
    return sum / entries_.size();
}

QMap<QString, int> PaperCausalFinder::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperCausalFinder::onFind() {
    CausalEntry e;
    e.id = entries_.size() + 1;
    e.cause = inputField_->text().trimmed();
    if (e.cause.isEmpty()) e.cause = QString("Var_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    QStringList effects = {"Citation Growth", "Impact Score", "Collaboration Rate", "Topic Shift", "Publication Volume"};
    e.effect = effects[QRandomGenerator::global()->bounded(effects.size())];
    e.strength = QRandomGenerator::global()->bounded(0.0, 1.0);
    e.confidence = QRandomGenerator::global()->bounded(0.0, 1.0);
    e.lag = QRandomGenerator::global()->bounded(0.0, 12.0);
    e.significant = e.confidence > 0.7 && e.strength > 0.5;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    updateInfo();
    saveSettings();
    emit causalFound(e.id, e.strength);
    update();
}

void PaperCausalFinder::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperCausalFinder::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawCausalList(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryChart(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperCausalFinder::drawCausalList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Causal Links:");
    int y = rect.top() + 20;
    for (int i = 0; i < qMin(entries_.size(), 8); ++i) {
        const auto& e = entries_[i];
        p.setPen(e.color);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.left(), y, 8, 8, 2, 2);
        p.setPen(QColor(0x334155));
        QString text = QString("%1 -> %2 | Str: %3 | Conf: %4 | Lag: %5")
            .arg(e.cause, e.effect)
            .arg(QString::number(e.strength, 'f', 2))
            .arg(QString::number(e.confidence, 'f', 2))
            .arg(QString::number(e.lag, 'f', 1));
        p.drawText(rect.left() + 14, y + 9, text);
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperCausalFinder::drawCategoryChart(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts();
    int y = rect.top() + 5;
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Method:");
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

void PaperCausalFinder::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Significant: %1").arg(significantCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Avg Strength: %1").arg(QString::number(avgStrength(), 'f', 3)));
}

void PaperCausalFinder::updateInfo() {
    infoLabel_->setText(QString("Links: %1 | Significant: %2 | Avg Strength: %3")
        .arg(entries_.size()).arg(significantCount())
        .arg(QString::number(avgStrength(), 'f', 2)));
}

void PaperCausalFinder::loadSettings() {
    settings_.beginGroup("CausalFinder");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        CausalEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.cause = settings_.value(QString("cause_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.effect = settings_.value(QString("effect_%1").arg(i)).toString();
        e.strength = settings_.value(QString("strength_%1").arg(i)).toDouble();
        e.confidence = settings_.value(QString("confidence_%1").arg(i)).toDouble();
        e.lag = settings_.value(QString("lag_%1").arg(i)).toDouble();
        e.significant = settings_.value(QString("significant_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperCausalFinder::saveSettings() {
    settings_.beginGroup("CausalFinder");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("cause_%1").arg(i), e.cause);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("effect_%1").arg(i), e.effect);
        settings_.setValue(QString("strength_%1").arg(i), e.strength);
        settings_.setValue(QString("confidence_%1").arg(i), e.confidence);
        settings_.setValue(QString("lag_%1").arg(i), e.lag);
        settings_.setValue(QString("significant_%1").arg(i), e.significant);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
