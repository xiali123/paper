#include "reading/PaperReadingElevation.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReadingElevation::PaperReadingElevation(QWidget* parent)
    : QWidget(parent) {
    setupUI();
    loadSettings();
}

void PaperReadingElevation::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Theory", "Experiment", "Review", "Survey", "Case Study"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Paper title...");
    measureBtn_ = new QPushButton("Measure", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Entries: 0 | Summits: 0 | Avg Gain: 0.00", this);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(measureBtn_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);
    connect(measureBtn_, &QPushButton::clicked, this, &PaperReadingElevation::onMeasure);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingElevation::onClear);
}

void PaperReadingElevation::addEntry(const ElevEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<ElevEntry> PaperReadingElevation::entries() const { return entries_; }

int PaperReadingElevation::summitCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.summit) c++;
    return c;
}

qreal PaperReadingElevation::avgGain() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.gain;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingElevation::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperReadingElevation::onMeasure() {
    ElevEntry e;
    e.id = entries_.size() + 1;
    e.paper = inputField_->text().trimmed();
    if (e.paper.isEmpty()) e.paper = QString("Paper_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    QStringList tiers = {"Basecamp", "Ridge", "Peak", "Summit"};
    e.tier = tiers[QRandomGenerator::global()->bounded(tiers.size())];
    e.level = QRandomGenerator::global()->bounded(0.0, 100.0);
    e.gain = QRandomGenerator::global()->bounded(0.0, 50.0);
    e.base = QRandomGenerator::global()->bounded(0.0, 50.0);
    e.summit = e.level > 80.0;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    updateInfo();
    saveSettings();
    emit elevationMeasured(e.id, e.gain);
    update();
}

void PaperReadingElevation::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperReadingElevation::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawElevationView(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryChart(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperReadingElevation::drawElevationView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Elevation Profile:");
    if (entries_.isEmpty()) return;

    int y = rect.top() + 20;
    int barMaxW = rect.width() - 120;
    for (int i = 0; i < qMin(entries_.size(), 10); ++i) {
        const auto& e = entries_[i];
        // Base bar (ground)
        int baseW = static_cast<int>(e.base / 100.0 * barMaxW);
        p.setBrush(QColor(0xe2e8f0));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(rect.left(), y, baseW, 14, 3, 3);
        // Gain bar (elevation)
        int gainW = static_cast<int>(e.gain / 100.0 * barMaxW);
        QColor barColor = e.summit ? QColor(0xdc2626) : e.color;
        p.setBrush(barColor);
        p.drawRoundedRect(rect.left() + baseW, y, gainW, 14, 3, 3);
        // Level marker
        int levelX = rect.left() + static_cast<int>(e.level / 100.0 * barMaxW);
        p.setPen(QPen(QColor(0x334155), 2));
        p.drawLine(levelX, y, levelX, y + 14);
        // Label
        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + baseW + gainW + 5, y + 12,
            QString("%1 | %2 | Lv%3").arg(e.paper.left(12), e.tier)
                .arg(QString::number(e.level, 'f', 0)));
        if (e.summit) {
            p.setPen(QColor(0xdc2626));
            p.drawText(levelX - 4, y - 2, QChar(0x26F0)); // mountain emoji
        }
        y += 20;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperReadingElevation::drawCategoryChart(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts();
    int y = rect.top() + 5;
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Category:");
    y += 18;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    int ci = 0;
    int total = entries_.size();
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        int barW = total > 0 ? static_cast<int>(static_cast<qreal>(it.value()) / total * (rect.width() - 20)) : 0;
        p.setBrush(colors[ci++ % colors.size()]);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(rect.left(), y, barW, 14, 3, 3);
        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + 4, y + 12, QString("%1: %2").arg(it.key()).arg(it.value()));
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperReadingElevation::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total Entries: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Summits: %1").arg(summitCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Avg Gain: %1").arg(QString::number(avgGain(), 'f', 1)));
    y += 16;
    p.drawText(rect.left(), y, QString("Categories: %1").arg(categoryCounts().size()));
}

void PaperReadingElevation::updateInfo() {
    infoLabel_->setText(QString("Entries: %1 | Summits: %2 | Avg Gain: %3")
        .arg(entries_.size()).arg(summitCount())
        .arg(QString::number(avgGain(), 'f', 2)));
}

void PaperReadingElevation::loadSettings() {
    settings_.beginGroup("ReadingElevation");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        ElevEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.paper = settings_.value(QString("paper_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.tier = settings_.value(QString("tier_%1").arg(i)).toString();
        e.level = settings_.value(QString("level_%1").arg(i)).toDouble();
        e.gain = settings_.value(QString("gain_%1").arg(i)).toDouble();
        e.base = settings_.value(QString("base_%1").arg(i)).toDouble();
        e.summit = settings_.value(QString("summit_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperReadingElevation::saveSettings() {
    settings_.beginGroup("ReadingElevation");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("paper_%1").arg(i), e.paper);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("tier_%1").arg(i), e.tier);
        settings_.setValue(QString("level_%1").arg(i), e.level);
        settings_.setValue(QString("gain_%1").arg(i), e.gain);
        settings_.setValue(QString("base_%1").arg(i), e.base);
        settings_.setValue(QString("summit_%1").arg(i), e.summit);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
