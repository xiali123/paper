#include "workspace/PaperCollaborationHub2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperCollaborationHub2::PaperCollaborationHub2(QWidget* parent)
    : QWidget(parent) {
    setupUI();
    loadSettings();
}

void PaperCollaborationHub2::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Research", "Writing", "Review", "Data", "Presentation"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Search member...");
    refreshBtn_ = new QPushButton("Refresh", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Members: 0 | Active: 0 | Avg Contribution: 0.00", this);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(refreshBtn_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);
    connect(refreshBtn_, &QPushButton::clicked, this, &PaperCollaborationHub2::onRefresh);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCollaborationHub2::onClear);
}

void PaperCollaborationHub2::addEntry(const CollaborationHub2Entry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<CollaborationHub2Entry> PaperCollaborationHub2::entries() const { return entries_; }

int PaperCollaborationHub2::activeCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.active) c++;
    return c;
}

qreal PaperCollaborationHub2::avgContribution() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.contribution;
    return sum / entries_.size();
}

QMap<QString, int> PaperCollaborationHub2::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperCollaborationHub2::onRefresh() {
    entries_.clear();
    QList<QColor> palette = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    QStringList categories = {"Research", "Writing", "Review", "Data", "Presentation"};
    QStringList projects = {"Alpha Study", "Beta Analysis", "Gamma Survey", "Delta Experiment", "Epsilon Review", "Zeta Report", "Eta Paper", "Thesis Final"};
    QStringList members = {"Alice", "Bob", "Carol", "Dave", "Eve", "Frank", "Grace", "Hank"};
    for (int i = 0; i < 8; ++i) {
        CollaborationHub2Entry e;
        e.id = i + 1;
        e.project = projects[i];
        e.category = categories[i % categories.size()];
        e.member = members[i];
        e.contribution = QRandomGenerator::global()->bounded(0.1, 1.0);
        e.commits = QRandomGenerator::global()->bounded(5, 150);
        e.active = e.contribution > 0.3;
        e.color = palette[i % palette.size()];
        entries_.append(e);
    }
    updateInfo();
    saveSettings();
    update();
}

void PaperCollaborationHub2::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperCollaborationHub2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawHubView(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryChart(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperCollaborationHub2::drawHubView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Collaboration Hub:");
    int y = rect.top() + 20;
    for (int i = 0; i < qMin(entries_.size(), 8); ++i) {
        const auto& e = entries_[i];
        // Draw avatar circle
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawEllipse(rect.left(), y, 10, 10);
        // Contribution bar
        int barW = static_cast<int>(e.contribution * 60);
        p.setBrush(QColor(0xe2e8f0));
        p.drawRoundedRect(rect.left() + 80, y + 1, 60, 8, 2, 2);
        p.setBrush(e.active ? e.color : QColor(0x94a3b8));
        p.drawRoundedRect(rect.left() + 80, y + 1, barW, 8, 2, 2);
        // Text
        p.setBrush(Qt::NoBrush);
        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + 15, y + 10, QString("%1 | %2 | %3 | %4 commits")
            .arg(e.member.left(12), e.category, e.project.left(15))
            .arg(e.commits));
        // Active indicator
        if (e.active) {
            p.setBrush(QColor(0x16a34a));
            p.setPen(Qt::NoPen);
            p.drawEllipse(rect.left() + 70, y + 3, 5, 5);
        }
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperCollaborationHub2::drawCategoryChart(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts();
    int y = rect.top() + 5;
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Category:");
    y += 18;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    int ci = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        int barWidth = qMin(it.value() * 20, rect.width());
        p.setPen(Qt::NoPen);
        p.setBrush(colors[ci++ % colors.size()]);
        p.drawRoundedRect(rect.left(), y, barWidth, 14, 3, 3);
        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + 4, y + 12, QString("%1: %2").arg(it.key()).arg(it.value()));
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperCollaborationHub2::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Active: %1").arg(activeCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Avg Contribution: %1").arg(QString::number(avgContribution(), 'f', 3)));
    y += 16;
    // Draw pie-style summary circles
    auto counts = categoryCounts();
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    int total = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it) total += it.value();
    if (total > 0) {
        int cx = rect.left() + 40;
        int cy = y + 30;
        int r = 25;
        int startAngle = 0;
        int ci = 0;
        for (auto it = counts.begin(); it != counts.end(); ++it) {
            int span = static_cast<int>(360.0 * it.value() / total * 16);
            p.setPen(Qt::NoPen);
            p.setBrush(colors[ci++ % colors.size()]);
            p.drawPie(cx - r, cy - r, r * 2, r * 2, startAngle, span);
            startAngle += span;
        }
        p.setPen(QColor(0x334155));
        p.setBrush(Qt::NoBrush);
    }
}

void PaperCollaborationHub2::updateInfo() {
    infoLabel_->setText(QString("Members: %1 | Active: %2 | Avg Contribution: %3")
        .arg(entries_.size()).arg(activeCount())
        .arg(QString::number(avgContribution(), 'f', 2)));
}

void PaperCollaborationHub2::loadSettings() {
    settings_.beginGroup("CollaborationHub2");
    int count = settings_.value("count", 0).toInt();
    if (count == 0) {
        // Seed default entries on first load
        settings_.endGroup();
        onRefresh();
        return;
    }
    for (int i = 0; i < count; ++i) {
        CollaborationHub2Entry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.project = settings_.value(QString("project_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.member = settings_.value(QString("member_%1").arg(i)).toString();
        e.contribution = settings_.value(QString("contribution_%1").arg(i)).toDouble();
        e.commits = settings_.value(QString("commits_%1").arg(i)).toInt();
        e.active = settings_.value(QString("active_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperCollaborationHub2::saveSettings() {
    settings_.beginGroup("CollaborationHub2");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("project_%1").arg(i), e.project);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("member_%1").arg(i), e.member);
        settings_.setValue(QString("contribution_%1").arg(i), e.contribution);
        settings_.setValue(QString("commits_%1").arg(i), e.commits);
        settings_.setValue(QString("active_%1").arg(i), e.active);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
