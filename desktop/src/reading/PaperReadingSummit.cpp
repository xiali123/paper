#include "reading/PaperReadingSummit.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReadingSummit::PaperReadingSummit(QWidget* parent)
    : QWidget(parent), settings_("PaperCrawler", "ReadingSummit") {
    setupUI();
    loadSettings();
}

void PaperReadingSummit::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    climbBtn_ = new QPushButton("Climb", this);
    climbBtn_->setStyleSheet("QPushButton{background:#3b82f6;color:#fff;padding:6px 14px;border-radius:4px;}");
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Theory", "Experiment", "Review", "Survey", "Case Study"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Peak name...");
    clearBtn_ = new QPushButton("Clear", this);
    clearBtn_->setStyleSheet("QPushButton{background:#dc2626;color:#fff;padding:6px 14px;border-radius:4px;}");
    toolbar->addWidget(climbBtn_);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    infoLabel_ = new QLabel("Peaks: 0 | Summited: 0 | Avg Alt: 0.00", this);
    mainLayout->addWidget(infoLabel_);
    connect(climbBtn_, &QPushButton::clicked, this, &PaperReadingSummit::onClimb);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingSummit::onClear);
}

void PaperReadingSummit::addEntry(const SummitEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<SummitEntry> PaperReadingSummit::entries() const { return entries_; }

int PaperReadingSummit::summitedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.summited) c++;
    return c;
}

qreal PaperReadingSummit::avgAltitude() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.altitude;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingSummit::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperReadingSummit::onClimb() {
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    QStringList peaks = {"Everest", "K2", "Kangchenjunga", "Lhotse", "Makalu", "Cho Oyu", "Dhaulagiri", "Manaslu", "Nanga Parbat", "Annapurna"};
    QStringList approaches = {"South Col", "North Ridge", "West Face", "East Buttress", "Cesen Route", "Abruzzi Spur"};
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        SummitEntry e;
        e.id = entries_.size() + 1;
        e.peak = inputField_->text().trimmed().isEmpty()
            ? peaks[QRandomGenerator::global()->bounded(peaks.size())]
            : inputField_->text().trimmed();
        e.category = categoryCombo_->currentText();
        e.approach = approaches[QRandomGenerator::global()->bounded(approaches.size())];
        e.altitude = 3000.0 + QRandomGenerator::global()->bounded(5500.0);
        e.papers = QRandomGenerator::global()->bounded(20) + 1;
        e.summited = QRandomGenerator::global()->bounded(2) == 0;
        e.color = colors[e.id % colors.size()];
        entries_.append(e);
        emit peakReached(e.id, e.altitude);
    }
    updateInfo();
    saveSettings();
    update();
}

void PaperReadingSummit::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperReadingSummit::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawMountainView(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryChart(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperReadingSummit::drawMountainView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Mountain View:");
    if (entries_.isEmpty()) return;
    int y = rect.top() + 20;
    int barMaxW = rect.width() - 140;
    qreal maxAlt = 8500.0;
    for (int i = 0; i < qMin(entries_.size(), 10); ++i) {
        const auto& e = entries_[i];
        int barW = static_cast<int>(e.altitude / maxAlt * barMaxW);
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(QColor(0xe2e8f0), 1));
        p.drawRect(rect.left(), y, barMaxW, 16);
        p.setPen(Qt::NoPen);
        p.setBrush(e.summited ? QColor(0xdc2626) : e.color);
        p.drawRoundedRect(rect.left(), y, barW, 16, 3, 3);
        p.setPen(QColor(0xffffff));
        p.setFont(QFont("Sans", 7));
        if (barW > 40) p.drawText(rect.left() + 4, y + 12, QString::number(e.altitude, 'f', 0));
        p.setPen(QColor(0x334155));
        p.setFont(QFont("Sans", 8));
        p.drawText(rect.left() + barW + 5, y + 12,
            QString("%1 | %2").arg(e.peak.left(12), e.approach));
        if (e.summited) {
            p.setPen(QColor(0xdc2626));
            p.setFont(QFont("Sans", 10));
            p.drawText(rect.left() + barW + 5, y - 2, QChar(0x26F0));
        }
        y += 22;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperReadingSummit::drawCategoryChart(QPainter& p, const QRect& rect) {
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

void PaperReadingSummit::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total Peaks: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Summited: %1").arg(summitedCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Avg Altitude: %1").arg(QString::number(avgAltitude(), 'f', 1)));
    y += 16;
    p.drawText(rect.left(), y, QString("Categories: %1").arg(categoryCounts().size()));
}

void PaperReadingSummit::updateInfo() {
    infoLabel_->setText(QString("Peaks: %1 | Summited: %2 | Avg Alt: %3")
        .arg(entries_.size()).arg(summitedCount())
        .arg(QString::number(avgAltitude(), 'f', 2)));
}

void PaperReadingSummit::loadSettings() {
    settings_.beginGroup("Summits");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        SummitEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.peak = settings_.value(QString("peak_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.approach = settings_.value(QString("approach_%1").arg(i)).toString();
        e.altitude = settings_.value(QString("altitude_%1").arg(i)).toDouble();
        e.papers = settings_.value(QString("papers_%1").arg(i)).toInt();
        e.summited = settings_.value(QString("summited_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperReadingSummit::saveSettings() {
    settings_.beginGroup("Summits");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("peak_%1").arg(i), e.peak);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("approach_%1").arg(i), e.approach);
        settings_.setValue(QString("altitude_%1").arg(i), e.altitude);
        settings_.setValue(QString("papers_%1").arg(i), e.papers);
        settings_.setValue(QString("summited_%1").arg(i), e.summited);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
