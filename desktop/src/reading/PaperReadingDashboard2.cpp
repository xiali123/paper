#include "reading/PaperReadingDashboard2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReadingDashboard2::PaperReadingDashboard2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingDashboard2")
{
    setupUI();
    loadSettings();
}

void PaperReadingDashboard2::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    refreshBtn_ = new QPushButton("Refresh");
    refreshBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(refreshBtn_, &QPushButton::clicked, this, &PaperReadingDashboard2::onRefresh);
    toolbar->addWidget(refreshBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"In Progress", "Completed", "Queued", "On Hold", "Recommended"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingDashboard2::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter paper title...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Reading dashboard 2");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperReadingDashboard2::addEntry(const ReadingDashboard2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit readingUpdated(entry.id, entry.progress);
    update();
}

QList<ReadingDashboard2Entry> PaperReadingDashboard2::entries() const { return entries_; }

int PaperReadingDashboard2::overdueCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.overdue) c++;
    return c;
}

qreal PaperReadingDashboard2::avgProgress() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.progress;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingDashboard2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperReadingDashboard2::onRefresh() {
    QString text = inputField_->text().trimmed();
    QString category = categoryCombo_->currentText();
    QStringList categories = {"In Progress", "Completed", "Queued", "On Hold", "Recommended"};
    QStringList statuses = {"Reading", "Done", "Pending", "Paused", "Suggested"};
    QColor palette[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    int catIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        ReadingDashboard2Entry e;
        e.id = entries_.size() + 1;
        e.title = text.isEmpty() ? (QString("Paper ") + QString::number(e.id)) : (text.left(12) + " #" + QString::number(i));
        int idx = (catIdx + i) % categories.size();
        e.category = categories[idx];
        e.status = statuses[idx];
        e.pages = 10 + QRandomGenerator::global()->bounded(200);
        e.progress = (idx == 1) ? 1.0 : static_cast<qreal>(QRandomGenerator::global()->bounded(101)) / 100.0;
        e.overdue = (idx == 0 && QRandomGenerator::global()->bounded(5) == 0);
        e.color = palette[idx];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperReadingDashboard2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Reading dashboard 2");
    update();
}

void PaperReadingDashboard2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Reading dashboard 2");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Dashboard 2");
    int w = width(), h = height();
    drawDashboardView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReadingDashboard2::drawDashboardView(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(34, (rect.height() - 10) / qMax(show, 1));
    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 3);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.title.left(14) + (e.overdue ? " [!]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + QString::number(e.pages) + "pg");
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.progress * 100, 'f', 0) + "%");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight, e.status);
    }
}

void PaperReadingDashboard2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"In Progress", "Completed", "Queued", "On Hold", "Recommended"};
    QColor palette[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 120));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 75, barH, Qt::AlignRight | Qt::AlignVCenter, categories[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(palette[i]);
        p.drawRoundedRect(rect.x() + 80, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 83 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperReadingDashboard2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Papers", QString::number(entries_.size()), QColor(59,130,246)},
        {"Overdue", QString::number(overdueCount()), QColor(220,38,38)},
        {"Avg Progress", QString::number(avgProgress() * 100, 'f', 0) + "%", QColor(22,163,74)},
        {"Categories", QString::number(categoryCounts().size()), QColor(124,58,237)}
    };
    int boxH = qMin(42, (rect.height() - 10) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 22, Qt::AlignVCenter, stats[i].value);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 26, rect.width() - 20, 14, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperReadingDashboard2::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Reading dashboard 2"); return; }
    infoLabel_->setText(QString("%1 papers | %2 overdue | %3% avg progress")
        .arg(entries_.size()).arg(overdueCount()).arg(avgProgress() * 100, 0, 'f', 0));
}

void PaperReadingDashboard2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ReadingDashboard2Entry e;
        e.id = settings_.value("id").toInt();
        e.title = settings_.value("title").toString();
        e.category = settings_.value("category").toString();
        e.status = settings_.value("status").toString();
        e.progress = settings_.value("progress").toDouble();
        e.pages = settings_.value("pages").toInt();
        e.overdue = settings_.value("overdue").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    if (entries_.isEmpty()) {
        QStringList categories = {"In Progress", "Completed", "Queued", "On Hold", "Recommended"};
        QStringList statuses = {"Reading", "Done", "Pending", "Paused", "Suggested"};
        QColor palette[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
        QString seedTitles[] = {
            "Attention Is All You Need", "BERT Pre-training", "GPT-4 Technical Report",
            "ResNet Deep Learning", "Transformers Survey", "Diffusion Models Overview",
            "Graph Neural Networks", "Federated Learning Review"
        };
        int seedProgress[] = {65, 100, 30, 85, 10, 45, 0, 55};
        int seedPages[] = {12, 24, 68, 15, 42, 38, 22, 50};
        bool seedOverdue[] = {true, false, false, false, false, true, false, false};
        int seedCatIdx[] = {0, 1, 4, 1, 2, 0, 3, 4};
        for (int i = 0; i < 8; ++i) {
            ReadingDashboard2Entry e;
            e.id = i + 1;
            e.title = seedTitles[i];
            e.category = categories[seedCatIdx[i]];
            e.status = statuses[seedCatIdx[i]];
            e.progress = seedProgress[i] / 100.0;
            e.pages = seedPages[i];
            e.overdue = seedOverdue[i];
            e.color = palette[seedCatIdx[i]];
            entries_.append(e);
        }
        saveSettings();
    }
    updateInfo();
}

void PaperReadingDashboard2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("title", entries_[i].title);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("status", entries_[i].status);
        settings_.setValue("progress", entries_[i].progress);
        settings_.setValue("pages", entries_[i].pages);
        settings_.setValue("overdue", entries_[i].overdue);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
