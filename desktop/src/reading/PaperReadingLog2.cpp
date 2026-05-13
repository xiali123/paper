#include "reading/PaperReadingLog2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperReadingLog2::PaperReadingLog2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingLog2")
{
    setupUI();
    loadSettings();

    if (entries_.isEmpty()) {
        QStringList categories = {"Theory", "Empirical", "Review", "Survey", "Case Study"};
        QStringList journals = {"Nature", "Science", "ICML", "NeurIPS", "AAAI", "CVPR", "ACL"};
        QStringList titles = {
            "Deep Transformer Models", "Statistical Learning Bounds",
            "Graph Neural Networks", "Attention Mechanism Survey",
            "Reinforcement Learning in Robotics", "Meta-Learning Strategies",
            "Causal Inference Methods", "Self-Supervised Representation"
        };
        for (int i = 0; i < 8; ++i) {
            ReadingLog2Entry e;
            e.id = i + 1;
            e.title = titles[i];
            e.category = categories[i % 5];
            e.journal = journals[QRandomGenerator::global()->bounded(journals.size())];
            e.pages = 5.0 + QRandomGenerator::global()->bounded(200) / 10.0;
            e.minutes = 15 + QRandomGenerator::global()->bounded(180);
            e.finished = QRandomGenerator::global()->bounded(3) != 0;

            QList<QColor> catColors = {
                QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
                QColor("#dc2626"), QColor("#7c3aed")
            };
            int catIdx = categories.indexOf(e.category);
            e.color = catIdx >= 0 ? catColors[catIdx] : catColors[0];

            entries_.append(e);
        }
        saveSettings();
        updateInfo();
        update();
    }
}

void PaperReadingLog2::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Theory", "Empirical", "Review", "Survey", "Case Study"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter paper title...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 2);

    logBtn_ = new QPushButton("Log");
    logBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 14px; border-radius: 4px; }");
    connect(logBtn_, &QPushButton::clicked, this, &PaperReadingLog2::onLog);
    toolbar->addWidget(logBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingLog2::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Log your reading progress");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(620, 520);
}

void PaperReadingLog2::addEntry(const ReadingLog2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit logUpdated(entry.id, entry.pages);
    update();
}

QList<ReadingLog2Entry> PaperReadingLog2::entries() const {
    return entries_;
}

int PaperReadingLog2::finishedCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.finished) c++;
    return c;
}

qreal PaperReadingLog2::totalPages() const {
    qreal total = 0;
    for (const auto& e : entries_)
        total += e.pages;
    return total;
}

QMap<QString, int> PaperReadingLog2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperReadingLog2::onLog() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"Theory", "Empirical", "Review", "Survey", "Case Study"};
    QList<QColor> catColors = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };
    QStringList journals = {"Nature", "Science", "ICML", "NeurIPS", "AAAI", "CVPR", "ACL"};

    int cIdx = categoryCombo_->currentIndex();
    QString category = (cIdx > 0 && cIdx <= categories.size())
                           ? categories[cIdx - 1]
                           : categories[QRandomGenerator::global()->bounded(categories.size())];
    int catColorIdx = categories.indexOf(category);

    ReadingLog2Entry e;
    e.id = entries_.size() + 1;
    e.title = text;
    e.category = category;
    e.journal = journals[QRandomGenerator::global()->bounded(journals.size())];
    e.pages = 5.0 + QRandomGenerator::global()->bounded(200) / 10.0;
    e.minutes = 15 + QRandomGenerator::global()->bounded(180);
    e.finished = QRandomGenerator::global()->bounded(2) == 0;
    e.color = catColors[qMax(0, catColorIdx)];

    addEntry(e);
    inputField_->clear();
}

void PaperReadingLog2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Log your reading progress");
    update();
}

void PaperReadingLog2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Log your reading progress");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Log");

    int w = width(), h = height();
    drawLogView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReadingLog2::drawLogView(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(36, (rect.height() - 10) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 3);

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        // Left color stripe
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        // Title and journal
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 2, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.title.left(18));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 18, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.journal + " | " + e.category);

        // Page progress and reading time on the right
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.pages, 'f', 1) + " pg | " + QString::number(e.minutes) + " min");

        // Finish status badge
        QString status = e.finished ? "Done" : "Reading";
        QColor statusColor = e.finished ? QColor("#16a34a") : QColor("#d97706");
        p.setPen(Qt::NoPen);
        p.setBrush(statusColor);
        p.drawRoundedRect(rect.x() + rect.width() - 44, y + 20, 36, 14, 3, 3);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(rect.x() + rect.width() - 44, y + 20, 36, 14,
                   Qt::AlignCenter, status);
    }
}

