#include "visualization/PaperStepPlot2.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>
#include <algorithm>

namespace {
const QList<QColor> kPalette = {
    QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706),
    QColor(0xdc2626), QColor(0x7c3aed)
};
const QStringList kSeries   = {"Revenue", "Users", "Latency", "Errors"};
const QStringList kPhases    = {"Growth", "Plateau", "Decline", "Recovery"};
const QStringList kCategories = {"Performance", "Business", "System", "Network", "Application"};
}

PaperStepPlot2::PaperStepPlot2(QWidget* parent)
    : QWidget(parent), settings_("PaperCrawler", "StepPlot2") {
    setupUI();
    loadSettings();
    if (entries_.isEmpty()) {
        for (int i = 0; i < 8; ++i) {
            StepPlot2Entry e;
            e.id = i + 1;
            e.series    = kSeries[i % kSeries.size()];
            e.category  = kCategories[i % kCategories.size()];
            e.phase     = kPhases[i % kPhases.size()];
            e.value     = QRandomGenerator::global()->bounded(10.0, 100.0);
            e.step      = i + 1;
            e.ascending = e.value > 50.0;
            e.color     = kPalette[i % kPalette.size()];
            entries_.append(e);
        }
        saveSettings();
        updateInfo();
        update();
    }
}

void PaperStepPlot2::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems(QStringList{"All"} + kCategories);
    categoryCombo_->setStyleSheet(
        "QComboBox{padding:5px 10px;border:1px solid #cbd5e1;border-radius:4px;"
        "background:white;min-width:120px;}"
        "QComboBox::drop-down{border:none;}");

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Search series...");
    inputField_->setStyleSheet(
        "QLineEdit{padding:5px 10px;border:1px solid #cbd5e1;border-radius:4px;"
        "background:white;}");

    plotBtn_ = new QPushButton("Plot", this);
    plotBtn_->setStyleSheet(
        "QPushButton{background:#3b82f6;color:white;padding:6px 14px;"
        "border-radius:4px;font-weight:bold;}"
        "QPushButton:hover{background:#2563eb;}");

    clearBtn_ = new QPushButton("Clear", this);
    clearBtn_->setStyleSheet(
        "QPushButton{background:#dc2626;color:white;padding:6px 14px;"
        "border-radius:4px;font-weight:bold;}"
        "QPushButton:hover{background:#b91c1c;}");

    infoLabel_ = new QLabel(this);
    infoLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    infoLabel_->setStyleSheet("color:#64748b;font-size:12px;");

    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(plotBtn_);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();
    toolbar->addWidget(infoLabel_);

    mainLayout->addLayout(toolbar);
    mainLayout->addStretch();

    setMinimumHeight(480);
    setMinimumWidth(720);

    connect(plotBtn_, &QPushButton::clicked, this, &PaperStepPlot2::onPlot);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperStepPlot2::onClear);
}

void PaperStepPlot2::addEntry(const StepPlot2Entry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<StepPlot2Entry> PaperStepPlot2::entries() const {
    return entries_;
}

int PaperStepPlot2::ascendingCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.ascending) ++c;
    return c;
}

qreal PaperStepPlot2::maxValue() const {
    qreal mx = 0;
    for (const auto& e : entries_)
        mx = qMax(mx, e.value);
    return mx;
}

QMap<QString, int> PaperStepPlot2::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_)
        m[e.category]++;
    return m;
}

// -- painting ----------------------------------------------------------------

void PaperStepPlot2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    int toolbarH = 40;

    p.fillRect(rect(), QColor(0xf8fafc));

    drawStepPlot(p, QRect(10, toolbarH, static_cast<int>(w * 0.6), h - toolbarH));
    drawCategoryLegend(p, QRect(static_cast<int>(w * 0.62), toolbarH, static_cast<int>(w * 0.37), static_cast<int>(h * 0.72)));
    drawStats(p, QRect(10, static_cast<int>(h * 0.75), w - 20, static_cast<int>(h * 0.24) - toolbarH));
}

