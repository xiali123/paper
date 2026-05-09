#include "workspace/PaperExperimentTracker.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

PaperExperimentTracker::PaperExperimentTracker(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ExperimentTracker")
{
    setupUI();
    loadSettings();
}

void PaperExperimentTracker::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Experiment");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperExperimentTracker::onAdd);
    toolbar->addWidget(addBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperExperimentTracker::onClear);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Track research experiments");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperExperimentTracker::addExperiment(const ExperimentEntry& experiment) {
    experiments_.append(experiment);
    saveSettings();
    updateInfo();
    update();
}

QList<ExperimentEntry> PaperExperimentTracker::experiments() const { return experiments_; }

QMap<QString, int> PaperExperimentTracker::statusCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : experiments_) counts[e.status]++;
    return counts;
}

int PaperExperimentTracker::activeExperiments() const {
    int c = 0;
    for (const auto& e : experiments_) if (e.status == "running") c++;
    return c;
}

qreal PaperExperimentTracker::completionRate() const {
    if (experiments_.isEmpty()) return 0;
    int completed = 0;
    for (const auto& e : experiments_) if (e.status == "completed") completed++;
    return static_cast<qreal>(completed) / experiments_.size();
}

void PaperExperimentTracker::onAdd() {
    bool ok;
    QString name = QInputDialog::getText(this, "Add Experiment", "Name:", QLineEdit::Normal, "", &ok);
    if (!ok || name.isEmpty()) return;
    QString hypothesis = QInputDialog::getText(this, "Add Experiment", "Hypothesis:", QLineEdit::Normal, "", &ok);
    if (!ok) return;
    QStringList statuses = {"planned", "running", "completed", "failed"};
    QString status = QInputDialog::getItem(this, "Add Experiment", "Status:", statuses, 0, false, &ok);
    if (!ok) return;

    ExperimentEntry e;
    e.id = experiments_.size() + 1;
    e.name = name;
    e.hypothesis = hypothesis.isEmpty() ? "TBD" : hypothesis;
    e.status = status;
    e.startDate = QDate::currentDate();
    e.endDate = QDate::currentDate().addDays(7 + QRandomGenerator::global()->bounded(30));

    if (status == "completed") e.progress = 100;
    else if (status == "running") e.progress = 10 + QRandomGenerator::global()->bounded(80);
    else if (status == "failed") e.progress = QRandomGenerator::global()->bounded(60);
    else e.progress = 0;

    for (int i = 0; i < 3; ++i) e.parameters.append("param" + QString::number(i + 1));
    e.result = status == "completed" ? "Success" : status == "failed" ? "Error" : "Pending";

    QColor statusColors[] = {QColor(100,116,139), QColor(59,130,246), QColor(16,185,129), QColor(239,68,68)};
    int sIdx = statuses.indexOf(status);
    e.color = statusColors[qBound(0, sIdx, 3)];
    addExperiment(e);
}

void PaperExperimentTracker::onClear() {
    experiments_.clear();
    saveSettings();
    infoLabel_->setText("Track research experiments");
    update();
}

void PaperExperimentTracker::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (experiments_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Track research experiments");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Experiment Tracker");

    int w = width(), h = height();
    drawExperimentCards(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawStatusChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperExperimentTracker::drawExperimentCards(QPainter& p, const QRect& rect) {
    int show = qMin(8, experiments_.size());
    int cardH = qMin(52, (rect.height() - 10) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& e = experiments_[i];
        int y = rect.y() + i * (cardH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(180));
        p.drawRoundedRect(rect.x(), y, rect.width(), cardH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, cardH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.name.left(18));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.status + " | " + e.hypothesis.left(14));

        p.drawText(rect.x() + 10, y + 34, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.startDate.toString("MM/dd") + " - " + e.endDate.toString("MM/dd"));

        // Progress bar
        int barX = rect.x() + rect.width() / 2 + 10;
        int barW = rect.width() / 2 - 25;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(241, 245, 249));
        p.drawRoundedRect(barX, y + 6, barW, 8, 4, 4);
        p.setBrush(e.color);
        p.drawRoundedRect(barX, y + 6, static_cast<int>(barW * e.progress / 100.0), 8, 4, 4);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(barX, y + 18, barW, 16, Qt::AlignVCenter,
                   QString::number(e.progress) + "%");

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(barX, y + 34, barW, 14, Qt::AlignVCenter,
                   "Result: " + e.result);
    }
}

void PaperExperimentTracker::drawStatusChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Status Distribution");

    auto counts = statusCounts();
    QStringList statuses = {"planned", "running", "completed", "failed"};
    QString labels[] = {"Planned", "Running", "Completed", "Failed"};
    QColor colors[] = {QColor(100,116,139), QColor(59,130,246), QColor(16,185,129), QColor(239,68,68)};
    int total = experiments_.size();
    if (total == 0) return;

    int pieW = qMin(rect.width(), rect.height() - 40);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 20 + pieW / 2;

    qreal startAngle = 0;
    for (int i = 0; i < 4; ++i) {
        int count = counts.contains(statuses[i]) ? counts[statuses[i]] : 0;
        qreal span = (static_cast<qreal>(count) / total) * 360;
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

void PaperExperimentTracker::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Experiments", QString::number(experiments_.size()), QColor(59,130,246)},
        {"Running", QString::number(activeExperiments()), QColor(16,185,129)},
        {"Completion", QString::number(completionRate() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Failed", QString::number(statusCounts().value("failed", 0)), QColor(239,68,68)}
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

void PaperExperimentTracker::updateInfo() {
    if (experiments_.isEmpty()) { infoLabel_->setText("Track research experiments"); return; }
    infoLabel_->setText(QString("%1 experiments | %2 running | %3% complete")
        .arg(experiments_.size()).arg(activeExperiments())
        .arg(completionRate() * 100, 0, 'f', 0));
}

void PaperExperimentTracker::loadSettings() {
    int size = settings_.beginReadArray("experiments");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ExperimentEntry e;
        e.id = settings_.value("id").toInt();
        e.name = settings_.value("name").toString();
        e.hypothesis = settings_.value("hypothesis").toString();
        e.status = settings_.value("status").toString();
        e.startDate = QDate::fromString(settings_.value("startDate").toString(), Qt::ISODate);
        e.endDate = QDate::fromString(settings_.value("endDate").toString(), Qt::ISODate);
        e.progress = settings_.value("progress").toDouble();
        e.parameters = settings_.value("parameters").toStringList();
        e.result = settings_.value("result").toString();
        e.color = QColor(settings_.value("color").toString());
        experiments_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperExperimentTracker::saveSettings() {
    settings_.beginWriteArray("experiments");
    for (int i = 0; i < experiments_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", experiments_[i].id);
        settings_.setValue("name", experiments_[i].name);
        settings_.setValue("hypothesis", experiments_[i].hypothesis);
        settings_.setValue("status", experiments_[i].status);
        settings_.setValue("startDate", experiments_[i].startDate.toString(Qt::ISODate));
        settings_.setValue("endDate", experiments_[i].endDate.toString(Qt::ISODate));
        settings_.setValue("progress", experiments_[i].progress);
        settings_.setValue("parameters", experiments_[i].parameters);
        settings_.setValue("result", experiments_[i].result);
        settings_.setValue("color", experiments_[i].color.name());
    }
    settings_.endArray();
}
