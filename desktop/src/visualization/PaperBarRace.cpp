#include "visualization/PaperBarRace.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <algorithm>

PaperBarRace::PaperBarRace(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "BarRace")
{
    setupUI();
    loadSettings();
    if (entries_.isEmpty()) {
        QStringList labels = {"Machine Learning", "NLP", "Computer Vision",
                              "Robotics", "Blockchain"};
        QStringList categories = {"AI", "Systems", "Theory", "Applied",
                                  "Interdisciplinary"};
        QStringList metrics = {"citations", "papers", "h-index"};
        QColor palette[] = {QColor("#3b82f6"), QColor("#16a34a"),
                            QColor("#d97706"), QColor("#dc2626"),
                            QColor("#7c3aed")};
        for (int i = 0; i < 5; ++i) {
            BarRaceEntry e;
            e.id = i + 1;
            e.label = labels[i];
            e.category = categories[i];
            e.metric = metrics[i % metrics.size()];
            e.value = 50.0 + QRandomGenerator::global()->bounded(5000) / 10.0;
            e.rank = 0;
            e.leading = false;
            e.color = palette[i];
            entries_.append(e);
        }
        for (int i = 5; i < 8; ++i) {
            BarRaceEntry e;
            e.id = i + 1;
            e.label = labels[i % labels.size()] + " " + QString::number(i - 4);
            e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
            e.metric = metrics[QRandomGenerator::global()->bounded(metrics.size())];
            e.value = 20.0 + QRandomGenerator::global()->bounded(4000) / 10.0;
            e.rank = 0;
            e.leading = false;
            e.color = palette[i % 5];
            entries_.append(e);
        }
        // Assign ranks and leading flag
        std::sort(entries_.begin(), entries_.end(),
                  [](const BarRaceEntry& a, const BarRaceEntry& b) {
                      return a.value > b.value;
                  });
        for (int i = 0; i < entries_.size(); ++i) {
            entries_[i].rank = i + 1;
            entries_[i].leading = (i < 3);
        }
        saveSettings();
        updateInfo();
    }
}

void PaperBarRace::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "AI", "Systems", "Theory", "Applied",
                              "Interdisciplinary"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; "
        "border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Search labels...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; "
        "border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    animateBtn_ = new QPushButton("Animate");
    animateBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; "
        "padding: 4px 12px; border-radius: 4px; }");
    connect(animateBtn_, &QPushButton::clicked, this,
            &PaperBarRace::onAnimate);
    toolbar->addWidget(animateBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperBarRace::onClear);
    toolbar->addWidget(clearBtn_);

    toolbar->addStretch();

    infoLabel_ = new QLabel("Bar Race Chart");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    infoLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    toolbar->addWidget(infoLabel_);

    mainLayout->addLayout(toolbar);
    setMinimumSize(700, 520);
}

void PaperBarRace::addEntry(const BarRaceEntry& entry) {
    entries_.append(entry);
    // Recompute ranks
    std::sort(entries_.begin(), entries_.end(),
              [](const BarRaceEntry& a, const BarRaceEntry& b) {
                  return a.value > b.value;
              });
    for (int i = 0; i < entries_.size(); ++i) {
        entries_[i].rank = i + 1;
        entries_[i].leading = (i < 3);
    }
    saveSettings();
    updateInfo();
    emit raceUpdated(entry.id, entry.value);
    update();
}

QList<BarRaceEntry> PaperBarRace::entries() const { return entries_; }

int PaperBarRace::leadingCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.leading) ++c;
    return c;
}

qreal PaperBarRace::topValue() const {
    if (entries_.isEmpty()) return 0.0;
    qreal maxVal = entries_[0].value;
    for (const auto& e : entries_)
        if (e.value > maxVal) maxVal = e.value;
    return maxVal;
}

