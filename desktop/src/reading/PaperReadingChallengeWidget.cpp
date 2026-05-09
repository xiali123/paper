#include "reading/PaperReadingChallengeWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

PaperReadingChallengeWidget::PaperReadingChallengeWidget(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingChallenge")
{
    setupUI();
    loadSettings();
}

void PaperReadingChallengeWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Challenge");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperReadingChallengeWidget::onAdd);
    toolbar->addWidget(addBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingChallengeWidget::onClear);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Set reading challenges");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperReadingChallengeWidget::addChallenge(const ChallengeEntry& challenge) {
    challenges_.append(challenge);
    saveSettings();
    updateInfo();
    update();
}

QList<ChallengeEntry> PaperReadingChallengeWidget::challenges() const { return challenges_; }

QMap<QString, int> PaperReadingChallengeWidget::typeCounts() const {
    QMap<QString, int> counts;
    for (const auto& c : challenges_) counts[c.type]++;
    return counts;
}

int PaperReadingChallengeWidget::activeChallenges() const {
    int c = 0;
    for (const auto& ch : challenges_) if (!ch.completed) c++;
    return c;
}

int PaperReadingChallengeWidget::completedChallenges() const {
    int c = 0;
    for (const auto& ch : challenges_) if (ch.completed) c++;
    return c;
}

void PaperReadingChallengeWidget::onAdd() {
    bool ok;
    QString name = QInputDialog::getText(this, "Add Challenge", "Challenge name:", QLineEdit::Normal, "", &ok);
    if (!ok || name.isEmpty()) return;
    QStringList types = {"daily", "weekly", "monthly", "custom"};
    QString type = QInputDialog::getItem(this, "Add Challenge", "Type:", types, 0, false, &ok);
    if (!ok) return;
    int target = QInputDialog::getInt(this, "Add Challenge", "Target (papers):", 5, 1, 100, 1, &ok);
    if (!ok) return;

    ChallengeEntry c;
    c.id = challenges_.size() + 1;
    c.name = name;
    c.type = type;
    c.target = target;
    c.current = QRandomGenerator::global()->bounded(target);
    c.startDate = QDate::currentDate();
    c.endDate = type == "daily" ? QDate::currentDate().addDays(1) :
                type == "weekly" ? QDate::currentDate().addDays(7) :
                type == "monthly" ? QDate::currentDate().addMonths(1) :
                QDate::currentDate().addDays(14);
    c.completed = c.current >= c.target;
    c.reward = c.completed ? "Badge earned!" : "Keep reading!";

    QColor typeColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int tIdx = types.indexOf(type);
    c.color = typeColors[qBound(0, tIdx, 3)];
    addChallenge(c);
}

void PaperReadingChallengeWidget::onClear() {
    challenges_.clear();
    saveSettings();
    infoLabel_->setText("Set reading challenges");
    update();
}

void PaperReadingChallengeWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (challenges_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Set reading challenges");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Challenges");

    int w = width(), h = height();
    drawChallengeCards(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawTypeChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReadingChallengeWidget::drawChallengeCards(QPainter& p, const QRect& rect) {
    int show = qMin(8, challenges_.size());
    int cardH = qMin(48, (rect.height() - 10) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& c = challenges_[i];
        int y = rect.y() + i * (cardH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(c.completed ? QColor(16,185,129).lighter(170) : c.color.lighter(180));
        p.drawRoundedRect(rect.x(), y, rect.width(), cardH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(c.completed ? QColor(16,185,129) : c.color);
        p.drawRoundedRect(rect.x(), y, 4, cardH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   c.name.left(18));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   c.type + " | " + QString::number(c.current) + "/" + QString::number(c.target));
        p.drawText(rect.x() + 10, y + 34, rect.width() / 2 - 10, 12, Qt::AlignVCenter,
                   c.endDate.toString("MM/dd") + " | " + c.reward.left(14));

        int barX = rect.x() + rect.width() / 2 + 10;
        int barW = rect.width() / 2 - 25;
        qreal prog = c.target > 0 ? static_cast<qreal>(c.current) / c.target : 0;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(241, 245, 249));
        p.drawRoundedRect(barX, y + 6, barW, 10, 5, 5);
        p.setBrush(c.completed ? QColor(16,185,129) : c.color);
        p.drawRoundedRect(barX, y + 6, static_cast<int>(barW * qMin(prog, 1.0)), 10, 5, 5);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(barX, y + 20, barW, 16, Qt::AlignVCenter,
                   QString::number(prog * 100, 'f', 0) + "%");

        if (c.completed) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(16, 185, 129));
            p.drawEllipse(rect.x() + rect.width() - 16, y + 6, 10, 10);
        }
    }
}

void PaperReadingChallengeWidget::drawTypeChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "By Type");

    auto counts = typeCounts();
    QStringList types = {"daily", "weekly", "monthly", "custom"};
    QString labels[] = {"Daily", "Weekly", "Monthly", "Custom"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};

    int barW = (rect.width() - 30) / 4;
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    for (int i = 0; i < 4; ++i) {
        int x = rect.x() + 10 + i * barW;
        int count = counts.contains(types[i]) ? counts[types[i]] : 0;
        qreal h = (static_cast<qreal>(count) / maxVal) * (rect.height() - 55);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(x + 4, rect.bottom() - 25 - static_cast<int>(h), barW - 8, static_cast<int>(h), 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(x, rect.bottom() - 8, barW, 14, Qt::AlignCenter, labels[i]);
    }
}

void PaperReadingChallengeWidget::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Challenges", QString::number(challenges_.size()), QColor(59,130,246)},
        {"Active", QString::number(activeChallenges()), QColor(16,185,129)},
        {"Completed", QString::number(completedChallenges()), QColor(245,158,11)},
        {"Types", QString::number(typeCounts().size()), QColor(139,92,246)}
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

void PaperReadingChallengeWidget::updateInfo() {
    if (challenges_.isEmpty()) { infoLabel_->setText("Set reading challenges"); return; }
    infoLabel_->setText(QString("%1 challenges | %2 active | %3 done")
        .arg(challenges_.size()).arg(activeChallenges()).arg(completedChallenges()));
}

void PaperReadingChallengeWidget::loadSettings() {
    int size = settings_.beginReadArray("challenges");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ChallengeEntry c;
        c.id = settings_.value("id").toInt();
        c.name = settings_.value("name").toString();
        c.type = settings_.value("type").toString();
        c.target = settings_.value("target").toInt();
        c.current = settings_.value("current").toInt();
        c.startDate = QDate::fromString(settings_.value("startDate").toString(), Qt::ISODate);
        c.endDate = QDate::fromString(settings_.value("endDate").toString(), Qt::ISODate);
        c.reward = settings_.value("reward").toString();
        c.completed = settings_.value("completed").toBool();
        c.color = QColor(settings_.value("color").toString());
        challenges_.append(c);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingChallengeWidget::saveSettings() {
    settings_.beginWriteArray("challenges");
    for (int i = 0; i < challenges_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", challenges_[i].id);
        settings_.setValue("name", challenges_[i].name);
        settings_.setValue("type", challenges_[i].type);
        settings_.setValue("target", challenges_[i].target);
        settings_.setValue("current", challenges_[i].current);
        settings_.setValue("startDate", challenges_[i].startDate.toString(Qt::ISODate));
        settings_.setValue("endDate", challenges_[i].endDate.toString(Qt::ISODate));
        settings_.setValue("reward", challenges_[i].reward);
        settings_.setValue("completed", challenges_[i].completed);
        settings_.setValue("color", challenges_[i].color.name());
    }
    settings_.endArray();
}
