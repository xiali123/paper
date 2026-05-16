#include "reading/PaperReadingDashboard.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReadingDashboard::PaperReadingDashboard(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingDashboard")
{
    setupUI();
    loadSettings();
}

void PaperReadingDashboard::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    updateBtn_ = new QPushButton("Update");
    updateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(updateBtn_, &QPushButton::clicked, this, &PaperReadingDashboard::onUpdate);
    toolbar->addWidget(updateBtn_);
    toolbar->addWidget(new QLabel("Period:"));
    periodCombo_ = new QComboBox();
    periodCombo_->addItems({"Daily", "Weekly", "Monthly", "Yearly"});
    toolbar->addWidget(periodCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingDashboard::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter user name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Reading dashboard");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperReadingDashboard::addEntry(const DashboardEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit dashboardUpdated(entry.id, entry.completionRate);
    update();
}

QList<DashboardEntry> PaperReadingDashboard::entries() const { return entries_; }

qreal PaperReadingDashboard::avgCompletion() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.completionRate;
    return sum / entries_.size();
}

int PaperReadingDashboard::onTrackCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.onTrack) c++;
    return c;
}

QMap<QString, int> PaperReadingDashboard::periodCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.period]++;
    return counts;
}

void PaperReadingDashboard::onUpdate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList periods = {"daily", "weekly", "monthly", "yearly"};
    QStringList categories = {"ML", "NLP", "CV", "Theory", "Applied"};
    int pIdx = periodCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        DashboardEntry e;
        e.id = entries_.size() + 1;
        e.userName = text.left(10) + " user" + QString::number(i);
        e.papersTotal = 20 + QRandomGenerator::global()->bounded(100);
        e.papersRead = QRandomGenerator::global()->bounded(e.papersTotal + 1);
        e.completionRate = static_cast<qreal>(e.papersRead) / e.papersTotal;
        e.period = periods[pIdx];
        e.streakDays = QRandomGenerator::global()->bounded(30);
        e.avgSpeed = 0.5 + QRandomGenerator::global()->bounded(50) / 10.0;
        e.topCategory = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.onTrack = e.completionRate >= 0.6;
        e.color = e.onTrack ? QColor(16,185,129) : (e.completionRate >= 0.3 ? QColor(245,158,11) : QColor(239,68,68));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperReadingDashboard::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Reading dashboard");
    update();
}

void PaperReadingDashboard::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Reading dashboard");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Dashboard");
    int w = width(), h = height();
    drawDashboardList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawPeriodChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReadingDashboard::drawDashboardList(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(34, (rect.height() - 10) / qMax(show, 1));
    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 3);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.userName.left(12) + (e.onTrack ? " [ON]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   QString::number(e.papersRead) + "/" + QString::number(e.papersTotal) + " | " + e.topCategory);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.completionRate * 100, 'f', 0) + "%");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.streakDays) + "d | " + QString::number(e.avgSpeed, 'f', 1) + "/day");
    }
}

void PaperReadingDashboard::drawPeriodChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Periods");
    auto counts = periodCounts();
    QStringList periods = {"daily", "weekly", "monthly", "yearly"};
    QString labels[] = {"Daily", "Weekly", "Monthly", "Yearly"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(periods[i]) ? counts[periods[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 65, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 70, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 73 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperReadingDashboard::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Users", QString::number(entries_.size()), QColor(59,130,246)},
        {"On Track", QString::number(onTrackCount()), QColor(16,185,129)},
        {"Avg Completion", QString::number(avgCompletion() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Periods", QString::number(periodCounts().size()), QColor(139,92,246)}
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

void PaperReadingDashboard::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Reading dashboard"); return; }
    infoLabel_->setText(QString("%1 users | %2 on track | %3% avg")
        .arg(entries_.size()).arg(onTrackCount()).arg(avgCompletion() * 100, 0, 'f', 0));
}

void PaperReadingDashboard::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        DashboardEntry e;
        e.id = settings_.value("id").toInt();
        e.userName = settings_.value("userName").toString();
        e.papersRead = settings_.value("papersRead").toInt();
        e.papersTotal = settings_.value("papersTotal").toInt();
        e.completionRate = settings_.value("completionRate").toDouble();
        e.period = settings_.value("period").toString();
        e.streakDays = settings_.value("streakDays").toInt();
        e.avgSpeed = settings_.value("avgSpeed").toDouble();
        e.topCategory = settings_.value("topCategory").toString();
        e.onTrack = settings_.value("onTrack").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingDashboard::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("userName", entries_[i].userName);
        settings_.setValue("papersRead", entries_[i].papersRead);
        settings_.setValue("papersTotal", entries_[i].papersTotal);
        settings_.setValue("completionRate", entries_[i].completionRate);
        settings_.setValue("period", entries_[i].period);
        settings_.setValue("streakDays", entries_[i].streakDays);
        settings_.setValue("avgSpeed", entries_[i].avgSpeed);
        settings_.setValue("topCategory", entries_[i].topCategory);
        settings_.setValue("onTrack", entries_[i].onTrack);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
