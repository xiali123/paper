#include "reading/PaperReadingThermometer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReadingThermometer::PaperReadingThermometer(QWidget* parent)
    : QWidget(parent) {
    setupUI();
    loadSettings();
}

void PaperReadingThermometer::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Introduction", "Method", "Results", "Discussion", "Conclusion"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Paper title...");
    measureBtn_ = new QPushButton("Measure", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Papers: 0 | Hot: 0 | Avg Temp: 0.00", this);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(measureBtn_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);
    connect(measureBtn_, &QPushButton::clicked, this, &PaperReadingThermometer::onMeasure);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingThermometer::onClear);
}

void PaperReadingThermometer::addEntry(const ThermEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<ThermEntry> PaperReadingThermometer::entries() const { return entries_; }

int PaperReadingThermometer::hotCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.hot) c++;
    return c;
}

qreal PaperReadingThermometer::avgTemperature() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.temperature;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingThermometer::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperReadingThermometer::onMeasure() {
    ThermEntry e;
    e.id = entries_.size() + 1;
    e.paper = inputField_->text().trimmed();
    if (e.paper.isEmpty()) e.paper = QString("Paper_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    QStringList levels = {"Cold", "Warm", "Hot", "Boiling"};
    e.level = levels[QRandomGenerator::global()->bounded(levels.size())];
    e.progress = QRandomGenerator::global()->bounded(0.0, 1.0);
    e.temperature = QRandomGenerator::global()->bounded(0.0, 100.0);
    e.pages = QRandomGenerator::global()->bounded(5, 50);
    e.hot = e.temperature > 70;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    updateInfo();
    saveSettings();
    emit readingMeasured(e.id, e.temperature);
    update();
}

void PaperReadingThermometer::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperReadingThermometer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawThermList(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryChart(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperReadingThermometer::drawThermList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Reading Thermometer:");
    int y = rect.top() + 20;
    for (int i = 0; i < qMin(entries_.size(), 8); ++i) {
        const auto& e = entries_[i];
        // Draw thermometer bar
        int barW = static_cast<int>(e.temperature / 100.0 * (rect.width() - 100));
        QColor barColor = e.hot ? QColor(0xdc2626) : (e.temperature > 40 ? QColor(0xd97706) : QColor(0x3b82f6));
        p.setBrush(barColor);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(rect.left(), y, barW, 12, 3, 3);
        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + barW + 5, y + 11, QString("%1 | %2 | %3°C")
            .arg(e.paper.left(15), e.level)
            .arg(QString::number(e.temperature, 'f', 1)));
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperReadingThermometer::drawCategoryChart(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts();
    int y = rect.top() + 5;
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Section:");
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

void PaperReadingThermometer::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Hot Papers: %1").arg(hotCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Avg Temp: %1").arg(QString::number(avgTemperature(), 'f', 1)));
}

void PaperReadingThermometer::updateInfo() {
    infoLabel_->setText(QString("Papers: %1 | Hot: %2 | Avg Temp: %3")
        .arg(entries_.size()).arg(hotCount())
        .arg(QString::number(avgTemperature(), 'f', 1)));
}

void PaperReadingThermometer::loadSettings() {
    settings_.beginGroup("ReadingThermometer");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        ThermEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.paper = settings_.value(QString("paper_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.level = settings_.value(QString("level_%1").arg(i)).toString();
        e.progress = settings_.value(QString("progress_%1").arg(i)).toDouble();
        e.temperature = settings_.value(QString("temperature_%1").arg(i)).toDouble();
        e.pages = settings_.value(QString("pages_%1").arg(i)).toInt();
        e.hot = settings_.value(QString("hot_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperReadingThermometer::saveSettings() {
    settings_.beginGroup("ReadingThermometer");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("paper_%1").arg(i), e.paper);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("level_%1").arg(i), e.level);
        settings_.setValue(QString("progress_%1").arg(i), e.progress);
        settings_.setValue(QString("temperature_%1").arg(i), e.temperature);
        settings_.setValue(QString("pages_%1").arg(i), e.pages);
        settings_.setValue(QString("hot_%1").arg(i), e.hot);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
