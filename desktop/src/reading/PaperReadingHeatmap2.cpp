#include "reading/PaperReadingHeatmap2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <cmath>

PaperReadingHeatmap2::PaperReadingHeatmap2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingHeatmap2")
{
    setupUI();
    loadSettings();
}

void PaperReadingHeatmap2::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Morning", "Afternoon", "Evening", "Night", "Weekend"});
    connect(categoryCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperReadingHeatmap2::onRefresh);
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Filter paper...");
    connect(inputField_, &QLineEdit::textChanged, this, &PaperReadingHeatmap2::onRefresh);
    toolbar->addWidget(inputField_);

    toolbar->addStretch();

    refreshBtn_ = new QPushButton("Refresh");
    refreshBtn_->setStyleSheet("color: #3b82f6;");
    connect(refreshBtn_, &QPushButton::clicked, this, &PaperReadingHeatmap2::onRefresh);
    toolbar->addWidget(refreshBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingHeatmap2::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Reading intensity heatmap across time slots");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(640, 520);
}

void PaperReadingHeatmap2::addEntry(const ReadingHeatmap2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit heatUpdated(entry.id, entry.intensity);
    update();
}

QList<ReadingHeatmap2Entry> PaperReadingHeatmap2::entries() const {
    return entries_;
}

int PaperReadingHeatmap2::peakCount() const {
    int count = 0;
    for (const auto& e : entries_) {
        if (e.peak) ++count;
    }
    return count;
}

qreal PaperReadingHeatmap2::totalMinutes() const {
    qreal total = 0;
    for (const auto& e : entries_) {
        total += e.minutes;
    }
    return total;
}

QMap<QString, int> PaperReadingHeatmap2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) {
        counts[e.category]++;
    }
    return counts;
}

void PaperReadingHeatmap2::onRefresh() {
    updateInfo();
    update();
}

void PaperReadingHeatmap2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Reading intensity heatmap across time slots");
    update();
}

void PaperReadingHeatmap2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No reading entries yet");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Intensity Heatmap");

    int w = width(), h = height();
    drawHeatmap(p, QRect(20, 50, w - 40, h / 2 - 20));
    drawCategoryChart(p, QRect(20, h / 2 + 40, w / 2 - 30, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 40, w / 2 - 30, h / 2 - 60));
}

void PaperReadingHeatmap2::drawHeatmap(QPainter& p, const QRect& rect) {
    static const QStringList categories = {"Morning", "Afternoon", "Evening", "Night", "Weekend"};
    static const QColor palette[] = {
        QColor(0x3b, 0x82, 0xf6),
        QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06),
        QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };

    // Filter entries by selected category
    QString selectedCategory = categoryCombo_->currentText();
    QString filterText = inputField_->text().trimmed().toLower();

    QList<ReadingHeatmap2Entry> filtered;
    for (const auto& e : entries_) {
        if (selectedCategory != "All" && e.category != selectedCategory) continue;
        if (!filterText.isEmpty() && !e.paper.toLower().contains(filterText)) continue;
        filtered.append(e);
    }

    if (filtered.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 10));
        p.drawText(rect, Qt::AlignCenter, "No matching entries");
        return;
    }

    // Draw row labels (days)
    QMap<QString, QList<ReadingHeatmap2Entry>> byDay;
    for (const auto& e : filtered) {
        byDay[e.day].append(e);
    }
    QStringList days = byDay.keys();
    std::sort(days.begin(), days.end());

    int cellSize = qMin(28, (rect.height() - 30) / qMax(1, days.size()));
    int colW = qMin(120, (rect.width() - 60) / qMax(1, categories.size()));

    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 8));
    for (int c = 0; c < categories.size(); ++c) {
        int x = rect.x() + 55 + c * colW;
        p.drawText(x, rect.y() + 12, categories[c]);
    }

    for (int r = 0; r < days.size(); ++r) {
        int y = rect.y() + 22 + r * (cellSize + 3);

        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + 2, 50, cellSize, Qt::AlignVCenter | Qt::AlignRight, days[r]);

        const auto& dayEntries = byDay[days[r]];
        for (int c = 0; c < categories.size(); ++c) {
            int x = rect.x() + 55 + c * colW;

            // Find matching entry for this day+category
            const ReadingHeatmap2Entry* matched = nullptr;
            for (const auto& e : dayEntries) {
                if (e.category == categories[c]) {
                    matched = &e;
                    break;
                }
            }

            QColor baseColor = palette[c];
            QColor cellColor;
            if (matched) {
                qreal opacity = qBound(0.15, matched->intensity, 1.0);
                cellColor = QColor(baseColor.red(), baseColor.green(), baseColor.blue(),
                                   static_cast<int>(opacity * 255));
            } else {
                cellColor = QColor(241, 245, 249);
            }

            p.setPen(Qt::NoPen);
            p.setBrush(cellColor);
            p.drawRoundedRect(x, y, colW - 4, cellSize, 4, 4);

            if (matched && matched->peak) {
                p.setPen(QColor(255, 255, 255));
                p.setFont(QFont("Arial", 7, QFont::Bold));
                p.drawText(x, y, colW - 4, cellSize, Qt::AlignCenter, QString("%1m").arg(matched->minutes));
            }
        }
    }
}

