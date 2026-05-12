#include "visualization/PaperTornadoChart.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <cmath>

namespace {
const QColor kPalette[] = {
    QColor(0x3b, 0x82, 0xf6),  // #3b82f6
    QColor(0x16, 0xa3, 0x4a),  // #16a34a
    QColor(0xd9, 0x77, 0x06),  // #d97706
    QColor(0xdc, 0x26, 0x26),  // #dc2626
    QColor(0x7c, 0x3a, 0xed),  // #7c3aed
};
constexpr int kPaletteSize = 5;
}

PaperTornadoChart::PaperTornadoChart(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "TornadoChart")
    , renderBtn_(nullptr)
    , clearBtn_(nullptr)
    , categoryCombo_(nullptr)
    , inputField_(nullptr)
    , infoLabel_(nullptr)
{
    setupUI();
    loadSettings();
}

void PaperTornadoChart::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperTornadoChart::onRender);
    toolbar->addWidget(renderBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Impact", "Methodology", "Demographics", "Outcomes", "Risk"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperTornadoChart::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText(
        "Enter factors as: factor1:positive,negative; factor2:positive,negative ...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Render tornado chart");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(640, 520);
}

void PaperTornadoChart::addEntry(const TornadoEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit tornadoRendered(entry.id, entry.net);
    update();
}

QList<TornadoEntry> PaperTornadoChart::entries() const {
    return entries_;
}

int PaperTornadoChart::significantCount() const {
    int count = 0;
    for (const auto& e : entries_) {
        if (e.significant) ++count;
    }
    return count;
}

qreal PaperTornadoChart::maxNet() const {
    qreal maxVal = 0;
    for (const auto& e : entries_) {
        maxVal = qMax(maxVal, std::abs(e.net));
    }
    return maxVal;
}

QMap<QString, int> PaperTornadoChart::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) {
        counts[e.category]++;
    }
    return counts;
}

void PaperTornadoChart::onRender() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList tokens = text.split(';', Qt::SkipEmptyParts);
    if (tokens.isEmpty()) return;

    QString category = categoryCombo_->currentText().toLower();
    if (category == "all") {
        QStringList cats = {"impact", "methodology", "demographics", "outcomes", "risk"};
        category = cats[QRandomGenerator::global()->bounded(cats.size())];
    }

    for (const auto& token : tokens) {
        QString t = token.trimmed();
        int colonPos = t.indexOf(':');
        if (colonPos < 0) continue;

        QString factor = t.left(colonPos).trimmed();
        QString valuesStr = t.mid(colonPos + 1).trimmed();

        QStringList values = valuesStr.split(',', Qt::SkipEmptyParts);
        if (values.size() < 2) continue;

        bool okPos = false, okNeg = false;
        qreal positive = values[0].trimmed().toDouble(&okPos);
        qreal negative = values[1].trimmed().toDouble(&okNeg);

        if (!okPos || !okNeg) continue;

        qreal net = positive - negative;
        bool significant = std::abs(net) > 30.0;

        // Determine side based on net direction
        QString side = (net >= 0) ? "right" : "left";

        // Assign color from palette cycling
        QColor color = kPalette[entries_.size() % kPaletteSize];

        TornadoEntry entry;
        entry.id = entries_.size() + 1;
        entry.factor = factor;
        entry.category = category;
        entry.side = side;
        entry.positive = positive;
        entry.negative = negative;
        entry.net = net;
        entry.significant = significant;
        entry.color = color;

        addEntry(entry);
    }

    inputField_->clear();
}