QMap<QString, int> PaperBarRace::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperBarRace::onAnimate() {
    QStringList labels = {"Machine Learning", "NLP", "Computer Vision",
                          "Robotics", "Blockchain"};
    QStringList categories = {"AI", "Systems", "Theory", "Applied",
                              "Interdisciplinary"};
    QStringList metrics = {"citations", "papers", "h-index"};
    QColor palette[] = {QColor("#3b82f6"), QColor("#16a34a"),
                        QColor("#d97706"), QColor("#dc2626"),
                        QColor("#7c3aed")};

    BarRaceEntry e;
    e.id = entries_.size() + 1;
    e.label = labels[QRandomGenerator::global()->bounded(labels.size())];
    e.category = categories[QRandomGenerator::global()->bounded(
        categories.size())];
    e.metric = metrics[QRandomGenerator::global()->bounded(metrics.size())];
    e.value = 10.0 + QRandomGenerator::global()->bounded(5000) / 10.0;
    e.rank = 0;
    e.leading = false;
    e.color = palette[QRandomGenerator::global()->bounded(5)];
    addEntry(e);
}

void PaperBarRace::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Bar Race Chart");
    update();
}

void PaperBarRace::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No entries - click Animate");
        return;
    }

    // Apply filter based on category combo
    QString selectedCategory = categoryCombo_->currentText();
    QString searchText = inputField_->text().trimmed().toLower();

    QList<BarRaceEntry> filtered;
    for (const auto& e : entries_) {
        if (selectedCategory != "All" && e.category != selectedCategory)
            continue;
        if (!searchText.isEmpty() && !e.label.toLower().contains(searchText))
            continue;
        filtered.append(e);
    }

    if (filtered.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No matching entries");
        return;
    }

    int w = width();
    int h = height();

    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Bar Race Chart");

    // Layout: bar race takes left 60% height, legend takes right 40%, stats bottom 25%
    int topY = 45;
    int statsH = static_cast<int>(h * 0.25);
    int mainH = h - topY - statsH - 10;
    int legendW = static_cast<int>(w * 0.40);
    int barW = w - legendW - 30;

    drawBarRace(p, QRect(15, topY, barW, mainH));
    drawCategoryLegend(p, QRect(w - legendW - 10, topY, legendW, mainH));
    drawStats(p, QRect(15, topY + mainH + 10, w - 30, statsH - 10));
}

void PaperBarRace::drawBarRace(QPainter& p, const QRect& rect) {
    // Get filtered entries, sorted by value descending
    QString selectedCategory = categoryCombo_->currentText();
    QString searchText = inputField_->text().trimmed().toLower();

    QList<BarRaceEntry> filtered;
    for (const auto& e : entries_) {
        if (selectedCategory != "All" && e.category != selectedCategory)
            continue;
        if (!searchText.isEmpty() && !e.label.toLower().contains(searchText))
            continue;
        filtered.append(e);
    }
    std::sort(filtered.begin(), filtered.end(),
              [](const BarRaceEntry& a, const BarRaceEntry& b) {
                  return a.value > b.value;
              });

    if (filtered.isEmpty()) return;

    int margin = 8;
    int rankColW = 30;
    int labelColW = 130;
    int barStartX = rect.x() + margin + rankColW + labelColW;
    int barAreaW = rect.width() - rankColW - labelColW - 3 * margin;
    qreal maxVal = filtered.first().value;
    if (maxVal <= 0) maxVal = 1.0;

    int barH = qMin(28, (rect.height() - 2 * margin) /
                            static_cast<int>(filtered.size()) - 4);
    barH = qMax(barH, 14);

    for (int i = 0; i < filtered.size(); ++i) {
        const auto& e = filtered[i];
        int y = rect.y() + margin + i * (barH + 4);
        if (y + barH > rect.y() + rect.height()) break;

        // Rank number
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 10, QFont::Bold));
        p.drawText(rect.x() + margin, y, rankColW, barH, Qt::AlignVCenter,
                   "#" + QString::number(i + 1));

        // Label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        QString elided = p.fontMetrics().elidedText(
            e.label, Qt::ElideRight, labelColW - 4);
        p.drawText(rect.x() + margin + rankColW, y, labelColW, barH,
                   Qt::AlignVCenter, elided);

        // Bar
        int barLen = static_cast<int>((e.value / maxVal) * barAreaW);
        barLen = qMax(barLen, 2);

        // Background track
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(241, 245, 249));
        p.drawRoundedRect(barStartX, y + 2, barAreaW, barH - 4, barH / 2 - 2,
                          barH / 2 - 2);

        // Filled bar with rounded ends
        p.setBrush(e.color);
        p.drawRoundedRect(barStartX, y + 2, barLen, barH - 4, barH / 2 - 2,
                          barH / 2 - 2);

        // Gold border for leading entries
        if (e.leading) {
            p.setPen(QPen(QColor(255, 215, 0), 2));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(barStartX - 1, y + 1, barLen + 2, barH - 2,
                              barH / 2, barH / 2);
        }

        // Value label at end of bar
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        QString valText = QString::number(e.value, 'f', 1);
        int valX = barStartX + barLen + 4;
        if (valX + 40 > rect.x() + rect.width())
            valX = barStartX + barLen - 40;
        p.drawText(valX, y, 50, barH, Qt::AlignVCenter, valText);
    }
}

