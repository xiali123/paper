#include "workspace/PaperResourceAllocator.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

PaperResourceAllocator::PaperResourceAllocator(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ResourceAllocator")
{
    setupUI();
    loadSettings();
}

void PaperResourceAllocator::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Resource");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperResourceAllocator::onAdd);
    toolbar->addWidget(addBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperResourceAllocator::onClear);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Allocate research resources");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperResourceAllocator::addResource(const ResourceEntry& resource) {
    resources_.append(resource);
    saveSettings();
    updateInfo();
    emit resourceAllocated(resource.id, resource.assignee);
    update();
}

QList<ResourceEntry> PaperResourceAllocator::resources() const { return resources_; }

QMap<QString, int> PaperResourceAllocator::typeCounts() const {
    QMap<QString, int> counts;
    for (const auto& r : resources_) counts[r.type]++;
    return counts;
}

QMap<QString, int> PaperResourceAllocator::statusCounts() const {
    QMap<QString, int> counts;
    for (const auto& r : resources_) counts[r.status]++;
    return counts;
}

int PaperResourceAllocator::availableCount() const {
    int c = 0;
    for (const auto& r : resources_) if (r.status == "available") c++;
    return c;
}

void PaperResourceAllocator::onAdd() {
    bool ok;
    QString name = QInputDialog::getText(this, "Add Resource", "Resource name:", QLineEdit::Normal, "", &ok);
    if (!ok || name.isEmpty()) return;
    QStringList types = {"paper", "dataset", "tool", "compute"};
    QString type = QInputDialog::getItem(this, "Add Resource", "Type:", types, 0, false, &ok);
    if (!ok) return;
    QStringList statuses = {"available", "in-use", "reserved"};
    QString status = QInputDialog::getItem(this, "Add Resource", "Status:", statuses, 0, false, &ok);
    if (!ok) return;
    QString assignee = QInputDialog::getText(this, "Add Resource", "Assignee:", QLineEdit::Normal, "", &ok);
    if (!ok) return;

    ResourceEntry r;
    r.id = resources_.size() + 1;
    r.name = name;
    r.type = type;
    r.status = status;
    r.assignee = assignee.isEmpty() ? "Unassigned" : assignee;
    r.deadline = QDate::currentDate().addDays(QRandomGenerator::global()->bounded(30));
    r.priority = 1 + QRandomGenerator::global()->bounded(5);

    QColor statusColors[] = {QColor(16,185,129), QColor(245,158,11), QColor(59,130,246)};
    int sIdx = statuses.indexOf(status);
    r.color = statusColors[qBound(0, sIdx, 2)];
    addResource(r);
}

void PaperResourceAllocator::onClear() {
    resources_.clear();
    saveSettings();
    infoLabel_->setText("Allocate research resources");
    update();
}

void PaperResourceAllocator::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (resources_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Allocate research resources");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Resource Allocator");

    int w = width(), h = height();
    drawResourceCards(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawStatusChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperResourceAllocator::drawResourceCards(QPainter& p, const QRect& rect) {
    int show = qMin(8, resources_.size());
    int cardH = qMin(48, (rect.height() - 10) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& r = resources_[i];
        int y = rect.y() + i * (cardH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(r.color.lighter(180));
        p.drawRoundedRect(rect.x(), y, rect.width(), cardH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(r.color);
        p.drawRoundedRect(rect.x(), y, 4, cardH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   r.name.left(18));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   r.type + " | P" + QString::number(r.priority));

        p.drawText(rect.x() + 10, y + 34, rect.width() / 2 - 10, 12, Qt::AlignVCenter,
                   r.assignee);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight, r.status);
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight, "Due: " + r.deadline.toString("MM/dd"));
    }
}

void PaperResourceAllocator::drawStatusChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Status Distribution");

    auto counts = statusCounts();
    QStringList statuses = {"available", "in-use", "reserved"};
    QString labels[] = {"Available", "In Use", "Reserved"};
    QColor colors[] = {QColor(16,185,129), QColor(245,158,11), QColor(59,130,246)};
    int total = resources_.size();
    if (total == 0) return;

    int pieW = qMin(rect.width(), rect.height() - 40);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 20 + pieW / 2;

    qreal startAngle = 0;
    for (int i = 0; i < 3; ++i) {
        int count = counts.contains(statuses[i]) ? counts[statuses[i]] : 0;
        qreal span = total > 0 ? (static_cast<qreal>(count) / total) * 360 : 0;
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawPie(cx - pieW / 2, cy - pieW / 2, pieW, pieW,
                  static_cast<int>(startAngle * 16), static_cast<int>(span * 16));
        startAngle += span;
    }

    p.setBrush(Qt::white);
    p.drawEllipse(cx - pieW / 4, cy - pieW / 4, pieW / 2, pieW / 2);

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(cx - 10, cy + 5, QString::number(total));
}

void PaperResourceAllocator::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Resources", QString::number(resources_.size()), QColor(59,130,246)},
        {"Available", QString::number(availableCount()), QColor(16,185,129)},
        {"Types", QString::number(typeCounts().size()), QColor(245,158,11)},
        {"In Use", QString::number(statusCounts().value("in-use", 0)), QColor(139,92,246)}
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

void PaperResourceAllocator::updateInfo() {
    if (resources_.isEmpty()) { infoLabel_->setText("Allocate research resources"); return; }
    infoLabel_->setText(QString("%1 resources | %2 available | %3 types")
        .arg(resources_.size()).arg(availableCount()).arg(typeCounts().size()));
}

void PaperResourceAllocator::loadSettings() {
    int size = settings_.beginReadArray("resources");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ResourceEntry r;
        r.id = settings_.value("id").toInt();
        r.name = settings_.value("name").toString();
        r.type = settings_.value("type").toString();
        r.status = settings_.value("status").toString();
        r.assignee = settings_.value("assignee").toString();
        r.deadline = QDate::fromString(settings_.value("deadline").toString(), Qt::ISODate);
        r.priority = settings_.value("priority").toInt();
        r.notes = settings_.value("notes").toString();
        r.color = QColor(settings_.value("color").toString());
        resources_.append(r);
    }
    settings_.endArray();
    updateInfo();
}

void PaperResourceAllocator::saveSettings() {
    settings_.beginWriteArray("resources");
    for (int i = 0; i < resources_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", resources_[i].id);
        settings_.setValue("name", resources_[i].name);
        settings_.setValue("type", resources_[i].type);
        settings_.setValue("status", resources_[i].status);
        settings_.setValue("assignee", resources_[i].assignee);
        settings_.setValue("deadline", resources_[i].deadline.toString(Qt::ISODate));
        settings_.setValue("priority", resources_[i].priority);
        settings_.setValue("notes", resources_[i].notes);
        settings_.setValue("color", resources_[i].color.name());
    }
    settings_.endArray();
}
