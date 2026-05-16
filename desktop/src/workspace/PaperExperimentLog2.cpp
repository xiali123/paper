#include "workspace/PaperExperimentLog2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperExperimentLog2::PaperExperimentLog2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ExperimentLog2")
{
    setupUI();
    loadSettings();
}

void PaperExperimentLog2::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    logBtn_ = new QPushButton("Log");
    logBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Baseline", "Treatment", "Control", "Replication", "Validation"});
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
    infoLabel_ = new QLabel("Entries: 0 | Significant: 0 | Avg Confidence: 0.00");
    mainLayout->addWidget(infoLabel_);
    connect(logBtn_, &QPushButton::clicked, this, &PaperExperimentLog2::onLog);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperExperimentLog2::onClear);
}

void PaperExperimentLog2::addEntry(const ExperimentLog2Entry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<ExperimentLog2Entry> PaperExperimentLog2::entries() const { return entries_; }

int PaperExperimentLog2::significantCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.significant) c++;
    return c;
}

qreal PaperExperimentLog2::avgConfidence() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.confidence;
    return sum / entries_.size();
}

QMap<QString, int> PaperExperimentLog2::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperExperimentLog2::onLog() {
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    QStringList categories = {"Baseline", "Treatment", "Control", "Replication", "Validation"};
    QStringList results = {"Positive", "Negative", "Inconclusive", "Pending"};
    int count = 3 + QRandomGenerator::global()->bounded(6);
    for (int i = 0; i < count; ++i) {
        ExperimentLog2Entry e;
        e.id = entries_.size() + 1;
        e.experiment = inputField_->text().trimmed();
        if (e.experiment.isEmpty()) e.experiment = QString("Exp_%1").arg(e.id);
        e.category = categoryCombo_->currentText();
        if (e.category == "All") {
            e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        }
        e.result = results[QRandomGenerator::global()->bounded(results.size())];
        e.confidence = QRandomGenerator::global()->bounded(0.0, 1.0);
        e.trials = 1 + QRandomGenerator::global()->bounded(30);
        e.significant = e.confidence > 0.6;
        e.color = colors[categories.indexOf(e.category) % colors.size()];
        if (e.color == QColor()) e.color = colors[0];
        entries_.append(e);
        emit experimentLogged(e.id, e.confidence);
    }
    updateInfo();
    saveSettings();
    update();
}

void PaperExperimentLog2::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperExperimentLog2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawLogView(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryChart(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperExperimentLog2::drawLogView(QPainter& p, const QRect& rect) {
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
        QString text = QString("%1 | %2 | %3 | Conf: %4 | Trials: %5 | %6")
            .arg(e.experiment, e.category, e.result)
            .arg(QString::number(e.confidence, 'f', 2))
            .arg(e.trials)
            .arg(e.significant ? "Significant" : "Not Significant");
        p.drawText(rect.left() + 14, y + 9, text);
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperExperimentLog2::drawCategoryChart(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts();
    int y = rect.top() + 5;
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Category:");
    y += 18;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    QStringList catOrder = {"Baseline", "Treatment", "Control", "Replication", "Validation"};
    for (int ci = 0; ci < catOrder.size(); ++ci) {
        int val = counts.value(catOrder[ci], 0);
        if (val == 0) continue;
        p.setPen(Qt::NoPen);
        p.setBrush(colors[ci]);
        p.drawRoundedRect(rect.left(), y, qMin(val * 20, rect.width()), 14, 3, 3);
        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + 4, y + 12, QString("%1: %2").arg(catOrder[ci]).arg(val));
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperExperimentLog2::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Significant: %1").arg(significantCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Avg Confidence: %1").arg(QString::number(avgConfidence(), 'f', 3)));
}

void PaperExperimentLog2::updateInfo() {
    infoLabel_->setText(QString("Entries: %1 | Significant: %2 | Avg Confidence: %3")
        .arg(entries_.size()).arg(significantCount())
        .arg(QString::number(avgConfidence(), 'f', 2)));
}

void PaperExperimentLog2::loadSettings() {
    settings_.beginGroup("ExperimentLog2");
    int count = settings_.value("count", 0).toInt();
    if (count == 0) {
        // Seed 8 entries
        QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
        QStringList categories = {"Baseline", "Treatment", "Control", "Replication", "Validation"};
        QStringList experiments = {
            "Token Ablation Study", "Learning Rate Sweep", "Dropout Regularization",
            "Batch Size Comparison", "Augmentation Impact", "Loss Function Variant",
            "Pretrained vs Random Init", "Cross-Validation Run"
        };
        QStringList results = {"Positive", "Negative", "Inconclusive", "Pending"};
        for (int i = 0; i < 8; ++i) {
            ExperimentLog2Entry e;
            e.id = i + 1;
            e.experiment = experiments[i];
            e.category = categories[i % categories.size()];
            e.result = results[i % results.size()];
            e.confidence = 0.35 + (i * 0.08);
            e.trials = 5 + i * 3;
            e.significant = e.confidence > 0.6;
            e.color = colors[i % colors.size()];
            entries_.append(e);
        }
        settings_.endGroup();
        saveSettings();
    } else {
        for (int i = 0; i < count; ++i) {
            ExperimentLog2Entry e;
            e.id = settings_.value(QString("id_%1").arg(i)).toInt();
            e.experiment = settings_.value(QString("experiment_%1").arg(i)).toString();
            e.category = settings_.value(QString("category_%1").arg(i)).toString();
            e.result = settings_.value(QString("result_%1").arg(i)).toString();
            e.confidence = settings_.value(QString("confidence_%1").arg(i)).toDouble();
            e.trials = settings_.value(QString("trials_%1").arg(i)).toInt();
            e.significant = settings_.value(QString("significant_%1").arg(i)).toBool();
            e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
            entries_.append(e);
        }
        settings_.endGroup();
    }
    updateInfo();
}

void PaperExperimentLog2::saveSettings() {
    settings_.beginGroup("ExperimentLog2");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("experiment_%1").arg(i), e.experiment);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("result_%1").arg(i), e.result);
        settings_.setValue(QString("confidence_%1").arg(i), e.confidence);
        settings_.setValue(QString("trials_%1").arg(i), e.trials);
        settings_.setValue(QString("significant_%1").arg(i), e.significant);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
