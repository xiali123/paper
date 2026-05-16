#include "reading/PaperReadingProgressGrid.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReadingProgressGrid::PaperReadingProgressGrid(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingProgressGrid")
{
    setupUI();
    loadSettings();
}

void PaperReadingProgressGrid::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    updateBtn_ = new QPushButton("Update");
    updateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(updateBtn_, &QPushButton::clicked, this, &PaperReadingProgressGrid::onUpdate);
    toolbar->addWidget(updateBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "ML", "NLP", "CV", "Theory"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingProgressGrid::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter user name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Reading progress grid");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperReadingProgressGrid::addEntry(const ProgressGridEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit progressUpdated(entry.id, entry.progress);
    update();
}

QList<ProgressGridEntry> PaperReadingProgressGrid::entries() const { return entries_; }

qreal PaperReadingProgressGrid::avgProgress() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.progress;
    return sum / entries_.size();
}

int PaperReadingProgressGrid::onTrackCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.onTrack) c++;
    return c;
}

QMap<QString, int> PaperReadingProgressGrid::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperReadingProgressGrid::onUpdate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"ML", "NLP", "CV", "Theory"};
    QStringList weeks = {"W1", "W2", "W3", "W4"};
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        ProgressGridEntry e;
        e.id = entries_.size() + 1;
        e.userName = text.left(8) + " user" + QString::number(i);
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.papersTotal = 10 + QRandomGenerator::global()->bounded(50);
        e.papersRead = QRandomGenerator::global()->bounded(e.papersTotal + 1);
        e.progress = static_cast<qreal>(e.papersRead) / e.papersTotal;
        e.week = weeks[QRandomGenerator::global()->bounded(weeks.size())];
        e.speed = 0.5 + QRandomGenerator::global()->bounded(40) / 10.0;
        e.onTrack = e.progress >= 0.5;
        e.color = e.onTrack ? QColor(16,185,129) : (e.progress >= 0.25 ? QColor(245,158,11) : QColor(239,68,68));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperReadingProgressGrid::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Reading progress grid");
    update();
}

void PaperReadingProgressGrid::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Reading progress grid");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Progress Grid");
    int w = width(), h = height();
    drawProgressGrid(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReadingProgressGrid::drawProgressGrid(QPainter& p, const QRect& rect) {
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
                   e.userName.left(10) + " [" + e.category + "] " + e.week);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   QString::number(e.papersRead) + "/" + QString::number(e.papersTotal) + " | " + QString::number(e.speed, 'f', 1) + "/day");
        int barW = static_cast<int>(e.progress * (rect.width() / 2 - 20));
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x() + rect.width() / 2, y + 10, barW, 14, 3, 3);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(rect.x() + rect.width() / 2 + barW + 4, y + 22, QString::number(e.progress * 100, 'f', 0) + "%");
    }
}

void PaperReadingProgressGrid::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"ML", "NLP", "CV", "Theory"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 80));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x(), y + barH - 2, 30, barH, Qt::AlignRight | Qt::AlignVCenter, categories[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 35, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 38 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperReadingProgressGrid::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Users", QString::number(entries_.size()), QColor(59,130,246)},
        {"On Track", QString::number(onTrackCount()), QColor(16,185,129)},
        {"Avg Progress", QString::number(avgProgress() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Categories", QString::number(categoryCounts().size()), QColor(139,92,246)}
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

void PaperReadingProgressGrid::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Reading progress grid"); return; }
    infoLabel_->setText(QString("%1 users | %2 on track | %3% avg")
        .arg(entries_.size()).arg(onTrackCount()).arg(avgProgress() * 100, 0, 'f', 0));
}

void PaperReadingProgressGrid::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ProgressGridEntry e;
        e.id = settings_.value("id").toInt();
        e.userName = settings_.value("userName").toString();
        e.progress = settings_.value("progress").toDouble();
        e.category = settings_.value("category").toString();
        e.papersRead = settings_.value("papersRead").toInt();
        e.papersTotal = settings_.value("papersTotal").toInt();
        e.week = settings_.value("week").toString();
        e.speed = settings_.value("speed").toDouble();
        e.onTrack = settings_.value("onTrack").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingProgressGrid::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("userName", entries_[i].userName);
        settings_.setValue("progress", entries_[i].progress);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("papersRead", entries_[i].papersRead);
        settings_.setValue("papersTotal", entries_[i].papersTotal);
        settings_.setValue("week", entries_[i].week);
        settings_.setValue("speed", entries_[i].speed);
        settings_.setValue("onTrack", entries_[i].onTrack);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
