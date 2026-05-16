#include "analysis/PaperArgumentMapper2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperArgumentMapper2::PaperArgumentMapper2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ArgumentMapper2")
{
    setupUI();
    loadSettings();
}

void PaperArgumentMapper2::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    mapBtn_ = new QPushButton("Map");
    mapBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(mapBtn_, &QPushButton::clicked, this, &PaperArgumentMapper2::onMap);
    toolbar->addWidget(mapBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Claim", "Counter", "Support", "Rebuttal", "Premise"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperArgumentMapper2::onClear);
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

void PaperArgumentMapper2::addEntry(const ArgumentMapper2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit argumentMapped(entry.id, entry.strength);
    update();
}

QList<ArgumentMapper2Entry> PaperArgumentMapper2::entries() const { return entries_; }

int PaperArgumentMapper2::validCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.valid) ++c;
    return c;
}

qreal PaperArgumentMapper2::avgStrength() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.strength;
    return sum / entries_.size();
}

QMap<QString, int> PaperArgumentMapper2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperArgumentMapper2::onMap() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"Claim", "Counter", "Support", "Rebuttal", "Premise"};
    QStringList stances = {"for", "against", "neutral", "qualified"};
    QColor palette[] = {
        QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06), QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };

    int count = 3 + QRandomGenerator::global()->bounded(6);
    for (int i = 0; i < count; ++i) {
        int ci = QRandomGenerator::global()->bounded(categories.size());
        ArgumentMapper2Entry e;
        e.id = entries_.size() + 1;
        e.argument = text.left(10) + " arg" + QString::number(i);
        e.category = categories[ci];
        e.stance = stances[QRandomGenerator::global()->bounded(stances.size())];
        e.strength = 0.15 + QRandomGenerator::global()->bounded(85) / 100.0;
        e.evidence = QRandomGenerator::global()->bounded(20);
        e.valid = e.strength >= 0.6 && e.evidence >= 3;
        e.color = palette[ci];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperArgumentMapper2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Map paper arguments");
    update();
}

void PaperArgumentMapper2::paintEvent(QPaintEvent*) {
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
    p.drawText(20, 30, "Argument Mapper 2");

    int w = width(), h = height();
    drawMapView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperArgumentMapper2::drawMapView(QPainter& p, const QRect& rect) {
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
                   e.category + " | " + e.stance + " | ev:" + QString::number(e.evidence));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.strength * 100, 'f', 0) + "% str");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "id:" + QString::number(e.id));
    }
}

void PaperArgumentMapper2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList categories = {"Claim", "Counter", "Support", "Rebuttal", "Premise"};
    QColor palette[] = {
        QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06), QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 120));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 75, barH, Qt::AlignRight | Qt::AlignVCenter, categories[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(palette[i]);
        p.drawRoundedRect(rect.x() + 80, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 83 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperArgumentMapper2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Arguments", QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Valid", QString::number(validCount()), QColor(16, 163, 74)},
        {"Avg Strength", QString::number(avgStrength() * 100, 'f', 0) + "%", QColor(217, 119, 6)},
        {"Categories", QString::number(categoryCounts().size()), QColor(124, 58, 237)}
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

void PaperArgumentMapper2::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Map paper arguments"); return; }
    infoLabel_->setText(QString("%1 args | %2 valid | %3% strength | %4 categories")
        .arg(entries_.size()).arg(validCount())
        .arg(avgStrength() * 100, 0, 'f', 0)
        .arg(categoryCounts().size()));
}

void PaperArgumentMapper2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ArgumentMapper2Entry e;
        e.id = settings_.value("id").toInt();
        e.argument = settings_.value("argument").toString();
        e.category = settings_.value("category").toString();
        e.stance = settings_.value("stance").toString();
        e.strength = settings_.value("strength").toDouble();
        e.evidence = settings_.value("evidence").toInt();
        e.valid = settings_.value("valid").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperArgumentMapper2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("argument", entries_[i].argument);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("stance", entries_[i].stance);
        settings_.setValue("strength", entries_[i].strength);
        settings_.setValue("evidence", entries_[i].evidence);
        settings_.setValue("valid", entries_[i].valid);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
