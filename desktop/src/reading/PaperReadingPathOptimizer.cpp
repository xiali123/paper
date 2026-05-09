#include "reading/PaperReadingPathOptimizer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>
#include <algorithm>

PaperReadingPathOptimizer::PaperReadingPathOptimizer(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingPathOptimizer")
{
    setupUI();
    loadSettings();
}

void PaperReadingPathOptimizer::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Paper");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperReadingPathOptimizer::onAdd);
    toolbar->addWidget(addBtn_);

    optimizeBtn_ = new QPushButton("Optimize");
    optimizeBtn_->setStyleSheet("QPushButton { background: #16a34a; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(optimizeBtn_, &QPushButton::clicked, this, &PaperReadingPathOptimizer::onOptimize);
    toolbar->addWidget(optimizeBtn_);

    toolbar->addWidget(new QLabel("Sort:"));
    sortCombo_ = new QComboBox();
    sortCombo_->addItems({"Order", "Difficulty", "Time"});
    toolbar->addWidget(sortCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingPathOptimizer::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Optimize reading path");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperReadingPathOptimizer::addPath(const PathEntry& entry) {
    paths_.append(entry);
    saveSettings();
    updateInfo();
    emit pathOptimized(paths_.size());
    update();
}

QList<PathEntry> PaperReadingPathOptimizer::paths() const { return paths_; }

QMap<QString, int> PaperReadingPathOptimizer::statusCounts() const {
    QMap<QString, int> counts;
    for (const auto& p : paths_) counts[p.status]++;
    return counts;
}

qreal PaperReadingPathOptimizer::totalEstimatedTime() const {
    qreal t = 0;
    for (const auto& p : paths_) t += p.estimatedTime;
    return t;
}

qreal PaperReadingPathOptimizer::avgDifficulty() const {
    if (paths_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& p : paths_) sum += p.difficulty;
    return sum / paths_.size();
}

void PaperReadingPathOptimizer::onAdd() {
    bool ok;
    QString title = QInputDialog::getText(this, "Add Paper", "Paper Title:", QLineEdit::Normal, "", &ok);
    if (!ok || title.isEmpty()) return;
    QString topic = QInputDialog::getText(this, "Add Paper", "Topic:", QLineEdit::Normal, "", &ok);
    if (!ok) return;

    PathEntry e;
    e.id = paths_.size() + 1;
    e.paperTitle = title;
    e.topic = topic.isEmpty() ? "General" : topic;
    e.order = paths_.size() + 1;
    e.difficulty = 1 + QRandomGenerator::global()->bounded(50) / 10.0;
    e.estimatedTime = 10 + QRandomGenerator::global()->bounded(120);
    e.prerequisite = paths_.isEmpty() ? "None" : paths_.last().paperTitle.left(12);
    e.status = "pending";

    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68), QColor(139,92,246)};
    e.color = colors[paths_.size() % 5];
    addPath(e);
}

void PaperReadingPathOptimizer::onOptimize() {
    if (paths_.size() < 2) return;
    std::sort(paths_.begin(), paths_.end(), [](const PathEntry& a, const PathEntry& b) {
        return a.difficulty < b.difficulty;
    });
    for (int i = 0; i < paths_.size(); ++i) {
        paths_[i].order = i + 1;
        paths_[i].status = i == 0 ? "ready" : "pending";
        paths_[i].prerequisite = i > 0 ? paths_[i - 1].paperTitle.left(12) : "None";
    }
    saveSettings();
    updateInfo();
    emit pathOptimized(paths_.size());
    update();
}

void PaperReadingPathOptimizer::onClear() {
    paths_.clear();
    saveSettings();
    infoLabel_->setText("Optimize reading path");
    update();
}

