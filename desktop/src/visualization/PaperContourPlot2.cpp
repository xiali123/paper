#include "visualization/PaperContourPlot2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperContourPlot2::PaperContourPlot2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ContourPlot2")
{
    setupUI();
    loadSettings();
}

void PaperContourPlot2::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperContourPlot2::onRender);
    toolbar->addWidget(renderBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Temperature", "Pressure", "Density", "Velocity", "Concentration"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperContourPlot2::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter variable label for contour plot...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Render contour plot");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperContourPlot2::addEntry(const ContourPlot2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit contourSelected(entry.id, entry.value);
    update();
}

QList<ContourPlot2Entry> PaperContourPlot2::entries() const { return entries_; }

int PaperContourPlot2::peakCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.peak) ++c;
    return c;
}

qreal PaperContourPlot2::avgValue() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.value;
    return sum / entries_.size();
}

QMap<QString, int> PaperContourPlot2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperContourPlot2::onRender() {
    QString text = inputField_->text().trimmed();

    QStringList categories = {"Temperature", "Pressure", "Density", "Velocity", "Concentration"};
    QStringList levels = {"Low", "Medium", "High", "Critical"};
    QColor palette[] = {
        QColor(0x3b, 0x82, 0xf6),  // #3b82f6
        QColor(0x16, 0xa3, 0x4a),  // #16a34a
        QColor(0xd9, 0x77, 0x06),  // #d97706
        QColor(0xdc, 0x26, 0x26),  // #dc2626
        QColor(0x7c, 0x3a, 0xed)   // #7c3aed
    };

    entries_.clear();

    int comboIdx = categoryCombo_->currentIndex();

    // Seed 8 entries
    struct SeedData {
        QString variable;
        qreal value;
        int contours;
        bool peak;
    };
    SeedData seeds[8] = {
        {"T1",  72.5, 6, false},
        {"P2",  95.0, 9, true},
        {"D3",  48.3, 4, false},
        {"V4",  83.7, 7, true},
        {"C5",  61.2, 5, false},
        {"T6",  88.4, 8, true},
        {"P7",  55.1, 4, false},
        {"D8",  76.9, 7, false},
    };

    int catIndex = 0;
    for (int i = 0; i < 8; ++i) {
        ContourPlot2Entry e;
        e.id = i + 1;
        e.variable = text.isEmpty() ? seeds[i].variable : text + QString::number(i + 1);

        if (comboIdx == 0) {
            e.category = categories[catIndex % categories.size()];
            ++catIndex;
        } else {
            e.category = categories[comboIdx - 1];
        }

        e.level = levels[static_cast<int>(seeds[i].value / 25.0)];
        if (e.level.isEmpty()) e.level = "Low";
        e.value = seeds[i].value;
        e.contours = seeds[i].contours;
        e.peak = seeds[i].peak;

        int cIdx = categories.indexOf(e.category);
        if (cIdx < 0) cIdx = 0;
        e.color = palette[cIdx];

        entries_.append(e);
    }

    saveSettings();
    updateInfo();
    emit contourSelected(entries_.size(), avgValue());
    update();
    inputField_->clear();
}

void PaperContourPlot2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Render contour plot");
    update();
}

