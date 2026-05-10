#include "reading/PaperReadingWorkshop.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReadingWorkshop::PaperReadingWorkshop(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingWorkshop")
{
    setupUI();
    loadSettings();
}

void PaperReadingWorkshop::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    createBtn_ = new QPushButton("Create");
    createBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(createBtn_, &QPushButton::clicked, this, &PaperReadingWorkshop::onCreate);
    toolbar->addWidget(createBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Method", "Theory", "Domain", "Skills"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingWorkshop::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter workshop topic...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Create reading workshops");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperReadingWorkshop::addEntry(const WorkshopEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit workshopCreated(entry.id, entry.rating);
    update();
}

QList<WorkshopEntry> PaperReadingWorkshop::entries() const { return entries_; }

int PaperReadingWorkshop::completedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.completed) c++;
    return c;
}

qreal PaperReadingWorkshop::avgRating() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.rating;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingWorkshop::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperReadingWorkshop::onCreate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"method", "theory", "domain", "skills"};
    QStringList facilitators = {"prof-A", "prof-B", "phd-C", "researcher-D"};
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        WorkshopEntry e;
        e.id = entries_.size() + 1;
        e.topic = text.left(10) + " ws" + QString::number(i);
        e.facilitator = facilitators[QRandomGenerator::global()->bounded(facilitators.size())];
        e.date = "2026-05-" + QString::number(10 + QRandomGenerator::global()->bounded(20));
        e.participants = 5 + QRandomGenerator::global()->bounded(30);
        e.rating = 1 + QRandomGenerator::global()->bounded(40) / 10.0;
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.duration = 30 + QRandomGenerator::global()->bounded(150);
        e.completed = QRandomGenerator::global()->bounded(4) != 0;
        e.color = e.completed ? QColor(16,185,129) : (e.rating >= 3.5 ? QColor(59,130,246) : QColor(245,158,11));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperReadingWorkshop::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Create reading workshops");
    update();
}

void PaperReadingWorkshop::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Create reading workshops");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Workshop");
    int w = width(), h = height();
    drawWorkshopList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReadingWorkshop::drawWorkshopList(QPainter& p, const QRect& rect) {
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
                   e.topic.left(14) + (e.completed ? " [OK]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.facilitator + " | " + QString::number(e.participants) + " ppl | " + e.date);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.rating, 'f', 1) + "/5.0");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.duration) + "min | " + e.category);
    }
}

void PaperReadingWorkshop::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"method", "theory", "domain", "skills"};
    QString labels[] = {"Method", "Theory", "Domain", "Skills"};
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
        p.drawText(rect.x(), y + barH - 2, 30, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 35, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 38 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperReadingWorkshop::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Workshops", QString::number(entries_.size()), QColor(59,130,246)},
        {"Completed", QString::number(completedCount()), QColor(16,185,129)},
        {"Avg Rating", QString::number(avgRating(), 'f', 1) + "/5", QColor(245,158,11)},
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

void PaperReadingWorkshop::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Create reading workshops"); return; }
    infoLabel_->setText(QString("%1 workshops | %2 done | %3/5 avg")
        .arg(entries_.size()).arg(completedCount()).arg(avgRating(), 0, 'f', 1));
}

void PaperReadingWorkshop::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        WorkshopEntry e;
        e.id = settings_.value("id").toInt();
        e.topic = settings_.value("topic").toString();
        e.facilitator = settings_.value("facilitator").toString();
        e.date = settings_.value("date").toString();
        e.participants = settings_.value("participants").toInt();
        e.rating = settings_.value("rating").toDouble();
        e.category = settings_.value("category").toString();
        e.duration = settings_.value("duration").toInt();
        e.completed = settings_.value("completed").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingWorkshop::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("topic", entries_[i].topic);
        settings_.setValue("facilitator", entries_[i].facilitator);
        settings_.setValue("date", entries_[i].date);
        settings_.setValue("participants", entries_[i].participants);
        settings_.setValue("rating", entries_[i].rating);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("duration", entries_[i].duration);
        settings_.setValue("completed", entries_[i].completed);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
