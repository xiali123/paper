#include "tools/PaperSurveyBuilder.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

PaperSurveyBuilder::PaperSurveyBuilder(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "SurveyBuilder")
{
    setupUI();
    loadSettings();
}

void PaperSurveyBuilder::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Survey");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperSurveyBuilder::onAdd);
    toolbar->addWidget(addBtn_);

    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Draft", "In Progress", "Complete"});
    toolbar->addWidget(filterCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperSurveyBuilder::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Build literature surveys");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperSurveyBuilder::addSurvey(const SurveyEntry& survey) {
    surveys_.append(survey);
    saveSettings();
    updateInfo();
    emit surveyAdded(survey.id, survey.topic);
    update();
}

QList<SurveyEntry> PaperSurveyBuilder::surveys() const { return surveys_; }

QMap<QString, int> PaperSurveyBuilder::statusCounts() const {
    QMap<QString, int> counts;
    for (const auto& s : surveys_) counts[s.status]++;
    return counts;
}

qreal PaperSurveyBuilder::avgCoverage() const {
    if (surveys_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& s : surveys_) sum += s.coverage;
    return sum / surveys_.size();
}

int PaperSurveyBuilder::totalQuestions() const {
    int t = 0;
    for (const auto& s : surveys_) t += s.questionCount;
    return t;
}

void PaperSurveyBuilder::onAdd() {
    bool ok;
    QString topic = QInputDialog::getText(this, "Add Survey", "Topic:", QLineEdit::Normal, "", &ok);
    if (!ok || topic.isEmpty()) return;
    QStringList statuses = {"draft", "in-progress", "complete"};
    QString status = QInputDialog::getItem(this, "Add Survey", "Status:", statuses, 0, false, &ok);
    if (!ok) return;

    SurveyEntry s;
    s.id = surveys_.size() + 1;
    s.topic = topic;
    s.status = status;
    s.questionCount = 3 + QRandomGenerator::global()->bounded(15);
    s.coverage = 0.2 + QRandomGenerator::global()->bounded(80) / 100.0;
    s.notes = "Survey on " + topic.left(15);
    s.papers = QStringList{"Paper1", "Paper2", "Paper3"};

    QColor statusColors[] = {QColor(100,116,139), QColor(59,130,246), QColor(16,185,129)};
    int sIdx = statuses.indexOf(status);
    s.color = statusColors[qBound(0, sIdx, 2)];
    addSurvey(s);
}

void PaperSurveyBuilder::onClear() {
    surveys_.clear();
    saveSettings();
    infoLabel_->setText("Build literature surveys");
    update();
}

void PaperSurveyBuilder::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (surveys_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Build literature surveys");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Survey Builder");

    int w = width(), h = height();
    drawSurveyList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawStatusChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperSurveyBuilder::drawSurveyList(QPainter& p, const QRect& rect) {
    int filterIdx = filterCombo_->currentIndex();
    int show = 0;
    int maxShow = 10;
    int itemH = qMin(38, (rect.height() - 10) / maxShow);

    for (int i = surveys_.size() - 1; i >= 0 && show < maxShow; --i) {
        const auto& s = surveys_[i];
        if (filterIdx == 1 && s.status != "draft") continue;
        if (filterIdx == 2 && s.status != "in-progress") continue;
        if (filterIdx == 3 && s.status != "complete") continue;

        int y = rect.y() + show * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(s.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(s.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   s.topic.left(20));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   s.status + " | " + QString::number(s.papers.size()) + " papers");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(s.questionCount) + " questions");

        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(s.coverage * 100, 'f', 0) + "% coverage");
        show++;
    }
}

void PaperSurveyBuilder::drawStatusChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Status");

    auto counts = statusCounts();
    QStringList statuses = {"draft", "in-progress", "complete"};
    QString labels[] = {"Draft", "In Progress", "Complete"};
    QColor colors[] = {QColor(100,116,139), QColor(59,130,246), QColor(16,185,129)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(24, (rect.height() - 30) / 3);
    for (int i = 0; i < 3; ++i) {
        int y = rect.y() + 22 + i * (barH + 4);
        int count = counts.contains(statuses[i]) ? counts[statuses[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 120));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 3, 75, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 80, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 83 + barW, y + barH - 3, QString::number(count));
    }
}

void PaperSurveyBuilder::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Surveys", QString::number(surveys_.size()), QColor(59,130,246)},
        {"Questions", QString::number(totalQuestions()), QColor(16,185,129)},
        {"Coverage", QString::number(avgCoverage() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Complete", QString::number(statusCounts().value("complete", 0)), QColor(139,92,246)}
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

void PaperSurveyBuilder::updateInfo() {
    if (surveys_.isEmpty()) { infoLabel_->setText("Build literature surveys"); return; }
    infoLabel_->setText(QString("%1 surveys | %2 questions | %3% coverage")
        .arg(surveys_.size()).arg(totalQuestions()).arg(avgCoverage() * 100, 0, 'f', 0));
}

void PaperSurveyBuilder::loadSettings() {
    int size = settings_.beginReadArray("surveys");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        SurveyEntry s;
        s.id = settings_.value("id").toInt();
        s.topic = settings_.value("topic").toString();
        s.papers = settings_.value("papers").toStringList();
        s.status = settings_.value("status").toString();
        s.questionCount = settings_.value("questionCount").toInt();
        s.coverage = settings_.value("coverage").toDouble();
        s.notes = settings_.value("notes").toString();
        s.color = QColor(settings_.value("color").toString());
        surveys_.append(s);
    }
    settings_.endArray();
    updateInfo();
}

void PaperSurveyBuilder::saveSettings() {
    settings_.beginWriteArray("surveys");
    for (int i = 0; i < surveys_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", surveys_[i].id);
        settings_.setValue("topic", surveys_[i].topic);
        settings_.setValue("papers", surveys_[i].papers);
        settings_.setValue("status", surveys_[i].status);
        settings_.setValue("questionCount", surveys_[i].questionCount);
        settings_.setValue("coverage", surveys_[i].coverage);
        settings_.setValue("notes", surveys_[i].notes);
        settings_.setValue("color", surveys_[i].color.name());
    }
    settings_.endArray();
}
