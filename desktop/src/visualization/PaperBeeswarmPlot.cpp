#include "visualization/PaperBeeswarmPlot.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperBeeswarmPlot::PaperBeeswarmPlot(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "BeeswarmPlot")
{
    setupUI();
    loadSettings();
}

void PaperBeeswarmPlot::setupUI() {
    auto* layout = new QHBoxLayout(this);

    auto* leftPanel = new QVBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Category-A", "Category-B", "Category-C", "Category-D", "Category-E"});
    leftPanel->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter label...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    leftPanel->addWidget(inputField_);

    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperBeeswarmPlot::onRender);
    leftPanel->addWidget(renderBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperBeeswarmPlot::onClear);
    leftPanel->addWidget(clearBtn_);

    infoLabel_ = new QLabel("Points: 0 | Outliers: 0 | Max: 0.0");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    leftPanel->addWidget(infoLabel_);

    leftPanel->addStretch();
    layout->addLayout(leftPanel);

    layout->addStretch(1);

    setMinimumSize(700, 500);
}

void PaperBeeswarmPlot::addEntry(const BeeswarmEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit dotSelected(entry.id, entry.value);
    update();
}

QList<BeeswarmEntry> PaperBeeswarmPlot::entries() const { return entries_; }

int PaperBeeswarmPlot::outlierCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.outlier) ++c;
    return c;
}

qreal PaperBeeswarmPlot::maxValue() const {
    qreal mx = 0;
    for (const auto& e : entries_)
        if (e.value > mx) mx = e.value;
    return mx;
}

QMap<QString, int> PaperBeeswarmPlot::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperBeeswarmPlot::onRender() {
    static const QStringList categories = {"Category-A", "Category-B", "Category-C", "Category-D", "Category-E"};
    static const QVector<QColor> palette = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"), QColor("#dc2626"), QColor("#7c3aed")
    };

    int count = 5 + QRandomGenerator::global()->bounded(8);
    for (int i = 0; i < count; ++i) {
        int catIndex = QRandomGenerator::global()->bounded(categories.size());
        BeeswarmEntry e;
        e.id = entries_.size() + 1;
        e.label = QString("Entry-%1").arg(e.id);
        e.category = categories[catIndex];
        e.group = categories[catIndex];
        e.value = QRandomGenerator::global()->bounded(10001) / 100.0;
        e.jitter = (QRandomGenerator::global()->bounded(20001) - 10000) / 10000.0;
        e.outlier = e.value > 90;
        e.color = palette[catIndex];
        entries_.append(e);
        emit dotSelected(e.id, e.value);
    }

    saveSettings();
    updateInfo();
    update();
}

void PaperBeeswarmPlot::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperBeeswarmPlot::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Add points to see beeswarm plot");
        return;
    }

    int w = width();
    int h = height();

    drawBeeswarm(p, QRect(20, 10, w * 3 / 5 - 30, h - 20));
    drawCategoryLegend(p, QRect(w * 3 / 5, 10, w / 5 - 10, h - 20));
    drawStats(p, QRect(w * 4 / 5, 10, w / 5 - 20, h - 20));
}

void PaperBeeswarmPlot::drawBeeswarm(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 20, "Beeswarm Plot");

    int margin = 30;
    int topMargin = 40;
    int plotX = rect.x() + margin;
    int plotY = rect.y() + topMargin;
    int plotW = rect.width() - 2 * margin;
    int plotH = rect.height() - topMargin - margin;

    if (plotW <= 0 || plotH <= 0) return;

    p.setPen(QPen(QColor(203, 213, 225), 1));
    p.drawLine(plotX, plotY, plotX, plotY + plotH);
    p.drawLine(plotX, plotY + plotH, plotX + plotW, plotY + plotH);

    p.setPen(QColor(148, 163, 184));
    p.setFont(QFont("Arial", 8));
    for (int tick = 0; tick <= 100; tick += 20) {
        int ty = plotY + plotH - static_cast<int>((tick / 100.0) * plotH);
        p.drawText(plotX - 28, ty - 6, 24, 14, Qt::AlignRight | Qt::AlignVCenter, QString::number(tick));
        p.setPen(QPen(QColor(241, 245, 249), 1, Qt::DotLine));
        p.drawLine(plotX + 1, ty, plotX + plotW, ty);
        p.setPen(QColor(148, 163, 184));
    }

    static const QStringList categories = {"Category-A", "Category-B", "Category-C", "Category-D", "Category-E"};
    qreal bandWidth = static_cast<qreal>(plotW) / categories.size();

    QMap<QString, QList<int>> catIndices;
    for (int i = 0; i < entries_.size(); ++i)
        catIndices[entries_[i].category].append(i);

    int dotSize = 8;
    for (const auto& cat : categories) {
        if (!catIndices.contains(cat)) continue;
        int catIdx = categories.indexOf(cat);
        qreal bandCenter = plotX + catIdx * bandWidth + bandWidth / 2.0;

        const auto& indices = catIndices[cat];
        int rowCount = 0;
        for (int idx : indices) {
            const auto& e = entries_[idx];
            int cy = plotY + plotH - static_cast<int>((e.value / 100.0) * plotH);
            cy = qBound(plotY, cy, plotY + plotH);
            qreal offset = e.jitter * bandWidth * 0.35;
            int cx = static_cast<int>(bandCenter + offset);
            cx = qBound(plotX, cx, plotX + plotW);

            if (e.outlier) {
                p.setPen(QPen(QColor("#dc2626"), 2));
                p.setBrush(e.color);
            } else {
                p.setPen(Qt::NoPen);
                p.setBrush(e.color);
            }
            p.drawEllipse(cx - dotSize / 2, cy - dotSize / 2, dotSize, dotSize);
            rowCount++;
        }
    }

    p.setPen(QColor(148, 163, 184));
    p.setFont(QFont("Arial", 8));
    for (int i = 0; i < categories.size(); ++i) {
        qreal cx = plotX + i * bandWidth + bandWidth / 2.0;
        p.drawText(static_cast<int>(cx - 30), plotY + plotH + 14, 60, 14, Qt::AlignCenter, categories[i]);
    }
}

