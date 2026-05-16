#include "analysis/PaperBiasMonitor.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperBiasMonitor::PaperBiasMonitor(QWidget* parent)
    : QWidget(parent) {
    setupUI();
    loadSettings();
}

void PaperBiasMonitor::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Selection", "Confirmation", "Publication", "Language", "Geographic"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Source name...");
    detectBtn_ = new QPushButton("Detect", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Biases: 0 | Flagged: 0 | Avg Severity: 0.00", this);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(detectBtn_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);
    connect(detectBtn_, &QPushButton::clicked, this, &PaperBiasMonitor::onDetect);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperBiasMonitor::onClear);
}

void PaperBiasMonitor::addEntry(const BiasEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<BiasEntry> PaperBiasMonitor::entries() const { return entries_; }

int PaperBiasMonitor::flaggedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.flagged) c++;
    return c;
}

qreal PaperBiasMonitor::avgSeverity() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.severity;
    return sum / entries_.size();
}

QMap<QString, int> PaperBiasMonitor::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperBiasMonitor::onDetect() {
    BiasEntry e;
    e.id = entries_.size() + 1;
    e.source = inputField_->text().trimmed();
    if (e.source.isEmpty()) e.source = QString("Source_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    QStringList types = {"Selection", "Confirmation", "Sampling", "Reporting", "Algorithmic"};
    e.biasType = types[QRandomGenerator::global()->bounded(types.size())];
    e.score = QRandomGenerator::global()->bounded(0.0, 1.0);
    e.severity = QRandomGenerator::global()->bounded(0.0, 1.0);
    e.occurrences = QRandomGenerator::global()->bounded(1, 50);
    e.flagged = e.severity > 0.6;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    updateInfo();
    saveSettings();
    emit biasDetected(e.id, e.severity);
    update();
}

void PaperBiasMonitor::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperBiasMonitor::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawBiasList(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryChart(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperBiasMonitor::drawBiasList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Bias Monitor:");
    int y = rect.top() + 20;
    for (int i = 0; i < qMin(entries_.size(), 8); ++i) {
        const auto& e = entries_[i];
        QColor dotColor = e.flagged ? QColor(0xdc2626) : QColor(0x16a34a);
        p.setPen(dotColor);
        p.setBrush(dotColor);
        p.drawEllipse(rect.left(), y, 8, 8);
        p.setPen(QColor(0x334155));
        QString text = QString("%1 | %2 | Score: %3 | Sev: %4 | %5x")
            .arg(e.source, e.biasType)
            .arg(QString::number(e.score, 'f', 2))
            .arg(QString::number(e.severity, 'f', 2))
            .arg(e.occurrences);
        p.drawText(rect.left() + 14, y + 9, text);
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperBiasMonitor::drawCategoryChart(QPainter& p, const QRect& rect) {
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

void PaperBiasMonitor::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Flagged: %1").arg(flaggedCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Avg Severity: %1").arg(QString::number(avgSeverity(), 'f', 3)));
}

void PaperBiasMonitor::updateInfo() {
    infoLabel_->setText(QString("Biases: %1 | Flagged: %2 | Avg Severity: %3")
        .arg(entries_.size()).arg(flaggedCount())
        .arg(QString::number(avgSeverity(), 'f', 2)));
}

void PaperBiasMonitor::loadSettings() {
    settings_.beginGroup("BiasMonitor");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        BiasEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.source = settings_.value(QString("source_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.biasType = settings_.value(QString("biasType_%1").arg(i)).toString();
        e.score = settings_.value(QString("score_%1").arg(i)).toDouble();
        e.severity = settings_.value(QString("severity_%1").arg(i)).toDouble();
        e.occurrences = settings_.value(QString("occurrences_%1").arg(i)).toInt();
        e.flagged = settings_.value(QString("flagged_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperBiasMonitor::saveSettings() {
    settings_.beginGroup("BiasMonitor");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("source_%1").arg(i), e.source);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("biasType_%1").arg(i), e.biasType);
        settings_.setValue(QString("score_%1").arg(i), e.score);
        settings_.setValue(QString("severity_%1").arg(i), e.severity);
        settings_.setValue(QString("occurrences_%1").arg(i), e.occurrences);
        settings_.setValue(QString("flagged_%1").arg(i), e.flagged);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