void PaperStepPlot2::drawStepPlot(QPainter& painter, const QRect& rect) {
    // title
    painter.setPen(QColor(0x1e293b));
    painter.setFont(QFont("Sans", 11, QFont::Bold));
    painter.drawText(rect.adjusted(0, 0, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Step Plot");

    // compute filtered entries
    QString filter = categoryCombo_->currentText();
    QString search = inputField_->text().trimmed().toLower();
    QList<StepPlot2Entry> visible;
    for (const auto& e : entries_) {
        if (filter != "All" && e.category != filter) continue;
        if (!search.isEmpty() && !e.series.toLower().contains(search)) continue;
        visible.append(e);
    }
    if (visible.isEmpty()) {
        painter.setPen(QColor(0x94a3b8));
        painter.setFont(QFont("Sans", 10));
        painter.drawText(rect, Qt::AlignCenter, "No data to display");
        return;
    }

    // layout constants
    int marginTop = 30;
    int marginBottom = 30;
    int marginLeft = 50;
    int marginRight = 10;
    int plotX = rect.left() + marginLeft;
    int plotY = rect.top() + marginTop;
    int plotW = rect.width() - marginLeft - marginRight;
    int plotH = rect.height() - marginTop - marginBottom;
    int baseY = plotY + plotH;

    // value range
    qreal maxVal = 0;
    int maxStep = 0;
    for (const auto& e : visible) {
        maxVal = qMax(maxVal, e.value);
        maxStep = qMax(maxStep, e.step);
    }
    if (maxVal <= 0) maxVal = 100.0;
    if (maxStep <= 0) maxStep = 1;

    // grid and Y axis
    painter.setPen(QPen(QColor(0xe2e8f0), 1));
    painter.setFont(QFont("Sans", 8));
    int yTicks = 5;
    for (int i = 0; i <= yTicks; ++i) {
        qreal val = maxVal * i / yTicks;
        int y = baseY - static_cast<int>(val / maxVal * plotH);
        painter.drawLine(plotX, y, plotX + plotW, y);
        painter.setPen(QColor(0x94a3b8));
        painter.drawText(plotX - 45, y - 6, 40, 14, Qt::AlignRight | Qt::AlignVCenter,
                         QString::number(val, 'f', 0));
        painter.setPen(QPen(QColor(0xe2e8f0), 1));
    }

    // X axis base
    painter.setPen(QPen(QColor(0x334155), 1));
    painter.drawLine(plotX, baseY, plotX + plotW, baseY);

    // X axis labels
    painter.setFont(QFont("Sans", 8));
    painter.setPen(QColor(0x64748b));
    for (const auto& e : visible) {
        int x = plotX + static_cast<int>(static_cast<qreal>(e.step) / maxStep * plotW);
        if (x >= plotX && x <= plotX + plotW) {
            painter.drawText(x - 12, baseY + 4, 24, 14, Qt::AlignCenter,
                             QString::number(e.step));
        }
    }

    // draw axis labels
    painter.save();
    painter.setPen(QColor(0x475569));
    painter.setFont(QFont("Sans", 9));
    painter.drawText(plotX - 45, plotY - 18, 45, 16, Qt::AlignRight, "Value");
    painter.drawText(plotX + plotW - 20, baseY + 16, 40, 14, Qt::AlignLeft, "Step");
    painter.restore();

    // group by series for step lines
    QMap<QString, QList<StepPlot2Entry>> seriesMap;
    for (const auto& e : visible)
        seriesMap[e.series].append(e);

    int seriesIdx = 0;
    for (auto it = seriesMap.begin(); it != seriesMap.end(); ++it, ++seriesIdx) {
        QColor color = kPalette[seriesIdx % kPalette.size()];
        auto& list = it.value();
        std::sort(list.begin(), list.end(),
                  [](const StepPlot2Entry& a, const StepPlot2Entry& b) {
                      return a.step < b.step;
                  });

        QPainterPath path;
        bool first = true;
        qreal prevVal = 0;
        int prevX = plotX;
        for (const auto& e : list) {
            int x = plotX + static_cast<int>(static_cast<qreal>(e.step) / maxStep * plotW);
            int y = baseY - static_cast<int>(e.value / maxVal * plotH);
            x = qBound(plotX, x, plotX + plotW);
            y = qBound(plotY, y, baseY);

            if (first) {
                path.moveTo(x, y);
                first = false;
            } else {
                // horizontal then vertical (step function)
                int prevY = baseY - static_cast<int>(prevVal / maxVal * plotH);
                prevY = qBound(plotY, prevY, baseY);
                path.lineTo(x, prevY); // horizontal
                path.lineTo(x, y);     // vertical
            }

            // ascending marker (triangle up)
            if (e.ascending) {
                painter.setPen(Qt::NoPen);
                painter.setBrush(color.lighter(130));
                QPolygonF tri;
                tri << QPointF(x, y - 8) << QPointF(x - 5, y) << QPointF(x + 5, y);
                painter.drawPolygon(tri);
            }

            // dot
            painter.setPen(Qt::NoPen);
            painter.setBrush(color);
            painter.drawEllipse(QPointF(x, y), 4, 4);

            // value label
            painter.setPen(color.darker(120));
            painter.setFont(QFont("Sans", 7));
            painter.drawText(x - 15, y - 12, 30, 12, Qt::AlignCenter,
                             QString::number(e.value, 'f', 1));

            prevVal = e.value;
            prevX = x;
        }

        // draw the step line
        painter.setPen(QPen(color, 2.5));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);
    }

    // series legend at bottom-left
    painter.setFont(QFont("Sans", 8));
    int lx = plotX;
    int ly = baseY + 22;
    seriesIdx = 0;
    for (auto it = seriesMap.begin(); it != seriesMap.end(); ++it, ++seriesIdx) {
        QColor c = kPalette[seriesIdx % kPalette.size()];
        painter.setPen(Qt::NoPen);
        painter.setBrush(c);
        painter.drawEllipse(lx, ly, 8, 8);
        painter.setPen(QColor(0x334155));
        painter.drawText(lx + 12, ly + 9, it.key());
        lx += 80;
    }
    painter.setBrush(Qt::NoBrush);
}

void PaperStepPlot2::drawCategoryLegend(QPainter& painter, const QRect& rect) {
    painter.setPen(QColor(0x1e293b));
    painter.setFont(QFont("Sans", 11, QFont::Bold));
    painter.drawText(rect.adjusted(6, 0, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Categories");

    auto counts = categoryCounts();
    int y = rect.top() + 26;
    int barMaxW = rect.width() - 80;

    for (int i = 0; i < kCategories.size(); ++i) {
        const QString& cat = kCategories[i];
        int count = counts.value(cat, 0);
        QColor color = kPalette[i % kPalette.size()];

        // background bar
        int barW = qMax(20, static_cast<int>(static_cast<qreal>(count) /
                      qMax(entries_.size(), 1) * barMaxW));
        painter.setPen(Qt::NoPen);
        painter.setBrush(color.lighter(160));
        painter.drawRoundedRect(rect.left() + 6, y, barW, 24, 4, 4);

        // colored indicator
        painter.setBrush(color);
        painter.drawRoundedRect(rect.left() + 6, y, 6, 24, 3, 3);

        // label
        painter.setPen(QColor(0x1e293b));
        painter.setFont(QFont("Sans", 9));
        painter.drawText(rect.left() + 16, y, barW - 10, 24,
                         Qt::AlignLeft | Qt::AlignVCenter, cat);

        // count badge
        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        int badgeX = rect.left() + barW + 14;
        painter.drawRoundedRect(badgeX, y + 4, 28, 16, 8, 8);
        painter.setPen(Qt::white);
        painter.setFont(QFont("Sans", 8, QFont::Bold));
        painter.drawText(badgeX, y + 4, 28, 16, Qt::AlignCenter, QString::number(count));

        y += 32;
    }
    painter.setBrush(Qt::NoBrush);
}

void PaperStepPlot2::drawStats(QPainter& painter, const QRect& rect) {
    painter.setPen(QColor(0x1e293b));
    painter.setFont(QFont("Sans", 11, QFont::Bold));
    painter.drawText(rect.adjusted(6, 0, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Statistics");

    struct Stat {
        QString label;
        QString value;
        QColor color;
    };

    qreal avgStep = 0;
    for (const auto& e : entries_) avgStep += e.step;
    if (!entries_.isEmpty()) avgStep /= entries_.size();

    QList<Stat> stats = {
        {"Total Points",   QString::number(entries_.size()),   kPalette[0]},
        {"Ascending Count", QString::number(ascendingCount()), kPalette[1]},
        {"Max Value",       QString::number(maxValue(), 'f', 2), kPalette[2]},
        {"Avg Step",        QString::number(avgStep, 'f', 1),   kPalette[3]}
    };

    int boxW = qMax(100, (rect.width() - 30) / 4);
    int boxH = rect.height() - 24;
    int x = rect.left() + 6;
    int y = rect.top() + 22;

    for (int i = 0; i < stats.size(); ++i) {
        const auto& s = stats[i];

        // box background
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0xffffff));
        painter.drawRoundedRect(x, y, boxW - 8, boxH, 6, 6);

        // top accent bar
        painter.setBrush(s.color);
        painter.drawRoundedRect(x, y, boxW - 8, 4, 2, 2);

        // value
        painter.setPen(s.color);
        painter.setFont(QFont("Sans", 16, QFont::Bold));
        painter.drawText(x + 8, y + 8, boxW - 24, boxH / 2,
                         Qt::AlignLeft | Qt::AlignVCenter, s.value);

        // label
        painter.setPen(QColor(0x64748b));
        painter.setFont(QFont("Sans", 9));
        painter.drawText(x + 8, y + boxH / 2, boxW - 24, boxH / 2 - 4,
                         Qt::AlignLeft | Qt::AlignVCenter, s.label);

        x += boxW;
    }
}

// -- slots -------------------------------------------------------------------

void PaperStepPlot2::onPlot() {
    static int nextId = entries_.isEmpty() ? 1 : entries_.last().id + 1;

    StepPlot2Entry e;
    e.id = nextId++;
    e.series    = kSeries[QRandomGenerator::global()->bounded(kSeries.size())];
    e.category  = kCategories[QRandomGenerator::global()->bounded(kCategories.size())];
    e.phase     = kPhases[QRandomGenerator::global()->bounded(kPhases.size())];
    e.value     = QRandomGenerator::global()->bounded(5.0, 100.0);
    e.step      = e.id;
    e.ascending = e.value > 50.0;
    e.color     = kPalette[QRandomGenerator::global()->bounded(kPalette.size())];
    entries_.append(e);
    emit stepReached(e.id, e.value);
    saveSettings();
    updateInfo();
    update();
}

void PaperStepPlot2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

// -- helpers -----------------------------------------------------------------

void PaperStepPlot2::updateInfo() {
    infoLabel_->setText(
        QString("Points: %1 | Ascending: %2 | Max: %3")
            .arg(entries_.size())
            .arg(ascendingCount())
            .arg(QString::number(maxValue(), 'f', 2)));
}

void PaperStepPlot2::loadSettings() {
    settings_.beginGroup("StepPlot2");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        StepPlot2Entry e;
        e.id       = settings_.value(QString("id_%1").arg(i)).toInt();
        e.series   = settings_.value(QString("series_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.phase    = settings_.value(QString("phase_%1").arg(i)).toString();
        e.value    = settings_.value(QString("value_%1").arg(i)).toDouble();
        e.step     = settings_.value(QString("step_%1").arg(i)).toInt();
        e.ascending = settings_.value(QString("ascending_%1").arg(i)).toBool();
        e.color    = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperStepPlot2::saveSettings() {
    settings_.beginGroup("StepPlot2");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("series_%1").arg(i), e.series);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("phase_%1").arg(i), e.phase);
        settings_.setValue(QString("value_%1").arg(i), e.value);
        settings_.setValue(QString("step_%1").arg(i), e.step);
        settings_.setValue(QString("ascending_%1").arg(i), e.ascending);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