void PaperReadingHeatmap2::drawCategoryChart(QPainter& p, const QRect& rect) {
    static const QStringList categories = {"Morning", "Afternoon", "Evening", "Night", "Weekend"};
    static const QColor palette[] = {
        QColor(0x3b, 0x82, 0xf6),
        QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06),
        QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(rect.topLeft(), "Category Distribution");

    QMap<QString, int> counts = categoryCounts();

    int maxVal = 1;
    for (const auto& cat : categories) {
        maxVal = qMax(maxVal, counts.value(cat, 0));
    }

    int barW = qMin(45, (rect.width() - 10) / categories.size());
    int chartBottom = rect.bottom() - 20;
    int chartHeight = rect.height() - 45;

    for (int i = 0; i < categories.size(); ++i) {
        int x = rect.x() + i * barW + 5;
        int count = counts.value(categories[i], 0);
        qreal h = (static_cast<qreal>(count) / maxVal) * chartHeight;

        p.setPen(Qt::NoPen);
        p.setBrush(count > 0 ? palette[i] : QColor(241, 245, 249));
        p.drawRoundedRect(x, chartBottom - static_cast<int>(h), barW - 6,
                          static_cast<int>(h), 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(x - 2, chartBottom + 4, barW - 2, 14, Qt::AlignCenter,
                   categories[i].left(3));

        if (count > 0) {
            p.setPen(palette[i]);
            p.setFont(QFont("Arial", 8, QFont::Bold));
            p.drawText(x - 2, chartBottom - static_cast<int>(h) - 12, barW - 2, 12,
                       Qt::AlignCenter, QString::number(count));
        }
    }
}

void PaperReadingHeatmap2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries",   QString::number(entries_.size()),    QColor(0x3b, 0x82, 0xf6)},
        {"Peak",      QString::number(peakCount()),        QColor(0x16, 0xa3, 0x4a)},
        {"Minutes",   QString::number(static_cast<int>(totalMinutes())), QColor(0xd9, 0x77, 0x06)},
        {"Avg Int.",  QString::number(entries_.isEmpty() ? 0.0 :
                     totalMinutes() / entries_.size(), 'f', 1),          QColor(0xdc, 0x26, 0x26)}
    };

    int boxW = qMin(110, (rect.width() - 10) / 2);
    int boxH = 42;

    for (int i = 0; i < stats.size(); ++i) {
        int col = i % 2;
        int row = i / 2;
        int x = rect.x() + col * (boxW + 5);
        int y = rect.y() + row * (boxH + 5);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(x, y, boxW, boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(x + 6, y + 4, boxW - 12, 22, Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(x + 6, y + 26, boxW - 12, 14, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperReadingHeatmap2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Reading intensity heatmap across time slots");
        return;
    }
    infoLabel_->setText(QString("%1 entries | %2 peak | %3 min | avg %4 min")
        .arg(entries_.size())
        .arg(peakCount())
        .arg(static_cast<int>(totalMinutes()))
        .arg(totalMinutes() / entries_.size(), 0, 'f', 1));
}

void PaperReadingHeatmap2::loadSettings() {
    static const QStringList categories = {"Morning", "Afternoon", "Evening", "Night", "Weekend"};
    static const QColor palette[] = {
        QColor(0x3b, 0x82, 0xf6),
        QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06),
        QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };
    static const QStringList days = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};

    int size = settings_.beginReadArray("entries");
    if (size == 0) {
        settings_.endArray();
        // Seed 8 entries
        QRandomGenerator rng(42);
        QStringList papers = {
            "Attention Is All You Need", "BERT: Pre-training of Deep Bidirectional Transformers",
            "GPT-4 Technical Report", "ResNet: Deep Residual Learning",
            "Diffusion Models Beat GANs", "Transformer-XL",
            "Vision Transformer", "Language Models are Few-Shot Learners"
        };
        for (int i = 0; i < 8; ++i) {
            ReadingHeatmap2Entry e;
            e.id = i + 1;
            e.paper = papers[i];
            e.category = categories[i % 5];
            e.day = days[i % 7];
            e.intensity = 0.3 + rng.bounded(0.7);
            e.minutes = 15 + rng.bounded(105);
            e.peak = (e.intensity > 0.7);
            e.color = palette[i % 5];
            entries_.append(e);
        }
        saveSettings();
    } else {
        for (int i = 0; i < size; ++i) {
            settings_.setArrayIndex(i);
            ReadingHeatmap2Entry e;
            e.id = settings_.value("id").toInt();
            e.paper = settings_.value("paper").toString();
            e.category = settings_.value("category").toString();
            e.day = settings_.value("day").toString();
            e.intensity = settings_.value("intensity").toReal();
            e.minutes = settings_.value("minutes").toInt();
            e.peak = settings_.value("peak").toBool();
            e.color = QColor(settings_.value("color").toString());
            entries_.append(e);
        }
        settings_.endArray();
    }
    updateInfo();
}

void PaperReadingHeatmap2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& e = entries_[i];
        settings_.setValue("id", e.id);
        settings_.setValue("paper", e.paper);
        settings_.setValue("category", e.category);
        settings_.setValue("day", e.day);
        settings_.setValue("intensity", e.intensity);
        settings_.setValue("minutes", e.minutes);
        settings_.setValue("peak", e.peak);
        settings_.setValue("color", e.color.name());
    }
    settings_.endArray();
}
