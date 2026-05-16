#include "visualization/PaperJoyPlot2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>
#include <QPainterPath>

PaperJoyPlot2::PaperJoyPlot2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "JoyPlot2")
{
    setupUI();
    loadSettings();
}

void PaperJoyPlot2::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperJoyPlot2::onRender);
    toolbar->addWidget(renderBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Normal", "Skewed", "Bimodal", "Uniform", "Exponential"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperJoyPlot2::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter variable:density (e.g. Temperature:0.85, Pressure:0.62, Humidity:0.91)");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Render joy plot");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperJoyPlot2::addEntry(const JoyPlot2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit distributionSelected(entry.id, entry.density);
    update();
}

QList<JoyPlot2Entry> PaperJoyPlot2::entries() const { return entries_; }

int PaperJoyPlot2::skewedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.skewed) c++;
    return c;
}

qreal PaperJoyPlot2::avgDensity() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.density;
    return sum / entries_.size();
}

QMap<QString, int> PaperJoyPlot2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperJoyPlot2::onRender() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"Normal", "Skewed", "Bimodal", "Uniform", "Exponential"};
    QColor palette[] = {
        QColor(0x3b, 0x82, 0xf6),  // #3b82f6
        QColor(0x16, 0xa3, 0x4a),  // #16a34a
        QColor(0xd9, 0x77, 0x06),  // #d97706
        QColor(0xdc, 0x26, 0x26),  // #dc2626
        QColor(0x7c, 0x3a, 0xed)   // #7c3aed
    };

    entries_.clear();
    int cIdx = categoryCombo_->currentIndex();

    QStringList parts = text.split(',', Qt::SkipEmptyParts);
    int nextId = 1;
    for (const auto& part : parts) {
        QStringList kv = part.split(':', Qt::SkipEmptyParts);
        if (kv.size() < 2) continue;

        QString variable = kv[0].trimmed();
        bool ok = false;
        qreal density = kv[1].trimmed().toDouble(&ok);
        if (!ok) continue;

        JoyPlot2Entry e;
        e.id = nextId++;
        e.variable = variable;
        e.distribution = variable.toLower().replace(' ', '-');
        e.category = cIdx == 0
            ? categories[QRandomGenerator::global()->bounded(categories.size())]
            : categories[cIdx - 1];
        e.density = qBound(0.0, density, 1.0);
        e.samples = 50 + QRandomGenerator::global()->bounded(950);
        e.skewed = e.density > 0.7;
        e.color = palette[(nextId - 2) % 5];
        entries_.append(e);
    }

    saveSettings();
    updateInfo();
    if (!entries_.isEmpty()) {
        emit distributionSelected(entries_.last().id, entries_.last().density);
    }
    update();
    inputField_->clear();
}

void PaperJoyPlot2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Render joy plot");
    update();
}

