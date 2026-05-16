#include "analysis/PaperArgumentMapper.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperArgumentMapper::PaperArgumentMapper(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ArgumentMapper")
{
    setupUI();
    loadSettings();
}

void PaperArgumentMapper::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    mapBtn_ = new QPushButton("Map");
    mapBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(mapBtn_, &QPushButton::clicked, this, &PaperArgumentMapper::onMap);
    toolbar->addWidget(mapBtn_);

    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Valid", "Weak", "Invalid"});
    toolbar->addWidget(filterCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperArgumentMapper::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter argument text to map...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Map paper arguments");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperArgumentMapper::addEntry(const ArgumentEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit argumentMapped(entry.id, entry.strength);
    update();
}

QList<ArgumentEntry> PaperArgumentMapper::entries() const { return entries_; }

qreal PaperArgumentMapper::avgStrength() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.strength;
    return sum / entries_.size();
}

int PaperArgumentMapper::validCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.valid) c++;
    return c;
}

QMap<QString, int> PaperArgumentMapper::argTypeCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.argType]++;
    return counts;
}

void PaperArgumentMapper::onMap() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList argTypes = {"deductive", "inductive", "analogical", "abductive"};
    QStringList premises = {"empirical data", "literature review", "theoretical framework", "expert opinion"};
    QStringList conclusions = {"supports claim", "rejects hypothesis", "inconclusive", "requires validation"};

    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        ArgumentEntry e;
        e.id = entries_.size() + 1;
        e.argument = text.left(12) + " arg" + QString::number(i);
        e.argType = argTypes[QRandomGenerator::global()->bounded(argTypes.size())];
        e.strength = 0.15 + QRandomGenerator::global()->bounded(85) / 100.0;
        e.premise = premises[QRandomGenerator::global()->bounded(premises.size())];
        e.supportCount = QRandomGenerator::global()->bounded(15);
        e.coherence = 0.2 + QRandomGenerator::global()->bounded(80) / 100.0;
        e.conclusion = conclusions[QRandomGenerator::global()->bounded(conclusions.size())];
        e.valid = e.strength >= 0.6 && e.coherence >= 0.5;
        e.color = e.valid ? QColor(16,185,129) : (e.strength >= 0.4 ? QColor(245,158,11) : QColor(239,68,68));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperArgumentMapper::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Map paper arguments");
    update();
}

void PaperArgumentMapper::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Map paper arguments");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Argument Mapper");

    int w = width(), h = height();
    drawArgumentList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawTypeChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperArgumentMapper::drawArgumentList(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(34, (rect.height() - 10) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.argument.left(14) + (e.valid ? " [OK]" : ""));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.argType + " | " + QString::number(e.supportCount) + " supports");
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.strength * 100, 'f', 0) + "% str");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "coh:" + QString::number(e.coherence * 100, 'f', 0) + "% | " + e.conclusion.left(14));
    }
}

void PaperArgumentMapper::drawTypeChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Arg Types");

    auto counts = argTypeCounts();
    QStringList types = {"deductive", "inductive", "analogical", "abductive"};
    QString labels[] = {"Deductive", "Inductive", "Analogical", "Abductive"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(types[i]) ? counts[types[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 120));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 75, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 80, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 83 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperArgumentMapper::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Arguments", QString::number(entries_.size()), QColor(59,130,246)},
        {"Valid", QString::number(validCount()), QColor(16,185,129)},
        {"Avg Strength", QString::number(avgStrength() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Types", QString::number(argTypeCounts().size()), QColor(139,92,246)}
    };

    int boxH = qMin(42, (rect.height() - 10) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 22, Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 26, rect.width() - 20, 14, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperArgumentMapper::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Map paper arguments"); return; }
    infoLabel_->setText(QString("%1 args | %2 valid | %3% strength")
        .arg(entries_.size()).arg(validCount()).arg(avgStrength() * 100, 0, 'f', 0));
}

void PaperArgumentMapper::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ArgumentEntry e;
        e.id = settings_.value("id").toInt();
        e.argument = settings_.value("argument").toString();
        e.argType = settings_.value("argType").toString();
        e.strength = settings_.value("strength").toDouble();
        e.premise = settings_.value("premise").toString();
        e.supportCount = settings_.value("supportCount").toInt();
        e.coherence = settings_.value("coherence").toDouble();
        e.conclusion = settings_.value("conclusion").toString();
        e.valid = settings_.value("valid").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperArgumentMapper::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("argument", entries_[i].argument);
        settings_.setValue("argType", entries_[i].argType);
        settings_.setValue("strength", entries_[i].strength);
        settings_.setValue("premise", entries_[i].premise);
        settings_.setValue("supportCount", entries_[i].supportCount);
        settings_.setValue("coherence", entries_[i].coherence);
        settings_.setValue("conclusion", entries_[i].conclusion);
        settings_.setValue("valid", entries_[i].valid);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