void PaperContourPlot2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Render contour plot");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Contour Plot 2");

    int w = width(), h = height();
    drawContourPlot(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperContourPlot2::drawContourPlot(QPainter& p, const QRect& rect) {
    int margin = 20;
    int plotW = rect.width() - margin * 2;
    int plotH = rect.height() - margin * 2;
    int cx = rect.x() + margin + plotW / 2;
    int cy = rect.y() + margin + plotH / 2;

    // Draw grid lines
    p.setPen(QPen(QColor(226, 232, 240), 1));
    for (int i = 0; i <= 5; ++i) {
        int x = rect.x() + margin + plotW * i / 5;
        p.drawLine(x, rect.y() + margin, x, rect.y() + margin + plotH);
        int y = rect.y() + margin + plotH * i / 5;
        p.drawLine(rect.x() + margin, y, rect.x() + margin + plotW, y);
    }

    // Sort entries so peaks are drawn on top
    QList<ContourPlot2Entry> sorted = entries_;
    std::stable_sort(sorted.begin(), sorted.end(),
                     [](const ContourPlot2Entry& a, const ContourPlot2Entry& b) {
                         return a.value < b.value;
                     });

    // Draw concentric contour rings for each entry
    int maxRadius = qMin(plotW, plotH) / 2 - 10;
    for (int ei = 0; ei < sorted.size(); ++ei) {
        const auto& e = sorted[ei];
        qreal normalizedValue = e.value / 100.0;

        // Position entries in a grid-like arrangement around center
        int cols = 3;
        int row = ei / cols;
        int col = ei % cols;
        int spacingX = plotW / (cols + 1);
        int spacingY = plotH / ((sorted.size() / cols) + 2);
        int ex = rect.x() + margin + spacingX * (col + 1);
        int ey = rect.y() + margin + spacingY * (row + 1);

        // Draw concentric rings with color gradients
        int rings = e.contours;
        for (int r = rings; r >= 1; --r) {
            qreal ratio = static_cast<qreal>(r) / rings;
            int ringRadius = static_cast<int>(maxRadius * 0.25 * ratio);

            // Gradient from light (outer) to saturated (inner)
            int alpha = 40 + static_cast<int>(160 * (1.0 - ratio));
            QColor ringColor = QColor(
                e.color.red(),
                e.color.green(),
                e.color.blue(),
                qMin(255, alpha));

            p.setPen(Qt::NoPen);
            p.setBrush(ringColor);
            p.drawEllipse(ex - ringRadius, ey - ringRadius, ringRadius * 2, ringRadius * 2);
        }

        // Center marker
        int markerSize = 4;
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawEllipse(ex - markerSize, ey - markerSize, markerSize * 2, markerSize * 2);

        // Peak indicator ring
        if (e.peak) {
            int peakRadius = static_cast<int>(maxRadius * 0.25) + 4;
            p.setPen(QPen(QColor(220, 38, 38, 180), 2, Qt::DashLine));
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(ex - peakRadius, ey - peakRadius, peakRadius * 2, peakRadius * 2);
        }

        // Variable label
        p.setPen(QColor(30, 41, 59));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(ex - 20, ey + static_cast<int>(maxRadius * 0.25) + 12, 40, 14,
                   Qt::AlignCenter, e.variable);
    }
}

void PaperContourPlot2::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    QStringList categories = {"Temperature", "Pressure", "Density", "Velocity", "Concentration"};
    QColor palette[] = {
        QColor(0x3b, 0x82, 0xf6),
        QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06),
        QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };
    auto counts = categoryCounts();
    int itemH = qMin(28, (rect.height() - 30) / 5);

    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (itemH + 4);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;

        p.setPen(Qt::NoPen);
        p.setBrush(palette[i]);
        p.drawEllipse(rect.x() + 5, y + 4, 14, 14);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18,
                   Qt::AlignVCenter, categories[i]);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(count) + " entries");
    }
}

void PaperContourPlot2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries",    QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Peaks",      QString::number(peakCount()),     QColor(220, 38, 38)},
        {"Avg Value",  QString::number(avgValue(), 'f', 1), QColor(217, 119, 6)},
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
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 22,
                   Qt::AlignVCenter, stats[i].value);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 26, rect.width() - 20, 14,
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperContourPlot2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Render contour plot");
        return;
    }
    infoLabel_->setText(QString("%1 entries | %2 peaks | avg %3 | %4 categories")
        .arg(entries_.size())
        .arg(peakCount())
        .arg(avgValue(), 0, 'f', 1)
        .arg(categoryCounts().size()));
}

void PaperContourPlot2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ContourPlot2Entry e;
        e.id        = settings_.value("id").toInt();
        e.variable  = settings_.value("variable").toString();
        e.category  = settings_.value("category").toString();
        e.level     = settings_.value("level").toString();
        e.value     = settings_.value("value").toDouble();
        e.contours  = settings_.value("contours").toInt();
        e.peak      = settings_.value("peak").toBool();
        e.color     = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperContourPlot2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",       entries_[i].id);
        settings_.setValue("variable", entries_[i].variable);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("level",    entries_[i].level);
        settings_.setValue("value",    entries_[i].value);
        settings_.setValue("contours", entries_[i].contours);
        settings_.setValue("peak",     entries_[i].peak);
        settings_.setValue("color",    entries_[i].color.name());
    }
    settings_.endArray();
}