void PaperJoyPlot2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Render joy plot");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Joy Plot 2 (Distribution Ridges)");

    int w = width(), h = height();
    drawJoyPlot(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperJoyPlot2::drawJoyPlot(QPainter& p, const QRect& rect) {
    int n = entries_.size();
    if (n == 0) return;

    int marginX = 50;
    int marginTop = 10;
    int marginBottom = 30;
    int plotW = rect.width() - 2 * marginX;

    qreal globalMaxDensity = 0;
    for (const auto& e : entries_) globalMaxDensity = qMax(globalMaxDensity, e.density);
    if (globalMaxDensity <= 0) globalMaxDensity = 1.0;

    int numRidges = n;
    int ridgeSpacing = qMax(25, (rect.height() - marginTop - marginBottom) / qMax(1, numRidges));

    for (int i = 0; i < n; ++i) {
        const auto& entry = entries_[i];

        int baseY = rect.y() + marginTop + i * ridgeSpacing;
        int curveH = static_cast<int>((entry.density / globalMaxDensity) * ridgeSpacing * 1.8);

        int numPoints = 80;
        qreal centerX = 0.5;
        qreal spread = 0.12;

        if (entry.category == "Skewed" || entry.category == "Exponential") {
            centerX = 0.35;
            spread = 0.10;
        } else if (entry.category == "Bimodal") {
            centerX = 0.5;
            spread = 0.18;
        } else if (entry.category == "Uniform") {
            spread = 0.25;
        }

        QPainterPath fillPath;
        fillPath.moveTo(rect.x() + marginX, baseY);

        for (int pi = 0; pi <= numPoints; ++pi) {
            qreal t = static_cast<qreal>(pi) / numPoints;
            qreal xPos = t;

            qreal density = 0.0;
            if (entry.category == "Bimodal") {
                qreal diff1 = xPos - 0.35;
                qreal diff2 = xPos - 0.65;
                density = qExp(-(diff1 * diff1) / (2.0 * 0.06 * 0.06))
                        + 0.8 * qExp(-(diff2 * diff2) / (2.0 * 0.06 * 0.06));
                density *= 0.5;
            } else if (entry.category == "Uniform") {
                density = (xPos > 0.2 && xPos < 0.8) ? 0.8 : 0.1;
            } else if (entry.category == "Exponential") {
                density = qExp(-3.0 * xPos);
                density *= 0.9;
            } else if (entry.category == "Skewed") {
                qreal diff = xPos - centerX;
                qreal skew = qExp(-(diff * diff) / (2.0 * spread * spread));
                skew *= (1.0 + 0.6 * diff);
                density = qMax(0.0, skew);
            } else {
                qreal diff = xPos - centerX;
                density = qExp(-(diff * diff) / (2.0 * spread * spread));
            }

            density *= entry.density;

            int px = rect.x() + marginX + static_cast<int>(t * plotW);
            int py = baseY - static_cast<int>(density * curveH);
            fillPath.lineTo(px, py);
        }

        fillPath.lineTo(rect.x() + marginX + plotW, baseY);
        fillPath.closeSubpath();

        QColor fill(entry.color.red(), entry.color.green(), entry.color.blue(), 80);
        p.setPen(Qt::NoPen);
        p.setBrush(fill);
        p.drawPath(fillPath);

        QColor strokeColor(entry.color.red(), entry.color.green(), entry.color.blue(), 200);
        p.setPen(QPen(strokeColor, entry.skewed ? 2.5 : 1.5));
        p.setBrush(Qt::NoBrush);
        QPainterPath strokePath;
        for (int pi = 0; pi <= numPoints; ++pi) {
            qreal t = static_cast<qreal>(pi) / numPoints;
            qreal xPos = t;

            qreal density = 0.0;
            if (entry.category == "Bimodal") {
                qreal diff1 = xPos - 0.35;
                qreal diff2 = xPos - 0.65;
                density = qExp(-(diff1 * diff1) / (2.0 * 0.06 * 0.06))
                        + 0.8 * qExp(-(diff2 * diff2) / (2.0 * 0.06 * 0.06));
                density *= 0.5;
            } else if (entry.category == "Uniform") {
                density = (xPos > 0.2 && xPos < 0.8) ? 0.8 : 0.1;
            } else if (entry.category == "Exponential") {
                density = qExp(-3.0 * xPos) * 0.9;
            } else if (entry.category == "Skewed") {
                qreal diff = xPos - centerX;
                qreal skew = qExp(-(diff * diff) / (2.0 * spread * spread));
                skew *= (1.0 + 0.6 * diff);
                density = qMax(0.0, skew);
            } else {
                qreal diff = xPos - centerX;
                density = qExp(-(diff * diff) / (2.0 * spread * spread));
            }

            density *= entry.density;

            int px = rect.x() + marginX + static_cast<int>(t * plotW);
            int py = baseY - static_cast<int>(density * curveH);
            if (pi == 0) strokePath.moveTo(px, py);
            else strokePath.lineTo(px, py);
        }
        p.drawPath(strokePath);

        if (entry.skewed) {
            int markerX = rect.x() + marginX + static_cast<int>(0.5 * plotW);
            int markerY = baseY - static_cast<int>(curveH * 0.75);
            p.setBrush(entry.color);
            p.setPen(Qt::NoPen);
            p.drawEllipse(markerX - 4, markerY - 4, 8, 8);
        }

        p.setPen(QColor(30, 41, 59));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), baseY - 6, marginX - 4, 14,
                   Qt::AlignRight | Qt::AlignVCenter, entry.variable);
    }

    if (numRidges > 0) {
        int lastY = rect.y() + marginTop + (numRidges - 1) * ridgeSpacing;
        p.setPen(QColor(203, 213, 225));
        p.drawLine(rect.x() + marginX, lastY + 4,
                   rect.x() + marginX + plotW, lastY + 4);

        p.setFont(QFont("Arial", 7));
        p.setPen(QColor(148, 163, 184));
        for (int tick = 0; tick <= 10; tick += 2) {
            qreal val = tick / 10.0;
            int tx = rect.x() + marginX + static_cast<int>(val * plotW);
            p.drawText(tx - 10, lastY + 8, 20, 14, Qt::AlignCenter,
                       QString::number(val, 'f', 1));
        }
    }
}

