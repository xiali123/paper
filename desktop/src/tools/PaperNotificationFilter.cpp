#include "tools/PaperNotificationFilter.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperNotificationFilter::PaperNotificationFilter(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "NotificationFilter")
{
    setupUI();
    loadSettings();
}

void PaperNotificationFilter::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperNotificationFilter::onAdd);
    toolbar->addWidget(addBtn_);
    toolbar->addWidget(new QLabel("Channel:"));
    channelCombo_ = new QComboBox();
    channelCombo_->addItems({"All", "Email", "Slack", "In-App", "Webhook"});
    toolbar->addWidget(channelCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperNotificationFilter::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter rule name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Configure notification filters");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperNotificationFilter::addEntry(const NotificationRule& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit filterCreated(entry.id, entry.ruleName);
    update();
}

QList<NotificationRule> PaperNotificationFilter::entries() const { return entries_; }

int PaperNotificationFilter::activeCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.active) c++;
    return c;
}

int PaperNotificationFilter::regexCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.regex) c++;
    return c;
}

QMap<QString, int> PaperNotificationFilter::channelCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.channel]++;
    return counts;
}

void PaperNotificationFilter::onAdd() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList channels = {"email", "slack", "in-app", "webhook"};
    QStringList priorities = {"critical", "high", "medium", "low"};
    QStringList actions = {"show", "silence", "digest", "forward"};
    QStringList keywords = {"error", "warning", "update", "review", "deadline", "mention"};
    int cIdx = channelCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(5);
    for (int i = 0; i < count; ++i) {
        NotificationRule e;
        e.id = entries_.size() + 1;
        e.ruleName = text.left(8) + " rule" + QString::number(i);
        e.channel = cIdx == 0 ? channels[QRandomGenerator::global()->bounded(channels.size())] : channels[cIdx - 1];
        e.priority = priorities[QRandomGenerator::global()->bounded(priorities.size())];
        e.keyword = keywords[QRandomGenerator::global()->bounded(keywords.size())];
        e.action = actions[QRandomGenerator::global()->bounded(actions.size())];
        e.active = QRandomGenerator::global()->bounded(5) != 0;
        e.regex = QRandomGenerator::global()->bounded(4) == 0;
        e.color = e.active ? (e.priority == "critical" ? QColor(239,68,68) : QColor(59,130,246)) : QColor(156,163,175);
        addEntry(e);
    }
    inputField_->clear();
}

void PaperNotificationFilter::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Configure notification filters");
    update();
}

void PaperNotificationFilter::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Configure notification filters");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Notification Filter");
    int w = width(), h = height();
    drawRuleList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawChannelChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperNotificationFilter::drawRuleList(QPainter& p, const QRect& rect) {
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
                   e.ruleName.left(14) + (e.regex ? " [RE]" : "") + (e.active ? "" : " [OFF]"));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.priority + " | " + e.keyword + " | " + e.action);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.channel);
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.active ? "active" : "disabled");
    }
}

void PaperNotificationFilter::drawChannelChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Channels");
    auto counts = channelCounts();
    QStringList channels = {"email", "slack", "in-app", "webhook"};
    QString labels[] = {"Email", "Slack", "In-App", "Webhook"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(channels[i]) ? counts[channels[i]] : 0;
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

void PaperNotificationFilter::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Rules", QString::number(entries_.size()), QColor(59,130,246)},
        {"Active", QString::number(activeCount()), QColor(16,185,129)},
        {"Regex", QString::number(regexCount()), QColor(245,158,11)},
        {"Channels", QString::number(channelCounts().size()), QColor(139,92,246)}
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

void PaperNotificationFilter::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Configure notification filters"); return; }
    infoLabel_->setText(QString("%1 rules | %2 active | %3 regex")
        .arg(entries_.size()).arg(activeCount()).arg(regexCount()));
}

void PaperNotificationFilter::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        NotificationRule e;
        e.id = settings_.value("id").toInt();
        e.ruleName = settings_.value("ruleName").toString();
        e.channel = settings_.value("channel").toString();
        e.priority = settings_.value("priority").toString();
        e.keyword = settings_.value("keyword").toString();
        e.action = settings_.value("action").toString();
        e.active = settings_.value("active").toBool();
        e.regex = settings_.value("regex").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperNotificationFilter::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("ruleName", entries_[i].ruleName);
        settings_.setValue("channel", entries_[i].channel);
        settings_.setValue("priority", entries_[i].priority);
        settings_.setValue("keyword", entries_[i].keyword);
        settings_.setValue("action", entries_[i].action);
        settings_.setValue("active", entries_[i].active);
        settings_.setValue("regex", entries_[i].regex);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
