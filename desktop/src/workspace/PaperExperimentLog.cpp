#include "workspace/PaperExperimentLog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperExperimentLog::PaperExperimentLog(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ExperimentLog")
{
    setupUI();
    loadSettings();
}

void PaperExperimentLog::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    logBtn_ = new QPushButton("Log");
    logBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Training", "Testing", "Validation", "Benchmark", "Ablation"});
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Experiment name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("QPushButton { background: #dc2626; color: white; padding: 4px 12px; border-radius: 4px; }");
    toolbar->addWidget(logBtn_);
    toolbar->addWidget(categoryCombo_, 1);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    infoLabel_ = new QLabel("Experiments: 0 | Success: 0 | Avg Result: 0.00");
    mainLayout->addWidget(infoLabel_);
    connect(logBtn_, &QPushButton::clicked, this, &PaperExperimentLog::onLog);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperExperimentLog::onClear);
}

void PaperExperimentLog::addEntry(const ExperimentEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<ExperimentEntry> PaperExperimentLog::entries() const { return entries_; }

int PaperExperimentLog::successCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.success) c++;
    return c;
}

qreal PaperExperimentLog::avgResult() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.result;
    return sum / entries_.size();
}

QMap<QString, int> PaperExperimentLog::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperExperimentLog::onLog() {
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    QStringList phases = {"Setup", "Running", "Analysis", "Complete"};
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        ExperimentEntry e;
        e.id = entries_.size() + 1;
        e.name = inputField_->text().trimmed();
        if (e.name.isEmpty()) e.name = QString("Exp_%1").arg(e.id);
        e.category = categoryCombo_->currentText();
        if (e.category == "All") {
            QStringList cats = {"Training", "Testing", "Validation", "Benchmark", "Ablation"};
            e.category = cats[QRandomGenerator::global()->bounded(cats.size())];
        }
        e.phase = phases[QRandomGenerator::global()->bounded(phases.size())];
        e.result = QRandomGenerator::global()->bounded(0.0, 1.0);
        e.runs = 1 + QRandomGenerator::global()->bounded(20);
        e.success = e.result > 0.4;
        e.color = colors[e.id % colors.size()];
        entries_.append(e);
        emit experimentLogged(e.id, e.result);
    }
    updateInfo();
    saveSettings();
    update();
}

void PaperExperimentLog::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperExperimentLog::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawLogView(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryChart(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperExperimentLog::drawLogView(QPainter& p, const QRect& rect) {
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
        QString text = QString("%1 | %2 | Phase: %3 | Result: %4 | Runs: %5 | %6")
            .arg(e.name, e.category, e.phase)
            .arg(QString::number(e.result, 'f', 2))
            .arg(e.runs)
            .arg(e.success ? "Success" : "Failed");
        p.drawText(rect.left() + 14, y + 9, text);
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperExperimentLog::drawCategoryChart(QPainter& p, const QRect& rect) {
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

void PaperExperimentLog::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Success: %1").arg(successCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Avg Result: %1").arg(QString::number(avgResult(), 'f', 3)));
}

void PaperExperimentLog::updateInfo() {
    infoLabel_->setText(QString("Experiments: %1 | Success: %2 | Avg Result: %3")
        .arg(entries_.size()).arg(successCount())
        .arg(QString::number(avgResult(), 'f', 2)));
}

void PaperExperimentLog::loadSettings() {
    settings_.beginGroup("ExperimentLog");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        ExperimentEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.name = settings_.value(QString("name_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.phase = settings_.value(QString("phase_%1").arg(i)).toString();
        e.result = settings_.value(QString("result_%1").arg(i)).toDouble();
        e.runs = settings_.value(QString("runs_%1").arg(i)).toInt();
        e.success = settings_.value(QString("success_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperExperimentLog::saveSettings() {
    settings_.beginGroup("ExperimentLog");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("name_%1").arg(i), e.name);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("phase_%1").arg(i), e.phase);
        settings_.setValue(QString("result_%1").arg(i), e.result);
        settings_.setValue(QString("runs_%1").arg(i), e.runs);
        settings_.setValue(QString("success_%1").arg(i), e.success);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