void PaperJoyPlot2::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList categories = {"Normal", "Skewed", "Bimodal", "Uniform", "Exponential"};
    QColor catColors[] = {
        QColor(0x3b, 0x82, 0xf6),
        QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06),
        QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };

    int itemH = qMin(28, (rect.height() - 50) / (categories.size() + 1));
    for (int i = 0; i < categories.size(); ++i) {
        int y = rect.y() + 22 + i * (itemH + 4);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;

        p.setPen(Qt::NoPen);
        p.setBrush(catColors[i]);
        p.drawRoundedRect(rect.x() + 5, y + 3, 14, 14, 3, 3);

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

    int skewY = rect.y() + 22 + categories.size() * (itemH + 4);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0x7c, 0x3a, 0xed));
    p.drawEllipse(rect.x() + 7, skewY + 5, 10, 10);

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 9, QFont::Bold));
    p.drawText(rect.x() + 24, skewY + 2, rect.width() / 2 - 24, 18,
               Qt::AlignVCenter, "Skewed");

    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 8));
    p.drawText(rect.x() + rect.width() / 2, skewY + 2, rect.width() / 2 - 5, 18,
               Qt::AlignVCenter | Qt::AlignRight,
               QString::number(skewedCount()));
}

void PaperJoyPlot2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries", QString::number(entries_.size()), QColor(0x3b, 0x82, 0xf6)},
        {"Skewed", QString::number(skewedCount()), QColor(0x7c, 0x3a, 0xed)},
        {"Avg Density", QString::number(avgDensity(), 'f', 2), QColor(0xd9, 0x77, 0x06)},
        {"Categories", QString::number(categoryCounts().size()), QColor(0x16, 0xa3, 0x4a)}
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

void PaperJoyPlot2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Render joy plot");
        return;
    }
    infoLabel_->setText(QString("%1 entries | %2 skewed | avg density %3")
        .arg(entries_.size())
        .arg(skewedCount())
        .arg(avgDensity(), 0, 'f', 2));
}

void PaperJoyPlot2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        JoyPlot2Entry e;
        e.id = settings_.value("id").toInt();
        e.distribution = settings_.value("distribution").toString();
        e.category = settings_.value("category").toString();
        e.variable = settings_.value("variable").toString();
        e.density = settings_.value("density").toDouble();
        e.samples = settings_.value("samples").toInt();
        e.skewed = settings_.value("skewed").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperJoyPlot2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("distribution", entries_[i].distribution);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("variable", entries_[i].variable);
        settings_.setValue("density", entries_[i].density);
        settings_.setValue("samples", entries_[i].samples);
        settings_.setValue("skewed", entries_[i].skewed);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
