#include "workspace/PaperStudyGroup.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperStudyGroup::PaperStudyGroup(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "StudyGroup")
{
    setupUI();
    loadSettings();
}

void PaperStudyGroup::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    joinBtn_ = new QPushButton("Join");
    joinBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(joinBtn_, &QPushButton::clicked, this, &PaperStudyGroup::onJoin);
    toolbar->addWidget(joinBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Reading", "Writing", "Research", "Discussion"});
    toolbar->addWidget(categoryCombo_, 1);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter group name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperStudyGroup::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    infoLabel_ = new QLabel("Join study groups");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperStudyGroup::addEntry(const StudyGroupEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit groupJoined(entry.id, entry.activity);
    update();
}

QList<StudyGroupEntry> PaperStudyGroup::entries() const { return entries_; }

int PaperStudyGroup::activeCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.active) c++;
    return c;
}

qreal PaperStudyGroup::avgActivity() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.activity;
    return sum / entries_.size();
}

QMap<QString, int> PaperStudyGroup::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperStudyGroup::onJoin() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"reading", "writing", "research", "discussion"};
    QStringList roles = {"leader", "member", "reviewer", "contributor"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(3);
    for (int i = 0; i < count; ++i) {
        StudyGroupEntry e;
        e.id = entries_.size() + 1;
        e.name = text.left(8) + " grp" + QString::number(i);
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.role = roles[QRandomGenerator::global()->bounded(roles.size())];
        e.activity = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
        e.members = 2 + QRandomGenerator::global()->bounded(15);
        e.active = QRandomGenerator::global()->bounded(2) == 0;
        e.color = colors[QRandomGenerator::global()->bounded(5)];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperStudyGroup::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Join study groups");
    update();
}

void PaperStudyGroup::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Join study groups");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Study Groups");
    int w = width(), h = height();
    drawGroupList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperStudyGroup::drawGroupList(QPainter& p, const QRect& rect) {
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
                   e.name.left(14) + (e.active ? " [ON]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.role + " | " + QString::number(e.members) + " mem");
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.activity * 100, 'f', 0) + "%");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.category);
    }
}

void PaperStudyGroup::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"reading", "writing", "research", "discussion"};
    QString labels[] = {"Reading", "Writing", "Research", "Discuss"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(124,58,237)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 65, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 70, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 73 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperStudyGroup::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Groups", QString::number(entries_.size()), QColor(59,130,246)},
        {"Active", QString::number(activeCount()), QColor(22,163,74)},
        {"Avg Activity", QString::number(avgActivity() * 100, 'f', 0) + "%", QColor(217,119,6)},
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

void PaperStudyGroup::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Join study groups"); return; }
    infoLabel_->setText(QString("%1 groups | %2 active | %3% activity")
        .arg(entries_.size()).arg(activeCount()).arg(avgActivity() * 100, 0, 'f', 0));
}

void PaperStudyGroup::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        StudyGroupEntry e;
        e.id = settings_.value("id").toInt();
        e.name = settings_.value("name").toString();
        e.category = settings_.value("category").toString();
        e.role = settings_.value("role").toString();
        e.activity = settings_.value("activity").toDouble();
        e.members = settings_.value("members").toInt();
        e.active = settings_.value("active").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperStudyGroup::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("name", entries_[i].name);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("role", entries_[i].role);
        settings_.setValue("activity", entries_[i].activity);
        settings_.setValue("members", entries_[i].members);
        settings_.setValue("active", entries_[i].active);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
