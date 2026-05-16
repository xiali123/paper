#include "workspace/PaperCollaborationHub.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperCollaborationHub::PaperCollaborationHub(QWidget* parent)
    : QWidget(parent) {
    setupUI();
    loadSettings();
}

void PaperCollaborationHub::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Author", "Reviewer", "Advisor", "Student", "Collaborator"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Member name...");
    addBtn_ = new QPushButton("Add", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Members: 0 | Active: 0 | Avg Activity: 0.00", this);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(addBtn_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);
    connect(addBtn_, &QPushButton::clicked, this, &PaperCollaborationHub::onAdd);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCollaborationHub::onClear);
}

void PaperCollaborationHub::addEntry(const CollabEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<CollabEntry> PaperCollaborationHub::entries() const { return entries_; }

int PaperCollaborationHub::activeCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.active) c++;
    return c;
}

qreal PaperCollaborationHub::avgActivity() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.activity;
    return sum / entries_.size();
}

QMap<QString, int> PaperCollaborationHub::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperCollaborationHub::onAdd() {
    CollabEntry e;
    e.id = entries_.size() + 1;
    e.member = inputField_->text().trimmed();
    if (e.member.isEmpty()) e.member = QString("Member_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    QStringList roles = {"Lead", "Contributor", "Reviewer", "Mentor", "Observer"};
    e.role = roles[QRandomGenerator::global()->bounded(roles.size())];
    e.contributions = QRandomGenerator::global()->bounded(1, 100);
    e.activity = QRandomGenerator::global()->bounded(0.0, 1.0);
    e.joined = QString("2026-%1-%2")
        .arg(QRandomGenerator::global()->bounded(1, 6), 2, 10, QChar('0'))
        .arg(QRandomGenerator::global()->bounded(1, 29), 2, 10, QChar('0'));
    e.active = e.activity > 0.3;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    updateInfo();
    saveSettings();
    emit memberAdded(e.id, e.activity);
    update();
}

void PaperCollaborationHub::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperCollaborationHub::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawMemberList(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryChart(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperCollaborationHub::drawMemberList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Collaboration Hub:");
    int y = rect.top() + 20;
    for (int i = 0; i < qMin(entries_.size(), 8); ++i) {
        const auto& e = entries_[i];
        // Draw avatar circle
        p.setPen(e.color);
        p.setBrush(e.color);
        p.drawEllipse(rect.left(), y, 10, 10);
        p.setBrush(Qt::NoBrush);
        p.setPen(QColor(0x334155));
        // Activity bar
        int barW = static_cast<int>(e.activity * 60);
        p.setBrush(QColor(0xe2e8f0));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(rect.left() + 80, y + 1, 60, 8, 2, 2);
        p.setBrush(e.active ? QColor(0x16a34a) : QColor(0x94a3b8));
        p.drawRoundedRect(rect.left() + 80, y + 1, barW, 8, 2, 2);
        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + 15, y + 10, QString("%1 | %2 | %3 contrib")
            .arg(e.member.left(12), e.role)
            .arg(e.contributions));
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperCollaborationHub::drawCategoryChart(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts();
    int y = rect.top() + 5;
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Role:");
    y += 18;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    int ci = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        p.setBrush(colors[ci++ % colors.size()]);
        p.drawRoundedRect(rect.left(), y, qMin(it.value() * 20, rect.width()), 14, 3, 3);
        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + 4, y + 12, QString("%1: %2").arg(it.key()).arg(it.value()));
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperCollaborationHub::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Active: %1").arg(activeCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Avg Activity: %1").arg(QString::number(avgActivity(), 'f', 3)));
}

void PaperCollaborationHub::updateInfo() {
    infoLabel_->setText(QString("Members: %1 | Active: %2 | Avg Activity: %3")
        .arg(entries_.size()).arg(activeCount())
        .arg(QString::number(avgActivity(), 'f', 2)));
}

void PaperCollaborationHub::loadSettings() {
    settings_.beginGroup("CollaborationHub");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        CollabEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.member = settings_.value(QString("member_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.role = settings_.value(QString("role_%1").arg(i)).toString();
        e.contributions = settings_.value(QString("contributions_%1").arg(i)).toInt();
        e.activity = settings_.value(QString("activity_%1").arg(i)).toDouble();
        e.joined = settings_.value(QString("joined_%1").arg(i)).toString();
        e.active = settings_.value(QString("active_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperCollaborationHub::saveSettings() {
    settings_.beginGroup("CollaborationHub");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("member_%1").arg(i), e.member);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("role_%1").arg(i), e.role);
        settings_.setValue(QString("contributions_%1").arg(i), e.contributions);
        settings_.setValue(QString("activity_%1").arg(i), e.activity);
        settings_.setValue(QString("joined_%1").arg(i), e.joined);
        settings_.setValue(QString("active_%1").arg(i), e.active);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
