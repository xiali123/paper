#include "workspace/PaperReleasePlanner.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QDateTime>

PaperReleasePlanner::PaperReleasePlanner(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReleasePlanner")
{
    setupUI();
    loadSettings();
}

void PaperReleasePlanner::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    updateBtn_ = new QPushButton("Update");
    updateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(updateBtn_, &QPushButton::clicked, this, &PaperReleasePlanner::onUpdate);
    toolbar->addWidget(updateBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Feature", "Bugfix", "Hotfix", "Major", "Minor"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReleasePlanner::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter version string (e.g. 2.1.0)...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Plan release versions");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperReleasePlanner::addEntry(const ReleaseEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit releaseUpdated(entry.id, entry.progress);
    update();
}

QList<ReleaseEntry> PaperReleasePlanner::entries() const { return entries_; }

int PaperReleasePlanner::releasedCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.released) ++c;
    return c;
}

qreal PaperReleasePlanner::avgProgress() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.progress;
    return sum / entries_.size();
}

QMap<QString, int> PaperReleasePlanner::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperReleasePlanner::onUpdate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    static const QStringList categories = {"feature", "bugfix", "hotfix", "major", "minor"};
    static const QStringList statuses = {"planning", "in-progress", "testing", "ready", "deployed"};
    static const QColor palette[] = {
        QColor(59, 130, 246),   // #3b82f6
        QColor(22, 163, 74),    // #16a34a
        QColor(217, 119, 6),    // #d97706
        QColor(220, 38, 38),    // #dc2626
        QColor(124, 58, 237)    // #7c3aed
    };

    int cIdx = categoryCombo_->currentIndex();
    int count = 2 + QRandomGenerator::global()->bounded(4);

    for (int i = 0; i < count; ++i) {
        ReleaseEntry e;
        e.id = entries_.size() + 1;

        QString suffix = (i > 0) ? QString("-%1").arg(i) : "";
        e.version = text + suffix;

        e.category = (cIdx == 0)
            ? categories[QRandomGenerator::global()->bounded(categories.size())]
            : categories[cIdx - 1];

        e.features = 1 + QRandomGenerator::global()->bounded(20);
        e.progress = QRandomGenerator::global()->bounded(101) / 100.0;
        e.status = statuses[QRandomGenerator::global()->bounded(statuses.size())];
        e.released = (e.progress >= 1.0);
        e.date = QDateTime::currentDateTime().addDays(-QRandomGenerator::global()->bounded(90))
                     .toString("yyyy-MM-dd");

        int colorIdx = 0;
        if (e.category == "feature")  colorIdx = 0;
        else if (e.category == "bugfix")  colorIdx = 1;
        else if (e.category == "hotfix")  colorIdx = 2;
        else if (e.category == "major")   colorIdx = 3;
        else                               colorIdx = 4;
        e.color = palette[colorIdx];

        addEntry(e);
    }
    inputField_->clear();
}

void PaperReleasePlanner::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Plan release versions");
    update();
}

void PaperReleasePlanner::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Plan release versions");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Release Planner");

    int w = width(), h = height();
    drawReleaseList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReleasePlanner::drawReleaseList(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(34, (rect.height() - 10) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 3);

        // Background pill
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        // Left accent bar
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        // Line 1: version + status badge
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        QString badge = e.released ? " RELEASED" : "";
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   "v" + e.version.left(12) + " [" + e.status.left(8) + "]" + badge);

        // Line 2: features + progress + date
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   QString::number(e.features) + " feat | " + e.category.left(8));

        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.progress * 100, 'f', 0) + "%");

        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.date);
    }
}

void PaperReleasePlanner::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    static const QStringList cats = {"feature", "bugfix", "hotfix", "major", "minor"};
    static const QString labels[] = {"Feature", "Bugfix", "Hotfix", "Major", "Minor"};
    static const QColor colors[] = {
        QColor(59, 130, 246),
        QColor(22, 163, 74),
        QColor(217, 119, 6),
        QColor(220, 38, 38),
        QColor(124, 58, 237)
    };

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 2);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 60, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 65, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 68 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperReleasePlanner::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries",     QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Released",    QString::number(releasedCount()), QColor(22, 163, 74)},
        {"Avg Progress",QString::number(avgProgress() * 100, 'f', 0) + "%", QColor(217, 119, 6)},
        {"Categories",  QString::number(categoryCounts().size()), QColor(124, 58, 237)}
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

void PaperReleasePlanner::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Plan release versions");
        return;
    }
    infoLabel_->setText(QString("%1 entries | %2 released | %3% avg progress")
        .arg(entries_.size())
        .arg(releasedCount())
        .arg(QString::number(avgProgress() * 100, 'f', 0)));
}

void PaperReleasePlanner::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ReleaseEntry e;
        e.id       = settings_.value("id").toInt();
        e.version  = settings_.value("version").toString();
        e.category = settings_.value("category").toString();
        e.status   = settings_.value("status").toString();
        e.features = settings_.value("features").toInt();
        e.progress = settings_.value("progress").toDouble();
        e.date     = settings_.value("date").toString();
        e.released = settings_.value("released").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReleasePlanner::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",       entries_[i].id);
        settings_.setValue("version",  entries_[i].version);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("status",   entries_[i].status);
        settings_.setValue("features", entries_[i].features);
        settings_.setValue("progress", entries_[i].progress);
        settings_.setValue("date",     entries_[i].date);
        settings_.setValue("released", entries_[i].released);
        settings_.setValue("color",    entries_[i].color.name());
    }
    settings_.endArray();
}
