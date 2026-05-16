#include "tools/PaperPatchManager.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QDateTime>

PaperPatchManager::PaperPatchManager(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "PatchManager") {
    setupUI();
    loadSettings();
}

void PaperPatchManager::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Security", "Bugfix", "Feature", "Performance", "Compat"});

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Patch name...");

    applyBtn_ = new QPushButton("Apply", this);
    clearBtn_ = new QPushButton("Clear", this);

    infoLabel_ = new QLabel("Patches: 0 | Applied: 0 | Avg Risk: 0.00", this);

    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(applyBtn_);
    toolbar->addWidget(clearBtn_);

    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);

    connect(applyBtn_, &QPushButton::clicked, this, &PaperPatchManager::onApply);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperPatchManager::onClear);
}

void PaperPatchManager::addEntry(const PatchEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<PatchEntry> PaperPatchManager::entries() const {
    return entries_;
}

int PaperPatchManager::appliedCount() const {
    int count = 0;
    for (const auto& e : entries_)
        if (e.applied) ++count;
    return count;
}

qreal PaperPatchManager::avgRisk() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.risk;
    return sum / entries_.size();
}

QMap<QString, int> PaperPatchManager::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperPatchManager::onApply() {
    PatchEntry e;
    e.id = entries_.size() + 1;
    e.name = inputField_->text().trimmed();
    if (e.name.isEmpty())
        e.name = QString("Patch_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    if (e.category == "All")
        e.category = "Bugfix";

    QStringList severities = {"Critical", "High", "Medium", "Low", "Info"};
    e.severity = severities[QRandomGenerator::global()->bounded(severities.size())];
    e.files = QRandomGenerator::global()->bounded(1, 20);
    e.risk = QRandomGenerator::global()->generateDouble();
    e.date = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    e.applied = e.risk < 0.3;

    QList<QColor> colors = {
        QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706),
        QColor(0xdc2626), QColor(0x7c3aed)
    };
    e.color = colors[e.id % colors.size()];

    entries_.append(e);
    updateInfo();
    saveSettings();
    emit patchApplied(e.id, e.risk);
    update();
}

void PaperPatchManager::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperPatchManager::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();

    p.fillRect(rect(), QColor(0xf8fafc));

    drawPatchList(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryChart(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperPatchManager::drawPatchList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Patch List:");

    int y = rect.top() + 20;
    for (int i = 0; i < qMin(entries_.size(), 8); ++i) {
        const auto& e = entries_[i];

        // Color indicator dot
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.left(), y, 8, 8, 2, 2);

        // Row text
        p.setPen(QColor(0x334155));
        QString text = QString("%1 | %2 | %3 | %4 files | risk:%5 | %6")
            .arg(e.name, e.category, e.severity)
            .arg(e.files)
            .arg(e.risk, 0, 'f', 2)
            .arg(e.applied ? "Applied" : "Pending");
        p.drawText(rect.left() + 14, y + 9, text);

        // Status badge
        if (e.applied) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(0x16a34a));
            p.drawRoundedRect(rect.right() - 60, y, 50, 14, 3, 3);
            p.setPen(Qt::white);
            p.setFont(QFont("Sans", 7, QFont::Bold));
            p.drawText(QRect(rect.right() - 60, y, 50, 14), Qt::AlignCenter, "APPLIED");
            p.setFont(QFont("Sans", 9));
            p.setPen(QColor(0x334155));
        }

        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperPatchManager::drawCategoryChart(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts();
    int y = rect.top() + 5;

    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Category:");

    y += 18;

    QList<QColor> colors = {
        QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706),
        QColor(0xdc2626), QColor(0x7c3aed)
    };
    int ci = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        int barWidth = qMin(it.value() * 20, rect.width());
        p.setPen(Qt::NoPen);
        p.setBrush(colors[ci++ % colors.size()]);
        p.drawRoundedRect(rect.left(), y, barWidth, 14, 3, 3);
        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + 4, y + 12,
                    QString("%1: %2").arg(it.key()).arg(it.value()));
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperPatchManager::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");

    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Applied: %1").arg(appliedCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Pending: %1").arg(entries_.size() - appliedCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Avg Risk: %1").arg(avgRisk(), 0, 'f', 2));

    // Risk gauge bar
    y += 20;
    int gaugeW = rect.width() - 10;
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0xe2e8f0));
    p.drawRoundedRect(rect.left(), y, gaugeW, 8, 4, 4);

    qreal riskFrac = qBound(0.0, avgRisk(), 1.0);
    QColor riskColor = riskFrac < 0.3 ? QColor(0x16a34a)
                     : riskFrac < 0.6 ? QColor(0xd97706)
                     : QColor(0xdc2626);
    p.setBrush(riskColor);
    p.drawRoundedRect(rect.left(), y, static_cast<int>(gaugeW * riskFrac), 8, 4, 4);
    p.setBrush(Qt::NoBrush);
}

void PaperPatchManager::updateInfo() {
    infoLabel_->setText(
        QString("Patches: %1 | Applied: %2 | Avg Risk: %3")
            .arg(entries_.size())
            .arg(appliedCount())
            .arg(avgRisk(), 0, 'f', 2));
}

void PaperPatchManager::loadSettings() {
    settings_.beginGroup("PatchManager");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        PatchEntry e;
        e.id       = settings_.value(QString("id_%1").arg(i)).toInt();
        e.name     = settings_.value(QString("name_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.severity = settings_.value(QString("severity_%1").arg(i)).toString();
        e.files    = settings_.value(QString("files_%1").arg(i)).toInt();
        e.risk     = settings_.value(QString("risk_%1").arg(i)).toDouble();
        e.date     = settings_.value(QString("date_%1").arg(i)).toString();
        e.applied  = settings_.value(QString("applied_%1").arg(i)).toBool();
        e.color    = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperPatchManager::saveSettings() {
    settings_.beginGroup("PatchManager");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i),       e.id);
        settings_.setValue(QString("name_%1").arg(i),     e.name);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("severity_%1").arg(i), e.severity);
        settings_.setValue(QString("files_%1").arg(i),    e.files);
        settings_.setValue(QString("risk_%1").arg(i),     e.risk);
        settings_.setValue(QString("date_%1").arg(i),     e.date);
        settings_.setValue(QString("applied_%1").arg(i),  e.applied);
        settings_.setValue(QString("color_%1").arg(i),    e.color.name());
    }
    settings_.endGroup();
}