void PaperReadingLog2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Category Distribution");

    auto counts = categoryCounts();
    QStringList categories = {"Theory", "Empirical", "Review", "Survey", "Case Study"};
    QList<QColor> catColors = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };

    int total = 0;
    for (const auto& cat : categories)
        total += counts.contains(cat) ? counts[cat] : 0;

    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 30 + (rect.height() - 60) / 2;
    int outerR = qMin(rect.width(), rect.height() - 60) / 2 - 10;
    int innerR = outerR * 55 / 100;

    if (total == 0) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect, Qt::AlignCenter, "No data");
        return;
    }

    qreal startAngle = 0.0;
    for (int i = 0; i < categories.size(); ++i) {
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        if (count == 0) continue;
        qreal span = 360.0 * count / total;

        p.setPen(Qt::NoPen);
        p.setBrush(catColors[i]);
        p.drawPie(cx - outerR, cy - outerR, outerR * 2, outerR * 2,
                  qFloor(startAngle * 16), qFloor(span * 16));

        startAngle += span;
    }

    // Donut hole
    p.setBrush(Qt::white);
    p.drawEllipse(cx - innerR, cy - innerR, innerR * 2, innerR * 2);

    // Center text
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 14, QFont::Bold));
    p.drawText(cx - innerR, cy - 10, innerR * 2, 20,
               Qt::AlignCenter, QString::number(entries_.size()));
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 7));
    p.drawText(cx - innerR, cy + 6, innerR * 2, 14,
               Qt::AlignCenter, "entries");

    // Legend
    int legendY = rect.y() + rect.height() - 20;
    int legendX = rect.x();
    p.setFont(QFont("Arial", 7));
    for (int i = 0; i < categories.size(); ++i) {
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        p.setPen(Qt::NoPen);
        p.setBrush(catColors[i]);
        p.drawRoundedRect(legendX, legendY, 8, 8, 2, 2);
        p.setPen(QColor(100, 116, 139));
        p.drawText(legendX + 10, legendY + 8,
                   categories[i].left(5) + " " + QString::number(count));
        legendX += 58;
    }
}

void PaperReadingLog2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total",       QString::number(entries_.size()), QColor("#3b82f6")},
        {"Finished",    QString::number(finishedCount()), QColor("#16a34a")},
        {"Total Pages", QString::number(totalPages(), 'f', 1), QColor("#d97706")},
        {"Total Minutes", QString::number(
            [](const QList<ReadingLog2Entry>& l) -> int {
                int s = 0; for (const auto& e : l) s += e.minutes; return s;
            }(entries_)), QColor("#dc2626")}
    };

    int cols = 2, rows = 2;
    int gap = 6;
    int boxW = (rect.width() - gap * (cols - 1)) / cols;
    int boxH = (rect.height() - gap * (rows - 1)) / rows;

    for (int i = 0; i < stats.size(); ++i) {
        int col = i % cols;
        int row = i / cols;
        int bx = rect.x() + col * (boxW + gap);
        int by = rect.y() + row * (boxH + gap);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(bx, by, boxW, boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(bx + 8, by + 4, boxW - 16, boxH / 2, Qt::AlignVCenter,
                   stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(bx + 8, by + boxH / 2, boxW - 16, boxH / 2, Qt::AlignVCenter,
                   stats[i].label);
    }
}

void PaperReadingLog2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Log your reading progress");
        return;
    }
    infoLabel_->setText(QString("%1 entries | %2 finished | %3 pages | %4 min")
        .arg(entries_.size())
        .arg(finishedCount())
        .arg(totalPages(), 0, 'f', 1)
        .arg([](const QList<ReadingLog2Entry>& l) -> int {
            int s = 0; for (const auto& e : l) s += e.minutes; return s;
        }(entries_)));
}

void PaperReadingLog2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ReadingLog2Entry e;
        e.id = settings_.value("id").toInt();
        e.title = settings_.value("title").toString();
        e.category = settings_.value("category").toString();
        e.journal = settings_.value("journal").toString();
        e.pages = settings_.value("pages").toDouble();
        e.minutes = settings_.value("minutes").toInt();
        e.finished = settings_.value("finished").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingLog2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("title", entries_[i].title);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("journal", entries_[i].journal);
        settings_.setValue("pages", entries_[i].pages);
        settings_.setValue("minutes", entries_[i].minutes);
        settings_.setValue("finished", entries_[i].finished);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
