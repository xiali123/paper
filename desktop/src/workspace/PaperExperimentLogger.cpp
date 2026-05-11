#include "workspace/PaperExperimentLogger.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperExperimentLogger::PaperExperimentLogger(QWidget* parent)
    : QWidget(parent) {
    setupUI();
    loadSettings();
}

void PaperExperimentLogger::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Training", "Testing", "Validation", "Benchmark", "Ablation"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Experiment name...");
    logBtn_ = new QPushButton("Log", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Experiments: 0 | Success: 0 | Avg Confidence: 0.00", this);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(logBtn_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);
    connect(logBtn_, &QPushButton::clicked, this, &PaperExperimentLogger::onLog);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperExperimentLogger::onClear);
}

void PaperExperimentLogger::addEntry(const ExpEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<ExpEntry> PaperExperimentLogger::entries() const { return entries_; }

int PaperExperimentLogger::successCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.success) c++;
    return c;
}

qreal PaperExperimentLogger::avgConfidence() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.confidence;
    return sum / entries_.size();
}

QMap<QString, int> PaperExperimentLogger::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperExperimentLogger::onLog() {
    ExpEntry e;
    e.id = entries_.size() + 1;
    e.name = inputField_->text().trimmed();
    if (e.name.isEmpty()) e.name = QString("Exp_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    QStringList statuses = {"Running", "Completed", "Failed", "Paused"};
    e.status = statuses[QRandomGenerator::global()->bounded(statuses.size())];
    QStringList results = {"Improved", "Baseline", "Degraded", "Inconclusive"};
    e.result = results[QRandomGenerator::global()->bounded(results.size())];
    e.confidence = QRandomGenerator::global()->bounded(0.0, 1.0);
    e.success = e.confidence > 0.5;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    updateInfo();
    saveSettings();
    emit experimentLogged(e.id, e.confidence);
    update();
}

void PaperExperimentLogger::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperExperimentLogger::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawExpList(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryChart(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperExperimentLogger::drawExpList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Experiment Log:");
    int y = rect.top() + 20;
    for (int i = 0; i < qMin(entries_.size(), 8); ++i) {
        const auto& e = entries_[i];
        p.setPen(e.color);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.left(), y, 8, 8, 2, 2);
        p.setPen(QColor(0x334155));
        QString text = QString("%1 | %2 | Status: %3 | Conf: %4 | %5")
            .arg(e.name, e.category, e.status)
            .arg(QString::number(e.confidence, 'f', 2))
            .arg(e.success ? "Success" : "Failed");
        p.drawText(rect.left() + 14, y + 9, text);
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperExperimentLogger::drawCategoryChart(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts();
    int y = rect.top() + 5;
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Category:");
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

void PaperExperimentLogger::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Success: %1").arg(successCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Avg Confidence: %1").arg(QString::number(avgConfidence(), 'f', 3)));
}

void PaperExperimentLogger::updateInfo() {
    infoLabel_->setText(QString("Experiments: %1 | Success: %2 | Avg Confidence: %3")
        .arg(entries_.size()).arg(successCount())
        .arg(QString::number(avgConfidence(), 'f', 2)));
}

void PaperExperimentLogger::loadSettings() {
    settings_.beginGroup("ExperimentLogger");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        ExpEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.name = settings_.value(QString("name_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.status = settings_.value(QString("status_%1").arg(i)).toString();
        e.result = settings_.value(QString("result_%1").arg(i)).toString();
        e.confidence = settings_.value(QString("confidence_%1").arg(i)).toDouble();
        e.success = settings_.value(QString("success_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperExperimentLogger::saveSettings() {
    settings_.beginGroup("ExperimentLogger");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("name_%1").arg(i), e.name);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("status_%1").arg(i), e.status);
        settings_.setValue(QString("result_%1").arg(i), e.result);
        settings_.setValue(QString("confidence_%1").arg(i), e.confidence);
        settings_.setValue(QString("success_%1").arg(i), e.success);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