void PaperTornadoChart::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperTornadoChart::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Render tornado chart");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Tornado Chart");

    int w = width(), h = height();
    drawTornadoView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperTornadoChart::drawTornadoView(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int barH = qMin(24, (rect.height() - 10) / qMax(show, 1));
    int labelW = 80;

    // Center line x position
    int centerX = rect.x() + labelW + (rect.width() - labelW) / 2;
    int halfBarAreaW = (rect.width() - labelW) / 2 - 10;

    // Find max absolute value for scaling
    qreal maxVal = 1.0;
    for (int i = 0; i < show; ++i) {
        maxVal = qMax(maxVal, entries_[i].positive);
        maxVal = qMax(maxVal, entries_[i].negative);
    }

    // Draw center axis line
    p.setPen(QPen(QColor(203, 213, 225), 1, Qt::DashLine));
    p.drawLine(centerX, rect.y(), centerX, rect.y() + show * (barH + 6));

    // Draw axis labels
    p.setPen(QColor(148, 163, 184));
    p.setFont(QFont("Arial", 7));
    p.drawText(centerX - halfBarAreaW, rect.y() - 4, "Negative");
    p.drawText(centerX + halfBarAreaW - 30, rect.y() - 4, "Positive");

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (barH + 6);

        // Factor label on left side
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        QString label = e.factor.length() > 10 ? e.factor.left(9) + ".." : e.factor;
        p.drawText(rect.x(), y, labelW, barH, Qt::AlignRight | Qt::AlignVCenter, label);

        // Positive bar extending right from center
        int posW = static_cast<int>((e.positive / maxVal) * halfBarAreaW);
        p.setPen(Qt::NoPen);
        QColor posColor = e.color;
        posColor.setAlpha(200);
        p.setBrush(posColor);
        p.drawRoundedRect(centerX, y + 2, posW, barH - 4, 3, 3);

        // Negative bar extending left from center
        int negW = static_cast<int>((e.negative / maxVal) * halfBarAreaW);
        QColor negColor = e.color.darker(140);
        negColor.setAlpha(200);
        p.setBrush(negColor);
        p.drawRoundedRect(centerX - negW, y + 2, negW, barH - 4, 3, 3);

        // Value labels at bar ends
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(centerX + posW + 3, y + barH - 5, QString::number(e.positive, 'f', 0));
        if (negW > 20) {
            p.drawText(centerX - negW + 3, y + barH - 5, QString::number(e.negative, 'f', 0));
        }

        // Significance marker
        if (e.significant) {
            p.setPen(QPen(QColor(220, 38, 38), 2));
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(QPoint(centerX - negW - 10, y + barH / 2), 4, 4);
        }
    }
}

void PaperTornadoChart::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"impact", "methodology", "demographics", "outcomes", "risk"};
    QString labels[] = {"Impact", "Methodology", "Demographics", "Outcomes", "Risk"};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(24, (rect.height() - 30) / cats.size());
    for (int i = 0; i < cats.size(); ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 120));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 85, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(kPalette[i]);
        p.drawRoundedRect(rect.x() + 90, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 93 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperTornadoChart::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries",      QString::number(entries_.size()),   QColor(0x3b, 0x82, 0xf6)},
        {"Significant",  QString::number(significantCount()), QColor(0x16, 0xa3, 0x4a)},
        {"Max Net",      QString::number(maxNet(), 'f', 0),  QColor(0xd9, 0x77, 0x06)},
        {"Categories",   QString::number(categoryCounts().size()), QColor(0x7c, 0x3a, 0xed)},
    };

    int boxH = qMin(42, (rect.height() - 10) / qMax(stats.size(), 1));
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 22,
                   Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 26, rect.width() - 20, 14,
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperTornadoChart::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Render tornado chart");
        return;
    }
    infoLabel_->setText(QString("%1 factors | %2 significant | max net: %3")
        .arg(entries_.size())
        .arg(significantCount())
        .arg(maxNet(), 0, 'f', 0));
}

void PaperTornadoChart::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        TornadoEntry e;
        e.id          = settings_.value("id").toInt();
        e.factor      = settings_.value("factor").toString();
        e.category    = settings_.value("category").toString();
        e.side        = settings_.value("side").toString();
        e.positive    = settings_.value("positive").toDouble();
        e.negative    = settings_.value("negative").toDouble();
        e.net         = settings_.value("net").toDouble();
        e.significant = settings_.value("significant").toBool();
        e.color       = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperTornadoChart::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& e = entries_[i];
        settings_.setValue("id",          e.id);
        settings_.setValue("factor",      e.factor);
        settings_.setValue("category",    e.category);
        settings_.setValue("side",        e.side);
        settings_.setValue("positive",    e.positive);
        settings_.setValue("negative",    e.negative);
        settings_.setValue("net",         e.net);
        settings_.setValue("significant", e.significant);
        settings_.setValue("color",       e.color.name());
    }
    settings_.endArray();
}
