#include "workspace/PaperCollaborationMatcher.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

PaperCollaborationMatcher::PaperCollaborationMatcher(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CollaborationMatcher")
{
    setupUI();
    loadSettings();
}

void PaperCollaborationMatcher::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    matchBtn_ = new QPushButton("Find Matches");
    matchBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(matchBtn_, &QPushButton::clicked, this, &PaperCollaborationMatcher::onMatch);
    toolbar->addWidget(matchBtn_);

    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Co-author", "Reviewer", "Mentor"});
    toolbar->addWidget(filterCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCollaborationMatcher::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter researcher name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Find collaboration matches");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperCollaborationMatcher::addMatch(const MatchEntry& entry) {
    matches_.append(entry);
    saveSettings();
    updateInfo();
    emit matchFound(entry.id, entry.compatibility);
    update();
}

QList<MatchEntry> PaperCollaborationMatcher::matches() const { return matches_; }

QMap<QString, int> PaperCollaborationMatcher::typeCounts() const {
    QMap<QString, int> counts;
    for (const auto& m : matches_) counts[m.collaborationType]++;
    return counts;
}

qreal PaperCollaborationMatcher::avgCompatibility() const {
    if (matches_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& m : matches_) sum += m.compatibility;
    return sum / matches_.size();
}

int PaperCollaborationMatcher::totalSharedPapers() const {
    int t = 0;
    for (const auto& m : matches_) t += m.sharedPapers;
    return t;
}

void PaperCollaborationMatcher::onMatch() {
    QString name = inputField_->text().trimmed();
    if (name.isEmpty()) return;

    QStringList researchers = {"Dr. Smith", "Prof. Lee", "Dr. Chen", "Prof. Kim", "Dr. Wang", "Prof. Garcia"};
    QStringList types = {"co-author", "reviewer", "mentor"};
    QColor typeColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11)};
    QStringList topics = {"ML", "NLP", "CV", "RL", "Graph", "Optimization"};

    int count = 2 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        MatchEntry e;
        e.id = matches_.size() + 1;
        e.researcherA = name;
        e.researcherB = researchers[QRandomGenerator::global()->bounded(researchers.size())];
        e.compatibility = 0.4 + QRandomGenerator::global()->bounded(60) / 100.0;
        int numTopics = 1 + QRandomGenerator::global()->bounded(3);
        for (int j = 0; j < numTopics; ++j) {
            QString topic = topics[QRandomGenerator::global()->bounded(topics.size())];
            if (!e.sharedTopics.contains(topic)) e.sharedTopics << topic;
        }
        int tIdx = QRandomGenerator::global()->bounded(types.size());
        e.collaborationType = types[tIdx];
        e.sharedPapers = 1 + QRandomGenerator::global()->bounded(15);
        e.score = e.compatibility * 0.6 + (e.sharedPapers / 16.0) * 0.4;
        e.color = typeColors[tIdx];
        addMatch(e);
    }
    inputField_->clear();
}

void PaperCollaborationMatcher::onClear() {
    matches_.clear();
    saveSettings();
    infoLabel_->setText("Find collaboration matches");
    update();
}

void PaperCollaborationMatcher::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (matches_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Find collaboration matches");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Collaboration Matcher");

    int w = width(), h = height();
    drawMatchList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawTypeChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperCollaborationMatcher::drawMatchList(QPainter& p, const QRect& rect) {
    int filterIdx = filterCombo_->currentIndex();
    int show = 0;
    int maxShow = 10;
    int itemH = qMin(36, (rect.height() - 10) / maxShow);

    for (int i = matches_.size() - 1; i >= 0 && show < maxShow; --i) {
        const auto& m = matches_[i];
        if (filterIdx == 1 && m.collaborationType != "co-author") continue;
        if (filterIdx == 2 && m.collaborationType != "reviewer") continue;
        if (filterIdx == 3 && m.collaborationType != "mentor") continue;

        int y = rect.y() + show * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(m.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(m.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   m.researcherB.left(16));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   m.collaborationType + " | " + QString::number(m.sharedPapers) + " shared");
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(m.compatibility * 100, 'f', 0) + "% compat");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   m.sharedTopics.join(", ").left(18));
        show++;
    }
}

void PaperCollaborationMatcher::drawTypeChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Types");

    auto counts = typeCounts();
    QStringList types = {"co-author", "reviewer", "mentor"};
    QString labels[] = {"Co-author", "Reviewer", "Mentor"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(28, (rect.height() - 30) / 3);
    for (int i = 0; i < 3; ++i) {
        int y = rect.y() + 22 + i * (barH + 4);
        int count = counts.contains(types[i]) ? counts[types[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 130));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x(), y + barH - 3, 75, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 80, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 83 + barW, y + barH - 3, QString::number(count));
    }
}

void PaperCollaborationMatcher::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Matches", QString::number(matches_.size()), QColor(59,130,246)},
        {"Shared Papers", QString::number(totalSharedPapers()), QColor(16,185,129)},
        {"Avg Compat", QString::number(avgCompatibility() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Co-authors", QString::number(typeCounts().value("co-author", 0)), QColor(139,92,246)}
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

void PaperCollaborationMatcher::updateInfo() {
    if (matches_.isEmpty()) { infoLabel_->setText("Find collaboration matches"); return; }
    infoLabel_->setText(QString("%1 matches | %2 shared | %3% compat")
        .arg(matches_.size()).arg(totalSharedPapers()).arg(avgCompatibility() * 100, 0, 'f', 0));
}

void PaperCollaborationMatcher::loadSettings() {
    int size = settings_.beginReadArray("matches");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        MatchEntry e;
        e.id = settings_.value("id").toInt();
        e.researcherA = settings_.value("researcherA").toString();
        e.researcherB = settings_.value("researcherB").toString();
        e.compatibility = settings_.value("compatibility").toDouble();
        e.sharedTopics = settings_.value("sharedTopics").toStringList();
        e.collaborationType = settings_.value("collaborationType").toString();
        e.sharedPapers = settings_.value("sharedPapers").toInt();
        e.score = settings_.value("score").toDouble();
        e.color = QColor(settings_.value("color").toString());
        matches_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperCollaborationMatcher::saveSettings() {
    settings_.beginWriteArray("matches");
    for (int i = 0; i < matches_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", matches_[i].id);
        settings_.setValue("researcherA", matches_[i].researcherA);
        settings_.setValue("researcherB", matches_[i].researcherB);
        settings_.setValue("compatibility", matches_[i].compatibility);
        settings_.setValue("sharedTopics", matches_[i].sharedTopics);
        settings_.setValue("collaborationType", matches_[i].collaborationType);
        settings_.setValue("sharedPapers", matches_[i].sharedPapers);
        settings_.setValue("score", matches_[i].score);
        settings_.setValue("color", matches_[i].color.name());
    }
    settings_.endArray();
}
