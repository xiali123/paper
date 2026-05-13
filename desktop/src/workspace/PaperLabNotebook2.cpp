#include "workspace/PaperLabNotebook2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperLabNotebook2::PaperLabNotebook2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "LabNotebook2")
{
    setupUI();
    loadSettings();

    if (entries_.isEmpty()) {
        QStringList categories = {"Experiment", "Analysis", "Protocol", "Observation", "Result"};
        QColor colors[] = {
            QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
            QColor("#dc2626"), QColor("#7c3aed")
        };
        QStringList experiments = {
            "Cell Culture Assay", "PCR Optimization", "Western Blot",
            "ELISA Validation", "Flow Cytometry"
        };
        QStringList notebooks = {
            "Primary Lab Book", "Analysis Journal", "Protocol Registry",
            "Field Notes", "Results Archive"
        };
        for (int i = 0; i < 8; ++i) {
            LabNotebook2Entry e;
            e.id = i + 1;
            e.experiment = experiments[i % experiments.size()];
            e.category = categories[i % categories.size()];
            e.notebook = notebooks[i % notebooks.size()];
            e.progress = 0.2 + QRandomGenerator::global()->bounded(80) / 100.0;
            e.entries = 3 + QRandomGenerator::global()->bounded(18);
            e.pinned = i < 2;
            e.color = colors[i % 5];
            entries_.append(e);
        }
        saveSettings();
        updateInfo();
        update();
    }
}

void PaperLabNotebook2::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Experiment", "Analysis", "Protocol", "Observation", "Result"});
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter experiment name...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 2);

    recordBtn_ = new QPushButton("Record");
    recordBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(recordBtn_, &QPushButton::clicked, this, &PaperLabNotebook2::onRecord);
    toolbar->addWidget(recordBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperLabNotebook2::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Record notebook entries");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(640, 520);
}

void PaperLabNotebook2::addEntry(const LabNotebook2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    if (entry.pinned) {
        emit entryPinned(entry.id, entry.progress);
    }
    update();
}

QList<LabNotebook2Entry> PaperLabNotebook2::entries() const {
    return entries_;
}

int PaperLabNotebook2::pinnedCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.pinned) c++;
    return c;
}

qreal PaperLabNotebook2::avgProgress() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_)
        sum += e.progress;
    return sum / entries_.size();
}

QMap<QString, int> PaperLabNotebook2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperLabNotebook2::onRecord() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"Experiment", "Analysis", "Protocol", "Observation", "Result"};
    QColor colors[] = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };
    int cIdx = categoryCombo_->currentIndex();
    int catIdx = cIdx == 0 ? QRandomGenerator::global()->bounded(categories.size())
                           : cIdx - 1;

    LabNotebook2Entry e;
    e.id = entries_.size() + 1;
    e.experiment = text;
    e.category = categories[catIdx];
    e.notebook = "Notebook " + QString::number(e.id);
    e.progress = 0.1 + QRandomGenerator::global()->bounded(90) / 100.0;
    e.entries = 1 + QRandomGenerator::global()->bounded(15);
    e.pinned = false;
    e.color = colors[catIdx];
    addEntry(e);
    inputField_->clear();
}

void PaperLabNotebook2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperLabNotebook2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Record notebook entries");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Lab Notebook");

    int w = width(), h = height();
    int topH = h / 2 - 30;
    int botH = h / 2 - 60;

    drawNotebookView(p, QRect(10, 50, w / 2 - 10, topH));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 20, topH));
    drawStats(p, QRect(10, 50 + topH + 10, w - 20, botH));
}

void PaperLabNotebook2::drawNotebookView(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(38, (rect.height() - 10) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 4);

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 5, 5);

        // Left color bar
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 5, itemH, 2, 2);

        // Pin indicator
        int xOff = rect.x() + 12;
        if (e.pinned) {
            p.setPen(e.color);
            p.setFont(QFont("Arial", 9, QFont::Bold));
            p.drawText(xOff, y, 20, itemH, Qt::AlignVCenter, "★");
            xOff += 18;
        }

        // Experiment name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(xOff, y + 2, rect.width() / 2 - xOff, itemH / 2,
                   Qt::AlignVCenter, e.experiment.left(16));

        // Category + notebook info
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(xOff, y + itemH / 2, rect.width() / 2 - xOff, itemH / 2,
                   Qt::AlignVCenter, e.category + " | " + e.notebook);

        // Progress bar
        int barX = rect.x() + rect.width() / 2 + 10;
        int barW = rect.width() / 3;
        int barY = y + 5;
        int barH = 8;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        p.drawRoundedRect(barX, barY, barW, barH, 3, 3);
        p.setBrush(e.color);
        p.drawRoundedRect(barX, barY, static_cast<int>(barW * e.progress), barH, 3, 3);

        // Progress text + entry count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(barX, barY + barH + 2, barW, 14, Qt::AlignVCenter,
                   QString::number(e.progress * 100, 'f', 0) + "% | " +
                   QString::number(e.entries) + " entries");
    }
}

