#include "tools/PaperUnitConverter.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperUnitConverter::PaperUnitConverter(QWidget* parent)
    : QWidget(parent) {
    setupUI();
    loadSettings();
}

void PaperUnitConverter::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Length", "Weight", "Temperature", "Time", "Data"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Value to convert...");
    convertBtn_ = new QPushButton("Convert", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Conversions: 0 | Favorites: 0 | Last Factor: 0.00", this);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(convertBtn_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);
    connect(convertBtn_, &QPushButton::clicked, this, &PaperUnitConverter::onConvert);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperUnitConverter::onClear);
}

void PaperUnitConverter::addEntry(const ConversionEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<ConversionEntry> PaperUnitConverter::entries() const { return entries_; }

int PaperUnitConverter::favoriteCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.favorite) c++;
    return c;
}

qreal PaperUnitConverter::lastFactor() const {
    if (entries_.isEmpty()) return 0.0;
    return entries_.last().factor;
}

QMap<QString, int> PaperUnitConverter::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperUnitConverter::onConvert() {
    ConversionEntry e;
    e.id = entries_.size() + 1;
    bool ok = false;
    e.inputValue = inputField_->text().trimmed().toDouble(&ok);
    if (!ok) e.inputValue = QRandomGenerator::global()->bounded(1.0, 1000.0);
    e.category = categoryCombo_->currentText();
    QMap<QString, QStringList> unitMap = {
        {"Length", {"mm", "cm", "m", "km", "in", "ft"}},
        {"Weight", {"mg", "g", "kg", "lb", "oz", "ton"}},
        {"Temperature", {"C", "F", "K"}},
        {"Time", {"ms", "s", "min", "h", "day", "week"}},
        {"Data", {"B", "KB", "MB", "GB", "TB", "PB"}}
    };
    QStringList units = unitMap.value(e.category, {"unit_a", "unit_b"});
    e.fromUnit = units[QRandomGenerator::global()->bounded(qMin(units.size(), 3))];
    e.toUnit = units[QRandomGenerator::global()->bounded(units.size())];
    e.factor = QRandomGenerator::global()->bounded(0.001, 1000.0);
    e.outputValue = e.inputValue * e.factor;
    e.favorite = e.factor > 100;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    updateInfo();
    saveSettings();
    emit conversionDone(e.id, e.outputValue);
    update();
}

void PaperUnitConverter::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperUnitConverter::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawConversionList(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryChart(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperUnitConverter::drawConversionList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Conversion History:");
    int y = rect.top() + 20;
    for (int i = 0; i < qMin(entries_.size(), 8); ++i) {
        const auto& e = entries_[i];
        p.setPen(e.color);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.left(), y, 8, 8, 2, 2);
        p.setPen(QColor(0x334155));
        QString text = QString("%1 %2 -> %3 %4 | x%5")
            .arg(QString::number(e.inputValue, 'f', 2), e.fromUnit)
            .arg(QString::number(e.outputValue, 'f', 2), e.toUnit)
            .arg(QString::number(e.factor, 'f', 4));
        p.drawText(rect.left() + 14, y + 9, text);
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperUnitConverter::drawCategoryChart(QPainter& p, const QRect& rect) {
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

void PaperUnitConverter::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Favorites: %1").arg(favoriteCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Last Factor: %1").arg(QString::number(lastFactor(), 'f', 4)));
}

void PaperUnitConverter::updateInfo() {
    infoLabel_->setText(QString("Conversions: %1 | Favorites: %2 | Last Factor: %3")
        .arg(entries_.size()).arg(favoriteCount())
        .arg(QString::number(lastFactor(), 'f', 2)));
}

void PaperUnitConverter::loadSettings() {
    settings_.beginGroup("UnitConverter");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        ConversionEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.fromUnit = settings_.value(QString("fromUnit_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.toUnit = settings_.value(QString("toUnit_%1").arg(i)).toString();
        e.inputValue = settings_.value(QString("inputValue_%1").arg(i)).toDouble();
        e.outputValue = settings_.value(QString("outputValue_%1").arg(i)).toDouble();
        e.factor = settings_.value(QString("factor_%1").arg(i)).toDouble();
        e.favorite = settings_.value(QString("favorite_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperUnitConverter::saveSettings() {
    settings_.beginGroup("UnitConverter");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("fromUnit_%1").arg(i), e.fromUnit);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("toUnit_%1").arg(i), e.toUnit);
        settings_.setValue(QString("inputValue_%1").arg(i), e.inputValue);
        settings_.setValue(QString("outputValue_%1").arg(i), e.outputValue);
        settings_.setValue(QString("factor_%1").arg(i), e.factor);
        settings_.setValue(QString("favorite_%1").arg(i), e.favorite);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
