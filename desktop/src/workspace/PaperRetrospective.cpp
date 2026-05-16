#include "workspace/PaperRetrospective.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperRetrospective::PaperRetrospective(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "Retrospective")
{
    setupUI();
    loadSettings();
}

void PaperRetrospective::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperRetrospective::onAdd);
    toolbar->addWidget(addBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Process", "People", "Tools", "Quality", "Timeline"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperRetrospective::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter retrospective topic...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Add retrospective entries");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperRetrospective::addEntry(const RetroEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit retroAdded(entry.id, entry.impact);
    update();
}

QList<RetroEntry> PaperRetrospective::entries() const { return entries_; }

int PaperRetrospective::completedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.completed) c++;
    return c;
}

qreal PaperRetrospective::avgImpact() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.impact;
    return sum / entries_.size();
}

QMap<QString, int> PaperRetrospective::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperRetrospective::onAdd() {
    QString topic = inputField_->text().trimmed();
    if (topic.isEmpty()) return;
    QStringList categories = {"process", "people", "tools", "quality", "timeline"};
    QStringList actions = {"improve", "continue", "start", "stop", "investigate"};
    QStringList owners = {"Team Lead", "Dev A", "Dev B", "QA Lead", "PM"};
    QColor palette[] = {QColor(59, 130, 246), QColor(22, 163, 74), QColor(217, 119, 6), QColor(220, 38, 38), QColor(124, 58, 237)};
    int cIdx = categoryCombo_->currentIndex();
    int count = 2 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        RetroEntry e;
        e.id = entries_.size() + 1;
        e.topic = topic.left(10) + " item" + QString::number(i);
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.action = actions[QRandomGenerator::global()->bounded(actions.size())];
        e.impact = 0.2 + QRandomGenerator::global()->bounded(80) / 100.0;
        e.effort = 0.1 + QRandomGenerator::global()->bounded(90) / 100.0;
        e.owner = owners[QRandomGenerator::global()->bounded(owners.size())];
        e.completed = e.effort > 0.8;
        e.color = palette[QRandomGenerator::global()->bounded(5)];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperRetrospective::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Add retrospective entries");
    update();
}

void PaperRetrospective::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Add retrospective entries");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Retrospective Board");
    int w = width(), h = height();
    drawRetroList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperRetrospective::drawRetroList(QPainter& p, const QRect& rect) {
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
                   e.topic.left(14) + (e.completed ? " [DONE]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.action + " | " + e.owner + " | " + e.category);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "imp:" + QString::number(e.impact * 100, 'f', 0) + "%");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "eff:" + QString::number(e.effort * 100, 'f', 0) + "%");
    }
}

void PaperRetrospective::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"process", "people", "tools", "quality", "timeline"};
    QString labels[] = {"Process", "People", "Tools", "Quality", "Timeline"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(22, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 120));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 70, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 75, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 78 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperRetrospective::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries", QString::number(entries_.size()), QColor(59,130,246)},
        {"Completed", QString::number(completedCount()), QColor(22,163,74)},
        {"Avg Impact", QString::number(avgImpact() * 100, 'f', 0) + "%", QColor(217,119,6)},
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

void PaperRetrospective::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Add retrospective entries"); return; }
    infoLabel_->setText(QString("%1 items | %2 done | %3% impact")
        .arg(entries_.size()).arg(completedCount()).arg(avgImpact() * 100, 0, 'f', 0));
}

void PaperRetrospective::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        RetroEntry e;
        e.id = settings_.value("id").toInt();
        e.topic = settings_.value("topic").toString();
        e.category = settings_.value("category").toString();
        e.action = settings_.value("action").toString();
        e.impact = settings_.value("impact").toDouble();
        e.effort = settings_.value("effort").toDouble();
        e.owner = settings_.value("owner").toString();
        e.completed = settings_.value("completed").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperRetrospective::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("topic", entries_[i].topic);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("action", entries_[i].action);
        settings_.setValue("impact", entries_[i].impact);
        settings_.setValue("effort", entries_[i].effort);
        settings_.setValue("owner", entries_[i].owner);
        settings_.setValue("completed", entries_[i].completed);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
