#include "workspace/PaperTaskDependency.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QPaintEvent>

PaperTaskDependency::PaperTaskDependency(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "TaskDependency")
{
    setupUI();
    loadSettings();
}

void PaperTaskDependency::setupUI() {
    auto* mainLayout = new QHBoxLayout(this);

    // Left panel
    auto* leftPanel = new QVBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Core", "Plugin", "UI", "Network", "Data"});
    leftPanel->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Task name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    leftPanel->addWidget(inputField_);

    analyzeBtn_ = new QPushButton("Analyze");
    analyzeBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(analyzeBtn_, &QPushButton::clicked, this, &PaperTaskDependency::onAnalyze);
    leftPanel->addWidget(analyzeBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperTaskDependency::onClear);
    leftPanel->addWidget(clearBtn_);

    infoLabel_ = new QLabel("Tasks: 0 | Critical: 0 | Avg Priority: 0.00");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    leftPanel->addWidget(infoLabel_);

    leftPanel->addStretch();
    mainLayout->addLayout(leftPanel);

    // Right area reserved for custom painting (paintEvent covers the full widget)
    mainLayout->addStretch(1);

    setMinimumSize(680, 480);
}

void PaperTaskDependency::addEntry(const TaskDependencyEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<TaskDependencyEntry> PaperTaskDependency::entries() const {
    return entries_;
}

int PaperTaskDependency::criticalCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.critical) c++;
    return c;
}

qreal PaperTaskDependency::avgPriority() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.priority;
    return sum / entries_.size();
}

QMap<QString, int> PaperTaskDependency::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperTaskDependency::onAnalyze() {
    QString taskName = inputField_->text().trimmed();
    if (taskName.isEmpty()) return;

    static const QStringList categories = {"Core", "Plugin", "UI", "Network", "Data"};
    static const QColor palette[] = {
        QColor(0x3b, 0x82, 0xf6),
        QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06),
        QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };

    int cIdx = categoryCombo_->currentIndex();
    QString category = (cIdx == 0)
        ? categories[QRandomGenerator::global()->bounded(categories.size())]
        : categories[cIdx - 1];

    qreal priority = QRandomGenerator::global()->bounded(100) / 100.0;
    int depth = QRandomGenerator::global()->bounded(6);
    bool critical = priority > 0.8;

    // Pick a dependency from existing entries if any
    QString dependsOn;
    if (!entries_.isEmpty()) {
        int depIdx = QRandomGenerator::global()->bounded(entries_.size());
        dependsOn = entries_[depIdx].task;
    }

    int colorIdx = categories.indexOf(category);
    if (colorIdx < 0) colorIdx = 0;

    TaskDependencyEntry entry;
    entry.id = entries_.size() + 1;
    entry.task = taskName;
    entry.category = category;
    entry.dependsOn = dependsOn;
    entry.priority = priority;
    entry.depth = depth;
    entry.critical = critical;
    entry.color = palette[colorIdx % 5];

    addEntry(entry);
    emit dependencyResolved(entry.id, entry.priority);
    inputField_->clear();
}

void PaperTaskDependency::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperTaskDependency::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No task dependencies");
        return;
    }

    int w = width();
    int h = height();
    int panelW = 180; // left panel area (widgets)
    int rightX = panelW + 10;
    int rightW = w - rightX - 10;
    int thirdW = rightW / 3;

    drawDependencyGraph(p, QRect(rightX, 10, thirdW, h - 20));
    drawCategoryChart(p, QRect(rightX + thirdW + 5, 10, thirdW, h - 20));
    drawStats(p, QRect(rightX + 2 * thirdW + 10, 10, thirdW - 5, h - 20));
}

