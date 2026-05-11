#include "workspace/PaperTeamScoreboard.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QSettings>
#include <QRandomGenerator>
#include <QScrollBar>

PaperTeamScoreboard::PaperTeamScoreboard(QWidget *parent)
    : QWidget(parent)
    , nextId_(1)
{
    setupUI();
    loadSettings();
}

void PaperTeamScoreboard::setupUI()
{
    setMinimumSize(580, 480);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(8);

    // Toolbar
    auto *toolbar = new QHBoxLayout;
    toolbar->setSpacing(8);

    addBtn_ = new QPushButton(tr("Add"));
    addBtn_->setStyleSheet(
        "QPushButton { background-color: #3b82f6; color: white; border: none; "
        "border-radius: 6px; padding: 8px 18px; font-weight: bold; font-size: 13px; }"
        "QPushButton:hover { background-color: #2563eb; }"
        "QPushButton:pressed { background-color: #1d4ed8; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperTeamScoreboard::onAdd);
    toolbar->addWidget(addBtn_);

    periodCombo_ = new QComboBox;
    periodCombo_->addItems({tr("All"), tr("Weekly"), tr("Monthly"), tr("Quarterly")});
    periodCombo_->setStyleSheet(
        "QComboBox { border: 1px solid #d1d5db; border-radius: 6px; padding: 6px 12px; "
        "min-width: 120px; }");
    toolbar->addWidget(periodCombo_);

    memberEdit_ = new QLineEdit;
    memberEdit_->setPlaceholderText(tr("Enter team member..."));
    memberEdit_->setStyleSheet(
        "QLineEdit { border: 1px solid #d1d5db; border-radius: 6px; padding: 6px 12px; }");
    toolbar->addWidget(memberEdit_, 1);

    infoLabel_ = new QLabel(tr("Track team scores"));
    infoLabel_->setStyleSheet("color: #6b7280; font-size: 12px;");
    toolbar->addWidget(infoLabel_);

    mainLayout->addLayout(toolbar);

    // Scroll area
    scrollArea_ = new QScrollArea;
    scrollArea_->setWidgetResizable(true);
    scrollArea_->setStyleSheet("QScrollArea { border: none; background: white; }");
    contentWidget_ = new QWidget;
    contentWidget_->setStyleSheet("background: white;");
    contentLayout_ = new QVBoxLayout(contentWidget_);
    contentLayout_->setSpacing(10);
    contentLayout_->setContentsMargins(4, 4, 4, 4);
    scrollArea_->setWidget(contentWidget_);
    mainLayout->addWidget(scrollArea_, 1);
}

void PaperTeamScoreboard::onAdd()
{
    const QStringList categories = {"weekly", "monthly", "quarterly"};
    const QStringList metrics = {"commits", "reviews", "tasks", "velocity"};
    const QString text = memberEdit_->text().trimmed().isEmpty()
        ? "member" : memberEdit_->text().trimmed();

    const int count = 3 + QRandomGenerator::global()->bounded(4); // 3-6

    for (int i = 0; i < count; ++i) {
        ScoreEntry e;
        e.id = nextId_++;
        e.member = QString("%1_%2").arg(text).arg(i + 1);
        e.category = categories.at(QRandomGenerator::global()->bounded(categories.size()));
        e.metric = metrics.at(QRandomGenerator::global()->bounded(metrics.size()));
        e.score = QRandomGenerator::global()->bounded(100);
        e.rank = 1 + QRandomGenerator::global()->bounded(10);
        e.period = e.category;
        e.leader = e.rank <= 3;

        // Color: leader amber, score>=70 green, else blue
        if (e.leader) {
            e.color = QColor("#f59e0b"); // amber #245,158,11 -> actual amber
        } else if (e.score >= 70) {
            e.color = QColor("#16a34a"); // green
        } else {
            e.color = QColor("#3b82f6"); // blue
        }

        entries_.append(e);
        emit scoreUpdated(e.id, e.score);
    }

    updateInfo();
    update();
}

void PaperTeamScoreboard::updateInfo()
{
    if (entries_.isEmpty()) {
        infoLabel_->setText(tr("Track team scores"));
        return;
    }
    const int leaderCount = std::count_if(entries_.constBegin(), entries_.constEnd(),
        [](const ScoreEntry &e) { return e.leader; });
    double avgScore = 0;
    for (const auto &e : entries_) avgScore += e.score;
    avgScore /= entries_.size();

    infoLabel_->setText(tr("%1 scores | %2 leaders | %3 avg")
        .arg(entries_.size()).arg(leaderCount).arg(avgScore, 0, 'f', 1));
}

void PaperTeamScoreboard::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    drawScoreList(p);
    drawCategoryChart(p);
    drawStats(p);
}

