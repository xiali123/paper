#include "workspace/PaperCodeReviewTracker.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>

PaperCodeReviewTracker::PaperCodeReviewTracker(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CodeReviewTracker")
{
    setupUI();
    loadSettings();
}

void PaperCodeReviewTracker::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Bug Fix", "Feature", "Refactor", "Performance", "Security"});
    toolbar->addWidget(new QLabel("Category:"));
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Review title...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 2);

    addBtn_ = new QPushButton("Add Review");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperCodeReviewTracker::onAdd);
    toolbar->addWidget(addBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCodeReviewTracker::onClear);
    toolbar->addWidget(clearBtn_);

    toolbar->addStretch();
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Code Review Tracker");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperCodeReviewTracker::addEntry(const CodeReviewEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit reviewComplete(entry.id, entry.complexity);
    update();
}

QList<CodeReviewEntry> PaperCodeReviewTracker::entries() const { return entries_; }

int PaperCodeReviewTracker::approvedCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.approved) c++;
    return c;
}

qreal PaperCodeReviewTracker::avgComplexity() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.complexity;
    return sum / entries_.size();
}

QMap<QString, int> PaperCodeReviewTracker::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperCodeReviewTracker::onAdd() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"Bug Fix", "Feature", "Refactor", "Performance", "Security"};
    QStringList reviewers = {"alice", "bob", "carol", "dave", "eve"};
    QColor palette[] = {
        QColor(59, 130, 246),   // #3b82f6
        QColor(22, 163, 74),    // #16a34a
        QColor(217, 119, 6),    // #d97706
        QColor(220, 38, 38),    // #dc2626
        QColor(124, 58, 237)    // #7c3aed
    };

    int cIdx = categoryCombo_->currentIndex();
    CodeReviewEntry e;
    e.id = entries_.size() + 1;
    e.title = text;
    e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())]
                            : categories[cIdx - 1];
    e.reviewer = reviewers[QRandomGenerator::global()->bounded(reviewers.size())];
    e.complexity = QRandomGenerator::global()->bounded(100) / 100.0;
    e.comments = QRandomGenerator::global()->bounded(51);
    e.approved = QRandomGenerator::global()->bounded(2) == 1;
    int colorIdx = categories.indexOf(e.category);
    e.color = colorIdx >= 0 ? palette[colorIdx] : palette[0];

    addEntry(e);
    inputField_->clear();
}

void PaperCodeReviewTracker::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperCodeReviewTracker::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No code reviews yet");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Code Review Tracker");

    int w = width(), h = height();
    drawReviewList(p, QRect(20, 50, w / 3 - 10, h - 80));
    drawCategoryChart(p, QRect(w / 3 + 10, 50, w / 3 - 20, h - 80));
    drawStats(p, QRect(2 * w / 3 + 10, 50, w / 3 - 30, h - 80));
}

void PaperCodeReviewTracker::drawReviewList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Code Reviews");

    int show = qMin(10, entries_.size());
    int itemH = qMin(40, (rect.height() - 30) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + 22 + i * (itemH + 3);

        // Background bar
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        // Left accent
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        // Complexity bar
        int barMaxW = rect.width() - 14;
        int barW = static_cast<int>(e.complexity * barMaxW);
        p.setBrush(e.color.lighter(140));
        p.drawRoundedRect(rect.x() + 8, y + itemH - 10, barW, 6, 3, 3);

        // Title and info line
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 2, rect.width() - 50, 16, Qt::AlignVCenter,
                   e.title.left(20));

        // Comment count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 18, rect.width() / 2, 14, Qt::AlignVCenter,
                   QString::number(e.comments) + " comments");

        // Approved badge (green check)
        if (e.approved) {
            p.setPen(QColor(22, 163, 74));
            p.setFont(QFont("Arial", 10, QFont::Bold));
            p.drawText(rect.x() + rect.width() - 22, y + 4, 20, 20,
                       Qt::AlignVCenter | Qt::AlignRight, QString::fromUtf8("✓"));
        } else {
            p.setPen(QColor(220, 38, 38));
            p.setFont(QFont("Arial", 9));
            p.drawText(rect.x() + rect.width() - 28, y + 4, 26, 20,
                       Qt::AlignVCenter | Qt::AlignRight, QString::fromUtf8("✗"));
        }

        // Reviewer
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + rect.width() / 2, y + 18, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.reviewer);
    }
}

void PaperCodeReviewTracker::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList categories = {"Bug Fix", "Feature", "Refactor", "Performance", "Security"};
    QColor colors[] = {
        QColor(59, 130, 246),
        QColor(22, 163, 74),
        QColor(217, 119, 6),
        QColor(220, 38, 38),
        QColor(124, 58, 237)
    };

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 40) / 5);

    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 6);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 120));

        // Label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 75, barH, Qt::AlignRight | Qt::AlignVCenter,
                   categories[i]);

        // Bar
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 80, y, barW, barH - 2, 3, 3);

        // Count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 83 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperCodeReviewTracker::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Reviews", QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Approved",      QString::number(approvedCount()), QColor(22, 163, 74)},
        {"Avg Complexity", QString::number(avgComplexity(), 'f', 2), QColor(217, 119, 6)}
    };

    int boxH = qMin(48, (rect.height() - 30) / 3);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + 22 + i * (boxH + 8);

        // Background box
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() - 20, 24, Qt::AlignVCenter,
                   stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 28, rect.width() - 20, 16, Qt::AlignVCenter,
                   stats[i].label);
    }
}

void PaperCodeReviewTracker::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Code Review Tracker");
        return;
    }
    infoLabel_->setText(QString("Reviews: %1 | Approved: %2 | Avg Complexity: %3")
        .arg(entries_.size())
        .arg(approvedCount())
        .arg(avgComplexity(), 0, 'f', 2));
}

void PaperCodeReviewTracker::loadSettings() {
    settings_.beginGroup("CodeReviewTracker");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        CodeReviewEntry e;
        e.id = settings_.value("id").toInt();
        e.title = settings_.value("title").toString();
        e.category = settings_.value("category").toString();
        e.reviewer = settings_.value("reviewer").toString();
        e.complexity = settings_.value("complexity").toDouble();
        e.comments = settings_.value("comments").toInt();
        e.approved = settings_.value("approved").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperCodeReviewTracker::saveSettings() {
    settings_.beginGroup("CodeReviewTracker");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("title", entries_[i].title);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("reviewer", entries_[i].reviewer);
        settings_.setValue("complexity", entries_[i].complexity);
        settings_.setValue("comments", entries_[i].comments);
        settings_.setValue("approved", entries_[i].approved);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