void PaperBeeswarmPlot::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 20, "Legend");

    static const QStringList categories = {"Category-A", "Category-B", "Category-C", "Category-D", "Category-E"};
    static const QVector<QColor> palette = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"), QColor("#dc2626"), QColor("#7c3aed")
    };

    auto counts = categoryCounts();
    int itemH = qMin(28, (rect.height() - 80) / static_cast<int>(categories.size() + 1));

    for (int i = 0; i < categories.size(); ++i) {
        int y = rect.y() + 40 + i * (itemH + 6);

        p.setPen(Qt::NoPen);
        p.setBrush(palette[i]);
        p.drawRoundedRect(rect.x() + 5, y + 2, 14, 14, 2, 2);

        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y, rect.width() / 2 - 10, 18, Qt::AlignVCenter, categories[i]);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y, rect.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight, QString::number(count) + " pts");
    }

    int y = rect.y() + 40 + categories.size() * (itemH + 6);
    p.setPen(QPen(QColor("#dc2626"), 2));
    p.setBrush(QColor(239, 239, 239));
    p.drawEllipse(rect.x() + 5, y + 2, 14, 14);
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 9, QFont::Bold));
    p.drawText(rect.x() + 24, y, rect.width() / 2 - 10, 18, Qt::AlignVCenter, "Outliers");
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 8));
    p.drawText(rect.x() + rect.width() / 2, y, rect.width() / 2 - 5, 18,
               Qt::AlignVCenter | Qt::AlignRight, QString::number(outlierCount()));
}

void PaperBeeswarmPlot::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Points", QString::number(entries_.size()), QColor("#3b82f6")},
        {"Outliers",     QString::number(outlierCount()),  QColor("#dc2626")},
        {"Max Value",    QString::number(maxValue(), 'f', 1), QColor("#d97706")}
    };

    int boxH = qMin(56, (rect.height() - 40) / stats.size());
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + 10 + i * (boxH + 8);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 16, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() - 20, boxH / 2, Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + boxH / 2, rect.width() - 20, boxH / 2 - 4, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperBeeswarmPlot::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Points: 0 | Outliers: 0 | Max: 0.0");
        return;
    }
    infoLabel_->setText(QString("Points: %1 | Outliers: %2 | Max: %3")
        .arg(entries_.size())
        .arg(outlierCount())
        .arg(maxValue(), 0, 'f', 1));
}

void PaperBeeswarmPlot::loadSettings() {
    settings_.beginGroup("BeeswarmPlot");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        BeeswarmEntry e;
        e.id       = settings_.value("id").toInt();
        e.label    = settings_.value("label").toString();
        e.category = settings_.value("category").toString();
        e.group    = settings_.value("group").toString();
        e.value    = settings_.value("value").toDouble();
        e.jitter   = settings_.value("jitter").toDouble();
        e.outlier  = settings_.value("outlier").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperBeeswarmPlot::saveSettings() {
    settings_.beginGroup("BeeswarmPlot");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",       entries_[i].id);
        settings_.setValue("label",    entries_[i].label);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("group",    entries_[i].group);
        settings_.setValue("value",    entries_[i].value);
        settings_.setValue("jitter",   entries_[i].jitter);
        settings_.setValue("outlier",  entries_[i].outlier);
        settings_.setValue("color",    entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
