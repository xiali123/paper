#include "tools/PaperAlertMonitor.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

PaperAlertMonitor::PaperAlertMonitor(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "AlertMonitor")
{
    setupUI();
    loadSettings();
}

void PaperAlertMonitor::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Unread", "Critical", "Warning", "Info"});
    connect(filterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperAlertMonitor::onFilterChanged);
    toolbar->addWidget(filterCombo_, 1);

    addBtn_ = new QPushButton("Add");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperAlertMonitor::onAdd);
    toolbar->addWidget(addBtn_);

    markReadBtn_ = new QPushButton("Mark Read");
    connect(markReadBtn_, &QPushButton::clicked, this, &PaperAlertMonitor::onMarkRead);
    toolbar->addWidget(markReadBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperAlertMonitor::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Monitor paper alerts");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 460);
}

void PaperAlertMonitor::addAlert(const AlertEntry& alert) {
    alerts_.append(alert);
    saveSettings();
    updateInfo();
    emit unreadChanged(unreadCount());
    emit alertTriggered(alert.id, alert.severity);
    update();
}

QList<AlertEntry> PaperAlertMonitor::alerts() const { return alerts_; }

int PaperAlertMonitor::unreadCount() const {
    int c = 0;
    for (const auto& a : alerts_) if (!a.read) c++;
    return c;
}

QMap<QString, int> PaperAlertMonitor::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& a : alerts_) counts[a.category]++;
    return counts;
}

QMap<QString, int> PaperAlertMonitor::severityCounts() const {
    QMap<QString, int> counts;
    for (const auto& a : alerts_) counts[a.severity]++;
    return counts;
}

void PaperAlertMonitor::onAdd() {
    bool ok;
    QString title = QInputDialog::getText(this, "Add Alert", "Title:", QLineEdit::Normal, "", &ok);
    if (!ok || title.isEmpty()) return;
    QStringList cats = {"citation", "publication", "keyword", "author", "trend"};
    QString cat = QInputDialog::getItem(this, "Add Alert", "Category:", cats, 0, false, &ok);
    if (!ok) return;
    QStringList sevs = {"info", "warning", "critical"};
    QString sev = QInputDialog::getItem(this, "Add Alert", "Severity:", sevs, 1, false, &ok);
    if (!ok) return;

    AlertEntry a;
    a.id = alerts_.size() + 1;
    a.title = title;
    a.category = cat;
    a.severity = sev;
    a.date = QDate::currentDate();
    a.description = title;

    QColor sevColors[] = {QColor(59,130,246), QColor(245,158,11), QColor(239,68,68)};
    int sIdx = sevs.indexOf(sev);
    a.color = sevColors[qBound(0, sIdx, 2)];
    addAlert(a);
}

void PaperAlertMonitor::onMarkRead() {
    for (auto& a : alerts_) a.read = true;
    saveSettings();
    updateInfo();
    emit unreadChanged(0);
    update();
}

void PaperAlertMonitor::onFilterChanged(int) { update(); }
void PaperAlertMonitor::onClear() {
    alerts_.clear();
    saveSettings();
    infoLabel_->setText("Monitor paper alerts");
    update();
}

void PaperAlertMonitor::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (alerts_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Monitor paper alerts");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Alert Monitor");

    int w = width(), h = height();
    drawAlertList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawSeverityChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperAlertMonitor::drawAlertList(QPainter& p, const QRect& rect) {
    int filterIdx = filterCombo_->currentIndex();
    int show = 0;
    int maxShow = 10;
    int itemH = qMin(36, (rect.height() - 10) / maxShow);

    for (int i = alerts_.size() - 1; i >= 0 && show < maxShow; --i) {
        const auto& a = alerts_[i];

        if (filterIdx == 1 && a.read) continue;
        if (filterIdx == 2 && a.severity != "critical") continue;
        if (filterIdx == 3 && a.severity != "warning") continue;
        if (filterIdx == 4 && a.severity != "info") continue;

        int y = rect.y() + show * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(a.read ? Qt::white : a.color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(a.color);
        p.drawEllipse(rect.x() + 6, y + itemH / 2 - 4, 8, 8);

        if (!a.read) {
            p.setBrush(a.color);
            p.drawEllipse(rect.x() + rect.width() - 14, y + 6, 6, 6);
        }

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 20, y + 2, rect.width() - 50, 16, Qt::AlignVCenter,
                   a.title.left(24));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 20, y + 18, rect.width() - 50, 14, Qt::AlignVCenter,
                   a.category + " | " + a.severity);

        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + rect.width() - 50, y + 2, 46, 14, Qt::AlignVCenter | Qt::AlignRight,
                   a.date.toString("MM/dd"));
        show++;
    }
}

void PaperAlertMonitor::drawSeverityChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "By Severity");

    auto counts = severityCounts();
    QStringList sevs = {"info", "warning", "critical"};
    QString labels[] = {"Info", "Warning", "Critical"};
    QColor colors[] = {QColor(59,130,246), QColor(245,158,11), QColor(239,68,68)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(28, (rect.height() - 30) / 3);
    for (int i = 0; i < 3; ++i) {
        int y = rect.y() + 22 + i * (barH + 4);
        int count = counts.contains(sevs[i]) ? counts[sevs[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x(), y + barH - 3, 60, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 65, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 68 + barW, y + barH - 3, QString::number(count));
    }
}

void PaperAlertMonitor::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total", QString::number(alerts_.size()), QColor(59,130,246)},
        {"Unread", QString::number(unreadCount()), QColor(239,68,68)},
        {"Critical", QString::number(severityCounts().value("critical", 0)), QColor(245,158,11)},
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

void PaperAlertMonitor::updateInfo() {
    if (alerts_.isEmpty()) { infoLabel_->setText("Monitor paper alerts"); return; }
    infoLabel_->setText(QString("%1 alerts | %2 unread | %3 critical")
        .arg(alerts_.size()).arg(unreadCount())
        .arg(severityCounts().value("critical", 0)));
}

void PaperAlertMonitor::loadSettings() {
    int size = settings_.beginReadArray("alerts");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        AlertEntry a;
        a.id = settings_.value("id").toInt();
        a.title = settings_.value("title").toString();
        a.category = settings_.value("category").toString();
        a.severity = settings_.value("severity").toString();
        a.date = QDate::fromString(settings_.value("date").toString(), Qt::ISODate);
        a.description = settings_.value("description").toString();
        a.read = settings_.value("read").toBool();
        a.color = QColor(settings_.value("color").toString());
        alerts_.append(a);
    }
    settings_.endArray();
    updateInfo();
}

void PaperAlertMonitor::saveSettings() {
    settings_.beginWriteArray("alerts");
    for (int i = 0; i < alerts_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", alerts_[i].id);
        settings_.setValue("title", alerts_[i].title);
        settings_.setValue("category", alerts_[i].category);
        settings_.setValue("severity", alerts_[i].severity);
        settings_.setValue("date", alerts_[i].date.toString(Qt::ISODate));
        settings_.setValue("description", alerts_[i].description);
        settings_.setValue("read", alerts_[i].read);
        settings_.setValue("color", alerts_[i].color.name());
    }
    settings_.endArray();
}