void PaperBarRace::drawCategoryLegend(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts();

    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.x(), rect.y() + 14, "Categories");

    QStringList categories = {"AI", "Systems", "Theory", "Applied",
                              "Interdisciplinary"};
    QColor palette[] = {QColor("#3b82f6"), QColor("#16a34a"),
                        QColor("#d97706"), QColor("#dc2626"),
                        QColor("#7c3aed")};

    int itemH = qMin(30, (rect.height() - 40) / static_cast<int>(categories.size()));
    for (int i = 0; i < categories.size(); ++i) {
        int y = rect.y() + 26 + i * (itemH + 4);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;

        // Color swatch
        p.setPen(Qt::NoPen);
        p.setBrush(palette[i]);
        p.drawRoundedRect(rect.x() + 5, y + 4, 16, 16, 3, 3);

        // Category name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 28, y + 2, rect.width() / 2 - 28, 20,
                   Qt::AlignVCenter, categories[i]);

        // Count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5,
                   20, Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(count) + " entries");
    }
}

void PaperBarRace::drawStats(QPainter& p, const QRect& rect) {
    struct Stat {
        QString label;
        QString value;
        QColor color;
    };

    qreal avgRank = 0.0;
    if (!entries_.isEmpty()) {
        for (const auto& e : entries_) avgRank += e.rank;
        avgRank /= entries_.size();
    }

    QList<Stat> stats = {
        {"Total Entries", QString::number(entries_.size()), QColor("#3b82f6")},
        {"Leading Count", QString::number(leadingCount()), QColor("#16a34a")},
        {"Top Value", QString::number(topValue(), 'f', 1), QColor("#d97706")},
        {"Avg Rank", QString::number(avgRank, 'f', 1), QColor("#7c3aed")},
    };

    int boxW = (rect.width() - 30) / 4;
    int boxH = qMin(50, rect.height() - 6);

    for (int i = 0; i < stats.size(); ++i) {
        int x = rect.x() + i * (boxW + 10);
        int y = rect.y();

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(x, y, boxW, boxH, 6, 6);

        // Top colored accent line
        p.setBrush(stats[i].color);
        p.drawRoundedRect(x, y, boxW, 3, 2, 2);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(x + 10, y + 8, boxW - 20, 22, Qt::AlignVCenter,
                   stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(x + 10, y + 28, boxW - 20, 16, Qt::AlignVCenter,
                   stats[i].label);
    }
}

void PaperBarRace::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Bar Race Chart");
        return;
    }
    infoLabel_->setText(
        QString("%1 entries | %2 leading | top: %3")
            .arg(entries_.size())
            .arg(leadingCount())
            .arg(topValue(), 0, 'f', 1));
}

void PaperBarRace::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        BarRaceEntry e;
        e.id = settings_.value("id").toInt();
        e.label = settings_.value("label").toString();
        e.category = settings_.value("category").toString();
        e.metric = settings_.value("metric").toString();
        e.value = settings_.value("value").toDouble();
        e.rank = settings_.value("rank").toInt();
        e.leading = settings_.value("leading").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperBarRace::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("label", entries_[i].label);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("metric", entries_[i].metric);
        settings_.setValue("value", entries_[i].value);
        settings_.setValue("rank", entries_[i].rank);
        settings_.setValue("leading", entries_[i].leading);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
