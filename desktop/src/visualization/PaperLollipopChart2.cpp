#include "visualization/PaperLollipopChart2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <algorithm>
#include <cmath>

namespace {
static const QVector<QColor> kPalette = {
    QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
    QColor("#dc2626"), QColor("#7c3aed")
};

static const QStringList kCategories = {
    "Impact", "Quality", "Novelty", "Relevance", "Timeliness"
};
}

PaperLollipopChart2::PaperLollipopChart2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "LollipopChart2")
{
    setupUI();
    loadSettings();
}

void PaperLollipopChart2::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperLollipopChart2::onRender);
    toolbar->addWidget(renderBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Impact", "Quality", "Novelty", "Relevance", "Timeliness"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperLollipopChart2::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter entries (format: label:value) ...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Render lollipop chart");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(640, 540);
}

void PaperLollipopChart2::addEntry(const LollipopChart2Entry& entry) {
    entries_.append(entry);
    std::stable_sort(entries_.begin(), entries_.end(),
        [](const LollipopChart2Entry& a, const LollipopChart2Entry& b) {
            return a.rank < b.rank;
        });
    saveSettings();
    updateInfo();
    emit lollipopSelected(entry.id, entry.value);
    update();
}

QList<LollipopChart2Entry> PaperLollipopChart2::entries() const {
    return entries_;
}

int PaperLollipopChart2::highlightedCount() const {
    int count = 0;
    for (const auto& e : entries_)
        if (e.highlighted) ++count;
    return count;
}

qreal PaperLollipopChart2::avgValue() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_)
        sum += e.value;
    return sum / entries_.size();
}

QMap<QString, int> PaperLollipopChart2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperLollipopChart2::onRender() {
    QString text = inputField_->text().trimmed();

    if (text.isEmpty()) {
        // Seed 8 default entries when input is empty
        entries_.clear();
        QStringList defaultLabels = {
            "Paper Alpha", "Paper Beta", "Paper Gamma", "Paper Delta",
            "Paper Epsilon", "Paper Zeta", "Paper Eta", "Paper Theta"
        };
        for (int i = 0; i < 8; ++i) {
            LollipopChart2Entry entry;
            entry.id = i + 1;
            entry.label = defaultLabels[i];
            entry.category = kCategories[i % kCategories.size()];
            entry.metric = QStringLiteral("Score %1").arg(i + 1);
            entry.value = 15.0 + QRandomGenerator::global()->bounded(80);
            entry.rank = i + 1;
            entry.highlighted = (entry.value >= 60.0);
            entry.color = kPalette[i % kPalette.size()];
            entries_.append(entry);
            emit lollipopSelected(entry.id, entry.value);
        }
    } else {
        entries_.clear();
        QStringList parts = text.split(',', Qt::SkipEmptyParts);
        for (int i = 0; i < parts.size(); ++i) {
            QString token = parts[i].trimmed();
            QString label = token;
            qreal baseValue = 0.0;

            int colonIdx = token.indexOf(':');
            if (colonIdx > 0) {
                label = token.left(colonIdx).trimmed();
                baseValue = token.mid(colonIdx + 1).trimmed().toDouble();
            }

            if (label.isEmpty()) continue;

            LollipopChart2Entry entry;
            entry.id = i + 1;
            entry.label = label;
            entry.category = kCategories[QRandomGenerator::global()->bounded(kCategories.size())];
            entry.metric = QStringLiteral("Score %1").arg(i + 1);

            if (baseValue > 0.0) {
                entry.value = baseValue + QRandomGenerator::global()->bounded(20);
            } else {
                entry.value = 10.0 + QRandomGenerator::global()->bounded(90);
            }
            entry.rank = i + 1;
            entry.highlighted = (entry.value >= 60.0);
            entry.color = kPalette[i % kPalette.size()];

            entries_.append(entry);
            emit lollipopSelected(entry.id, entry.value);
        }
    }

    std::stable_sort(entries_.begin(), entries_.end(),
        [](const LollipopChart2Entry& a, const LollipopChart2Entry& b) {
            return a.rank < b.rank;
        });

    saveSettings();
    updateInfo();
    inputField_->clear();
    update();
}