void PaperTaskDependency::drawDependencyGraph(QPainter& p, const QRect& rect) {
    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 18, "Task Dependencies");

    int topMargin = 30;
    int maxDepth = 1;
    for (const auto& e : entries_) maxDepth = qMax(maxDepth, e.depth + 1);

    qreal colW = static_cast<qreal>(rect.width()) / maxDepth;
    qreal rowH = qMin(36.0, static_cast<qreal>(rect.height() - topMargin - 10) /
                                 qMax(entries_.size(), 1));

    // Build id-to-position map for drawing dependency lines
    QMap<int, QPointF> nodeCenters;

    int show = qMin(entries_.size(), 15);
    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        qreal x = rect.x() + e.depth * colW + colW / 2 - 50;
        qreal y = rect.y() + topMargin + i * rowH;

        // Node rounded rect
        QRectF nodeRect(x, y, 100, rowH - 4);
        p.setPen(e.critical ? QPen(QColor(0xdc, 0x26, 0x26), 2) :
                              QPen(QColor(0x94, 0xa3, 0xb8), 1));
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(nodeRect, 6, 6);

        // Task name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(nodeRect.adjusted(4, 0, -4, 0), Qt::AlignVCenter | Qt::AlignLeft,
                   e.task.left(12));

        // Critical badge
        if (e.critical) {
            p.setFont(QFont("Arial", 6, QFont::Bold));
            p.setPen(QColor(0xdc, 0x26, 0x26));
            p.drawText(nodeRect.adjusted(4, 0, -4, 0), Qt::AlignBottom | Qt::AlignRight, "CRIT");
        }

        nodeCenters[e.id] = nodeRect.center();
    }

    // Draw dependency lines
    p.setPen(QPen(QColor(0xcb, 0xd5, 0xe1), 1, Qt::DashLine));
    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        if (e.dependsOn.isEmpty()) continue;
        // Find the dependency target by task name
        for (int j = 0; j < show; ++j) {
            if (entries_[j].task == e.dependsOn) {
                QPointF from = nodeCenters.value(e.id);
                QPointF to = nodeCenters.value(entries_[j].id);
                if (!from.isNull() && !to.isNull()) {
                    p.drawLine(from.toPoint(), to.toPoint());
                }
                break;
            }
        }
    }
}

void PaperTaskDependency::drawCategoryChart(QPainter& p, const QRect& rect) {
    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 18, "Categories");

    static const QStringList categories = {"Core", "Plugin", "UI", "Network", "Data"};
    static const QColor colors[] = {
        QColor(0x3b, 0x82, 0xf6),
        QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06),
        QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };

    auto counts = categoryCounts();
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int topMargin = 30;
    int barH = qMin(24, (rect.height() - topMargin - 10) / 5);

    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + topMargin + i * (barH + 6);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) *
                                     (rect.width() - 110));

        // Label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 65, barH,
                   Qt::AlignRight | Qt::AlignVCenter, categories[i]);

        // Bar
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 70, y, barW, barH - 2, 3, 3);

        // Count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 73 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperTaskDependency::drawStats(QPainter& p, const QRect& rect) {
    struct Stat {
        QString label;
        QString value;
        QColor color;
    };

    QList<Stat> stats = {
        {"Total Tasks",  QString::number(entries_.size()),   QColor(0x3b, 0x82, 0xf6)},
        {"Critical",     QString::number(criticalCount()),   QColor(0xdc, 0x26, 0x26)},
        {"Avg Priority", QString::number(avgPriority(), 'f', 2),
                                                         QColor(0xd9, 0x77, 0x06)}
    };

    int boxH = qMin(50, (rect.height() - 20) / stats.size());
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 8);

        // Background
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 16, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 26,
                   Qt::AlignVCenter, stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 30, rect.width() - 20, 16,
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperTaskDependency::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Tasks: 0 | Critical: 0 | Avg Priority: 0.00");
        return;
    }
    infoLabel_->setText(QString("Tasks: %1 | Critical: %2 | Avg Priority: %3")
        .arg(entries_.size())
        .arg(criticalCount())
        .arg(avgPriority(), 0, 'f', 2));
}

void PaperTaskDependency::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        TaskDependencyEntry e;
        e.id        = settings_.value("id").toInt();
        e.task      = settings_.value("task").toString();
        e.category  = settings_.value("category").toString();
        e.dependsOn = settings_.value("dependsOn").toString();
        e.priority  = settings_.value("priority").toDouble();
        e.depth     = settings_.value("depth").toInt();
        e.critical  = settings_.value("critical").toBool();
        e.color     = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperTaskDependency::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",        entries_[i].id);
        settings_.setValue("task",      entries_[i].task);
        settings_.setValue("category",  entries_[i].category);
        settings_.setValue("dependsOn", entries_[i].dependsOn);
        settings_.setValue("priority",  entries_[i].priority);
        settings_.setValue("depth",     entries_[i].depth);
        settings_.setValue("critical",  entries_[i].critical);
        settings_.setValue("color",     entries_[i].color.name());
    }
    settings_.endArray();
}