void PaperLabNotebook2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList categories = {"Experiment", "Analysis", "Protocol", "Observation", "Result"};
    QColor colors[] = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };

    // Draw donut chart
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 30 + (rect.height() - 50) / 2;
    int outerR = qMin(rect.width(), rect.height() - 50) / 2 - 20;
    int innerR = outerR * 55 / 100;

    qreal total = 0;
    for (int i = 0; i < 5; ++i)
        total += counts.contains(categories[i]) ? counts[categories[i]] : 0;

    if (total > 0) {
        qreal startAngle = 0.0;
        for (int i = 0; i < 5; ++i) {
            int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
            if (count == 0) continue;
            qreal span = (count / total) * 360.0;

            p.setPen(Qt::NoPen);
            p.setBrush(colors[i]);
            p.drawPie(cx - outerR, cy - outerR, outerR * 2, outerR * 2,
                      static_cast<int>(startAngle * 16),
                      static_cast<int>(span * 16));
            startAngle += span;
        }
        // Inner circle (donut hole)
        p.setBrush(Qt::white);
        p.drawEllipse(cx - innerR, cy - innerR, innerR * 2, innerR * 2);

        // Center text
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 12, QFont::Bold));
        p.drawText(cx - innerR, cy - 10, innerR * 2, 20,
                   Qt::AlignCenter, QString::number(static_cast<int>(total)));
        p.setFont(QFont("Arial", 7));
        p.setPen(QColor(100, 116, 139));
        p.drawText(cx - innerR, cy + 6, innerR * 2, 14,
                   Qt::AlignCenter, "total");
    }

    // Legend below chart
    int legendY = cy + outerR + 8;
    int legendW = rect.width() / 5;
    p.setFont(QFont("Arial", 7));
    for (int i = 0; i < 5; ++i) {
        int lx = rect.x() + i * legendW;
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(lx, legendY, 8, 8, 2, 2);
        p.setPen(QColor(100, 116, 139));
        p.drawText(lx + 10, legendY + 8, categories[i].left(5));
    }
}

void PaperLabNotebook2::drawStats(QPainter& p, const QRect& rect) {
    int totalEntries = 0;
    for (const auto& e : entries_)
        totalEntries += e.entries;

    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total",       QString::number(entries_.size()),            QColor("#3b82f6")},
        {"Pinned",      QString::number(pinnedCount()),              QColor("#16a34a")},
        {"Avg Progress",QString::number(avgProgress() * 100, 'f', 0) + "%", QColor("#d97706")},
        {"Total Entries",QString::number(totalEntries),              QColor("#7c3aed")}
    };

    int cols = 2;
    int rows = 2;
    int gap = 8;
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
        p.setFont(QFont("Arial", 16, QFont::Bold));
        p.drawText(bx + 10, by + 5, boxW - 20, boxH / 2, Qt::AlignVCenter,
                   stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(bx + 10, by + boxH / 2, boxW - 20, boxH / 2, Qt::AlignVCenter,
                   stats[i].label);
    }
}

void PaperLabNotebook2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Record notebook entries");
        return;
    }
    infoLabel_->setText(
        QString("%1 notebooks | %2 pinned | %3% avg progress")
            .arg(entries_.size())
            .arg(pinnedCount())
            .arg(avgProgress() * 100, 0, 'f', 0));
}

void PaperLabNotebook2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        LabNotebook2Entry e;
        e.id = settings_.value("id").toInt();
        e.experiment = settings_.value("experiment").toString();
        e.category = settings_.value("category").toString();
        e.notebook = settings_.value("notebook").toString();
        e.progress = settings_.value("progress").toDouble();
        e.entries = settings_.value("entries").toInt();
        e.pinned = settings_.value("pinned").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperLabNotebook2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("experiment", entries_[i].experiment);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("notebook", entries_[i].notebook);
        settings_.setValue("progress", entries_[i].progress);
        settings_.setValue("entries", entries_[i].entries);
        settings_.setValue("pinned", entries_[i].pinned);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
