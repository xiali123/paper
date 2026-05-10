#include "workspace/PaperTeamAvailability.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperTeamAvailability::PaperTeamAvailability(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "TeamAvailability")
{
    setupUI();
    loadSettings();
}

void PaperTeamAvailability::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    refreshBtn_ = new QPushButton("Refresh");
    refreshBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(refreshBtn_, &QPushButton::clicked, this, &PaperTeamAvailability::onRefresh);
    toolbar->addWidget(refreshBtn_);

    toolbar->addWidget(new QLabel("Role:"));
    roleCombo_ = new QComboBox();
    roleCombo_->addItems({"All", "Researcher", "Reviewer", "Writer", "Admin"});
    toolbar->addWidget(roleCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperTeamAvailability::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter team member or project name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Track team availability");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperTeamAvailability::addEntry(const TeamAvailEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit availabilityUpdated(entry.id, entry.availability);
    update();
}

QList<TeamAvailEntry> PaperTeamAvailability::entries() const { return entries_; }

qreal PaperTeamAvailability::avgAvailability() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.availability;
    return sum / entries_.size();
}

int PaperTeamAvailability::availableCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.availability >= 0.5) c++;
    return c;
}

QMap<QString, int> PaperTeamAvailability::roleCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.role]++;
    return counts;
}

void PaperTeamAvailability::onRefresh() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList roles = {"researcher", "reviewer", "writer", "admin"};
    QStringList specs = {"ML", "NLP", "CV", "Security", "Theory"};
    QStringList statuses = {"available", "busy", "offline"};
    QStringList names = {"Alice", "Bob", "Charlie", "Diana", "Eve", "Frank"};

    int rIdx = roleCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        TeamAvailEntry e;
        e.id = entries_.size() + 1;
        e.member = names[QRandomGenerator::global()->bounded(names.size())];
        e.role = rIdx == 0 ? roles[QRandomGenerator::global()->bounded(roles.size())] : roles[rIdx - 1];
        e.availability = QRandomGenerator::global()->bounded(101) / 100.0;
        e.tasksActive = QRandomGenerator::global()->bounded(8);
        e.tasksCompleted = 5 + QRandomGenerator::global()->bounded(30);
        e.specialization = specs[QRandomGenerator::global()->bounded(specs.size())];
        e.workload = e.tasksActive / 8.0;
        e.status = e.availability >= 0.5 ? "available" : (e.availability >= 0.2 ? "busy" : "offline");
        e.capacity = 10 + QRandomGenerator::global()->bounded(20);
        e.color = e.status == "available" ? QColor(16,185,129) : (e.status == "busy" ? QColor(245,158,11) : QColor(239,68,68));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperTeamAvailability::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Track team availability");
    update();
}

void PaperTeamAvailability::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Track team availability");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Team Availability");

    int w = width(), h = height();
    drawMemberList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawRoleChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperTeamAvailability::drawMemberList(QPainter& p, const QRect& rect) {
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
                   e.member + " [" + e.role.left(5) + "]");

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.specialization + " | " + e.status);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.availability * 100, 'f', 0) + "% avail");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.tasksActive) + "/" + QString::number(e.tasksCompleted) + " tasks");
    }
}

void PaperTeamAvailability::drawRoleChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Roles");

    auto counts = roleCounts();
    QStringList roles = {"researcher", "reviewer", "writer", "admin"};
    QString labels[] = {"Research", "Reviewer", "Writer", "Admin"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(28, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 4);
        int count = counts.contains(roles[i]) ? counts[roles[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x(), y + barH - 3, 60, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 65, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 68 + barW, y + barH - 3, QString::number(count));
    }
}

void PaperTeamAvailability::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Members", QString::number(entries_.size()), QColor(59,130,246)},
        {"Available", QString::number(availableCount()), QColor(16,185,129)},
        {"Avg Avail", QString::number(avgAvailability() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Roles", QString::number(roleCounts().size()), QColor(139,92,246)}
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

void PaperTeamAvailability::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Track team availability"); return; }
    infoLabel_->setText(QString("%1 members | %2 avail | %3% avg")
        .arg(entries_.size()).arg(availableCount()).arg(avgAvailability() * 100, 0, 'f', 0));
}

void PaperTeamAvailability::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        TeamAvailEntry e;
        e.id = settings_.value("id").toInt();
        e.member = settings_.value("member").toString();
        e.role = settings_.value("role").toString();
        e.availability = settings_.value("availability").toDouble();
        e.tasksActive = settings_.value("tasksActive").toInt();
        e.tasksCompleted = settings_.value("tasksCompleted").toInt();
        e.specialization = settings_.value("specialization").toString();
        e.workload = settings_.value("workload").toDouble();
        e.status = settings_.value("status").toString();
        e.capacity = settings_.value("capacity").toInt();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperTeamAvailability::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("member", entries_[i].member);
        settings_.setValue("role", entries_[i].role);
        settings_.setValue("availability", entries_[i].availability);
        settings_.setValue("tasksActive", entries_[i].tasksActive);
        settings_.setValue("tasksCompleted", entries_[i].tasksCompleted);
        settings_.setValue("specialization", entries_[i].specialization);
        settings_.setValue("workload", entries_[i].workload);
        settings_.setValue("status", entries_[i].status);
        settings_.setValue("capacity", entries_[i].capacity);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