void PaperReadingPathOptimizer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (paths_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Optimize reading path");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Path Optimizer");

    int w = width(), h = height();
    drawPathTimeline(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawDifficultyChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReadingPathOptimizer::drawPathTimeline(QPainter& p, const QRect& rect) {
    int show = qMin(10, paths_.size());
    int itemH = qMin(40, (rect.height() - 10) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& e = paths_[i];
        int y = rect.y() + i * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        if (i < show - 1) {
            p.setPen(QPen(QColor(203, 213, 225), 2, Qt::DashLine));
            p.drawLine(rect.x() + 2, y + itemH, rect.x() + 2, y + itemH + 3);
        }

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   "#" + QString::number(e.order) + " " + e.paperTitle.left(16));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.topic.left(14) + " | " + e.status);
        p.drawText(rect.x() + 10, y + 32, rect.width() / 2 - 10, 10, Qt::AlignVCenter,
                   "pre: " + e.prerequisite);

        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.difficulty, 'f', 1) + " diff");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(static_cast<int>(e.estimatedTime)) + " min");
    }
}

void PaperReadingPathOptimizer::drawDifficultyChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Difficulty");

    int show = qMin(8, paths_.size());
    int barH = qMin(20, (rect.height() - 30) / qMax(show, 1));
    qreal maxDiff = 1;
    for (const auto& e : paths_) maxDiff = qMax(maxDiff, e.difficulty);

    for (int i = 0; i < show; ++i) {
        const auto& e = paths_[i];
        int y = rect.y() + 22 + i * (barH + 2);
        int barW = static_cast<int>((e.difficulty / maxDiff) * (rect.width() - 100));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 50, barH, Qt::AlignRight | Qt::AlignVCenter,
                   "#" + QString::number(e.order));

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x() + 55, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 58 + barW, y + barH - 2, QString::number(e.difficulty, 'f', 1));
    }
}

void PaperReadingPathOptimizer::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Papers", QString::number(paths_.size()), QColor(59,130,246)},
        {"Total Time", QString::number(static_cast<int>(totalEstimatedTime())) + " min", QColor(16,185,129)},
        {"Avg Difficulty", QString::number(avgDifficulty(), 'f', 1), QColor(245,158,11)},
        {"Ready", QString::number(statusCounts().value("ready", 0)), QColor(139,92,246)}
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

void PaperReadingPathOptimizer::updateInfo() {
    if (paths_.isEmpty()) { infoLabel_->setText("Optimize reading path"); return; }
    infoLabel_->setText(QString("%1 papers | %2 min total | %3 diff avg")
        .arg(paths_.size()).arg(static_cast<int>(totalEstimatedTime())).arg(avgDifficulty(), 0, 'f', 1));
}

void PaperReadingPathOptimizer::loadSettings() {
    int size = settings_.beginReadArray("paths");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        PathEntry e;
        e.id = settings_.value("id").toInt();
        e.paperTitle = settings_.value("paperTitle").toString();
        e.topic = settings_.value("topic").toString();
        e.order = settings_.value("order").toInt();
        e.difficulty = settings_.value("difficulty").toDouble();
        e.estimatedTime = settings_.value("estimatedTime").toDouble();
        e.prerequisite = settings_.value("prerequisite").toString();
        e.status = settings_.value("status").toString();
        e.color = QColor(settings_.value("color").toString());
        paths_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingPathOptimizer::saveSettings() {
    settings_.beginWriteArray("paths");
    for (int i = 0; i < paths_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", paths_[i].id);
        settings_.setValue("paperTitle", paths_[i].paperTitle);
        settings_.setValue("topic", paths_[i].topic);
        settings_.setValue("order", paths_[i].order);
        settings_.setValue("difficulty", paths_[i].difficulty);
        settings_.setValue("estimatedTime", paths_[i].estimatedTime);
        settings_.setValue("prerequisite", paths_[i].prerequisite);
        settings_.setValue("status", paths_[i].status);
        settings_.setValue("color", paths_[i].color.name());
    }
    settings_.endArray();
}