void PaperTeamScoreboard::drawScoreList(QPainter &p)
{
    if (entries_.isEmpty()) return;

    int y = 60;
    p.setFont(QFont("Sans", 11, QFont::Bold));
    p.setPen(QColor("#1f2937"));
    p.drawText(12, y, tr("Team Scores"));
    y += 24;

    p.setFont(QFont("Sans", 10));
    for (const auto &e : entries_) {
        if (y > height() - 120) break;

        // Background card
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#f9fafb"));
        p.drawRoundedRect(10, y - 14, width() - 20, 44, 6, 6);

        // Color indicator
        p.setBrush(e.color);
        p.drawRoundedRect(14, y - 6, 6, 28, 3, 3);

        // Rank badge
        QColor rankBg = e.leader ? QColor("#f59e0b") : QColor("#6b7280");
        p.setBrush(rankBg);
        p.drawRoundedRect(28, y - 6, 30, 20, 4, 4);
        p.setPen(Qt::white);
        p.setFont(QFont("Sans", 9, QFont::Bold));
        p.drawText(32, y + 8, QString("#%1").arg(e.rank));

        // Member name
        p.setPen(QColor("#1f2937"));
        p.setFont(QFont("Sans", 10));
        p.drawText(68, y + 6, e.member);

        // Metric
        p.setPen(QColor("#6b7280"));
        p.drawText(180, y + 6, e.metric);

        // Score bar
        int barWidth = static_cast<int>((e.score / 100.0) * 100);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(270, y - 2, barWidth, 14, 4, 4);
        p.setPen(QColor("#374151"));
        p.setFont(QFont("Sans", 9));
        p.drawText(270 + barWidth + 4, y + 8, QString::number(e.score));

        // Period
        p.setPen(QColor("#6b7280"));
        p.drawText(420, y + 6, e.period);

        // Leader indicator
        if (e.leader) {
            p.setPen(QColor("#f59e0b"));
            p.setFont(QFont("Sans", 9, QFont::Bold));
            p.drawText(490, y + 6, "Leader");
        }

        y += 52;
    }
}

void PaperTeamScoreboard::drawCategoryChart(QPainter &p)
{
    if (entries_.isEmpty()) return;

    QMap<QString, int> counts;
    for (const auto &e : entries_) counts[e.category]++;

    int y = height() - 100;
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.setPen(QColor("#1f2937"));
    p.drawText(12, y, tr("By Period"));
    y += 18;

    const QStringList colors = {"#3b82f6", "#139,92,246", "#16a34a"};
    int idx = 0;
    int maxVal = *std::max_element(counts.constBegin(), counts.constEnd());
    if (maxVal == 0) maxVal = 1;

    p.setFont(QFont("Sans", 9));
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        QColor barColor(colors.at(idx % colors.size()));
        p.setPen(Qt::NoPen);
        p.setBrush(barColor);
        int bw = static_cast<int>((static_cast<double>(it.value()) / maxVal) * 140);
        p.drawRoundedRect(80, y - 10, bw, 16, 4, 4);

        p.setPen(QColor("#374151"));
        p.drawText(12, y + 2, it.key());
        p.drawText(80 + bw + 6, y + 2, QString::number(it.value()));
        y += 22;
        ++idx;
    }
}

void PaperTeamScoreboard::drawStats(QPainter &p)
{
    if (entries_.isEmpty()) return;

    int x = width() - 200;
    int y = height() - 90;
    const int leaderCount = std::count_if(entries_.constBegin(), entries_.constEnd(),
        [](const ScoreEntry &e) { return e.leader; });
    double avgScore = 0;
    for (const auto &e : entries_) avgScore += e.score;
    avgScore /= entries_.size();
    QSet<QString> cats;
    for (const auto &e : entries_) cats.insert(e.category);

    p.setFont(QFont("Sans", 9));
    p.setPen(QColor("#6b7280"));
    p.drawText(x, y, tr("Members: %1").arg(entries_.size()));
    p.drawText(x, y + 16, tr("Leaders: %1").arg(leaderCount));
    p.drawText(x, y + 32, tr("Avg Score: %1").arg(avgScore, 0, 'f', 1));
    p.drawText(x, y + 48, tr("Categories: %1").arg(cats.size()));
}

void PaperTeamScoreboard::loadSettings()
{
    QSettings s("PaperCrawler", "PaperTeamScoreboard");
    const int size = s.beginReadArray("entries");
    entries_.clear();
    for (int i = 0; i < size; ++i) {
        s.setArrayIndex(i);
        ScoreEntry e;
        e.id = s.value("id").toInt();
        e.member = s.value("member").toString();
        e.category = s.value("category").toString();
        e.metric = s.value("metric").toString();
        e.score = s.value("score").toInt();
        e.rank = s.value("rank").toInt();
        e.period = s.value("period").toString();
        e.leader = s.value("leader").toBool();
        e.color = QColor(s.value("color").toString());
        entries_.append(e);
        if (e.id >= nextId_) nextId_ = e.id + 1;
    }
    s.endArray();
    updateInfo();
    update();
}

void PaperTeamScoreboard::saveSettings()
{
    QSettings s("PaperCrawler", "PaperTeamScoreboard");
    s.beginWriteArray("entries", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        s.setArrayIndex(i);
        const auto &e = entries_.at(i);
        s.setValue("id", e.id);
        s.setValue("member", e.member);
        s.setValue("category", e.category);
        s.setValue("metric", e.metric);
        s.setValue("score", e.score);
        s.setValue("rank", e.rank);
        s.setValue("period", e.period);
        s.setValue("leader", e.leader);
        s.setValue("color", e.color.name());
    }
    s.endArray();
}