void PaperLollipopChart2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperLollipopChart2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Render lollipop chart");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Lollipop Chart 2");

    int w = width(), h = height();
    drawLollipopChart(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperLollipopChart2::drawLollipopChart(QPainter& p, const QRect& area) {
    int show = qMin(static_cast<int>(entries_.size()), 12);
    if (show == 0) return;

    // Find max value for scaling
    qreal mv = 0.0;
    for (int i = 0; i < show; ++i)
        mv = qMax(mv, entries_[i].value);
    if (mv <= 0.0) mv = 1.0;

    int labelW = 80;
    int chartTop = area.y();
    int chartBottom = area.y() + area.height() - 6;
    int chartH = chartBottom - chartTop;
    int rowH = qMax(16, chartH / qMax(show, 1));

    // Draw axis line at left
    int axisX = area.x() + labelW;
    p.setPen(QPen(QColor(203, 213, 225), 1));
    p.drawLine(axisX, chartTop, axisX, chartBottom);
    p.drawLine(axisX, chartBottom, area.x() + area.width(), chartBottom);

    // Draw grid lines
    int gridSteps = 5;
    p.setPen(QPen(QColor(241, 245, 249), 1, Qt::DotLine));
    for (int g = 1; g <= gridSteps; ++g) {
        int gx = axisX + static_cast<int>((static_cast<qreal>(g) / gridSteps) * (area.width() - labelW - 10));
        p.drawLine(gx, chartTop, gx, chartBottom);

        p.setPen(QColor(148, 163, 184));
        p.setFont(QFont("Arial", 7));
        qreal gridVal = (static_cast<qreal>(g) / gridSteps) * mv;
        p.drawText(gx - 15, chartBottom + 2, 30, 12,
                   Qt::AlignHCenter | Qt::AlignTop,
                   QString::number(gridVal, 'f', 0));
        p.setPen(QPen(QColor(241, 245, 249), 1, Qt::DotLine));
    }

    // Draw horizontal lollipop stems sorted by rank
    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = chartTop + static_cast<int>((static_cast<qreal>(i) + 0.5) * rowH);

        // Label on the left
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(area.x(), y - 7, labelW - 6, 14,
                   Qt::AlignRight | Qt::AlignVCenter, e.label);

        // Horizontal stem from axis to value position
        int stemEndX = axisX + static_cast<int>((e.value / mv) * (area.width() - labelW - 10));

        QPen stemPen(e.color, 2);
        p.setPen(stemPen);
        p.drawLine(axisX, y, stemEndX, y);

        // Circle head sized by value
        int headR = static_cast<int>(4.0 + (e.value / mv) * 10.0);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawEllipse(stemEndX - headR, y - headR, headR * 2, headR * 2);

        // Highlighted glow ring
        if (e.highlighted) {
            p.setPen(QPen(QColor(255, 255, 255), 2));
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(stemEndX - headR - 2, y - headR - 2,
                          (headR + 2) * 2, (headR + 2) * 2);
        }

        // Value text next to head
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 7));
        p.drawText(stemEndX + headR + 3, y - 5, 40, 12,
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QString::number(e.value, 'f', 0));
    }
}

void PaperLollipopChart2::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QColor catColors[] = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 30) / kCategories.size());
    for (int i = 0; i < kCategories.size(); ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(kCategories[i]) ? counts[kCategories[i]] : 0;
        int barW = static_cast<int>(
            (static_cast<qreal>(count) / maxVal) * (rect.width() - 130));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 85, barH,
                   Qt::AlignRight | Qt::AlignVCenter, kCategories[i]);

        // Color indicator dot
        p.setPen(Qt::NoPen);
        p.setBrush(catColors[i]);
        p.drawEllipse(rect.x() + 90, y + barH / 2 - 4, 8, 8);

        // Count bar
        if (barW > 0) {
            p.setBrush(catColors[i].lighter(140));
            p.drawRoundedRect(rect.x() + 104, y, barW, barH - 2, 3, 3);
        }

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 108 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperLollipopChart2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Entries",   QString::number(entries_.size()),       QColor("#3b82f6")},
        {"Highlighted",     QString::number(highlightedCount()),    QColor("#16a34a")},
        {"Avg Value",       QString::number(avgValue(), 'f', 1),    QColor("#d97706")},
        {"Categories",      QString::number(categoryCounts().size()), QColor("#7c3aed")}
    };

    int boxH = qMin(42, (rect.height() - 10) / stats.size());
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        // Left accent bar
        p.setBrush(stats[i].color);
        p.drawRoundedRect(rect.x(), y, 4, boxH, 2, 2);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 12, y + 5, rect.width() - 22, 22,
                   Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 12, y + 26, rect.width() - 22, 14,
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperLollipopChart2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Render lollipop chart");
        return;
    }
    infoLabel_->setText(
        QString("%1 entries | %2 highlighted | avg %3")
            .arg(entries_.size())
            .arg(highlightedCount())
            .arg(avgValue(), 0, 'f', 1));
}

void PaperLollipopChart2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        LollipopChart2Entry e;
        e.id          = settings_.value("id").toInt();
        e.label       = settings_.value("label").toString();
        e.category    = settings_.value("category").toString();
        e.metric      = settings_.value("metric").toString();
        e.value       = settings_.value("value").toDouble();
        e.rank        = settings_.value("rank").toInt();
        e.highlighted = settings_.value("highlighted").toBool();
        e.color       = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperLollipopChart2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",          entries_[i].id);
        settings_.setValue("label",       entries_[i].label);
        settings_.setValue("category",    entries_[i].category);
        settings_.setValue("metric",      entries_[i].metric);
        settings_.setValue("value",       entries_[i].value);
        settings_.setValue("rank",        entries_[i].rank);
        settings_.setValue("highlighted", entries_[i].highlighted);
        settings_.setValue("color",       entries_[i].color.name());
    }
    settings_.endArray();
}
