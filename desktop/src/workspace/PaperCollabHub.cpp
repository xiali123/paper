#include "workspace/PaperCollabHub.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperCollabHub::PaperCollabHub(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CollabHub")
{
    setupUI();
    loadSettings();
}

void PaperCollabHub::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    toolbar->addWidget(addBtn_);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"Research", "Writing", "Review", "Analysis", "Outreach"});
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Member name...");
    toolbar->addWidget(inputField_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("QPushButton { color: #dc2626; padding: 4px 12px; border-radius: 4px; }");
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Add members to collaboration hub");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 450);

    connect(addBtn_, &QPushButton::clicked, this, &PaperCollabHub::onAdd);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCollabHub::onClear);
}

void PaperCollabHub::addEntry(const CollabEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit memberAdded(entry.id, entry.contribution);
    update();
}

QList<CollabEntry> PaperCollabHub::entries() const { return entries_; }

int PaperCollabHub::leadCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.lead) c++;
    return c;
}

qreal PaperCollabHub::avgContribution() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.contribution;
    return sum / entries_.size();
}

QMap<QString, int> PaperCollabHub::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperCollabHub::onAdd() {
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    QStringList members = {"Alice", "Bob", "Carol", "Dave", "Eve", "Frank", "Grace", "Hank"};
    QStringList roles = {"Author", "Reviewer", "Editor", "Contributor", "Advisor"};
    int count = 3 + QRandomGenerator::global()->bounded(4);

    for (int i = 0; i < count; ++i) {
        CollabEntry e;
        e.id = entries_.size() + 1;
        e.member = members[QRandomGenerator::global()->bounded(members.size())];
        e.category = categoryCombo_->currentText();
        e.role = roles[QRandomGenerator::global()->bounded(roles.size())];
        e.contribution = QRandomGenerator::global()->bounded(0.1, 1.0);
        e.papers = QRandomGenerator::global()->bounded(1, 20);
        e.lead = QRandomGenerator::global()->bounded(100) < 25;
        e.color = colors[e.id % colors.size()];
        entries_.append(e);
        emit memberAdded(e.id, e.contribution);
    }
    saveSettings();
    updateInfo();
    update();
}

void PaperCollabHub::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Add members to collaboration hub");
    update();
}

void PaperCollabHub::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(0xf8fafc));

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Sans", 12));
        p.drawText(rect(), Qt::AlignCenter, "Add members to collaboration hub");
        return;
    }

    int w = width(), h = height();
    drawHubView(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryChart(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperCollabHub::drawHubView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Collaboration Hub:");
    int y = rect.top() + 20;
    int maxShow = qMin(entries_.size(), 8);
    for (int i = 0; i < maxShow; ++i) {
        const auto& e = entries_[i];
        int barW = static_cast<int>(e.contribution * 80);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0xe2e8f0));
        p.drawRoundedRect(rect.left(), y + 2, 80, 10, 2, 2);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.left(), y + 2, barW, 10, 2, 2);
        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + 86, y + 11, QString("%1 | %2 | %3 | Papers: %4%5")
            .arg(e.member, e.category, e.role)
            .arg(e.papers)
            .arg(e.lead ? " [Lead]" : ""));
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperCollabHub::drawCategoryChart(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts();
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Category:");
    int y = rect.top() + 18;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    int ci = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        p.setPen(Qt::NoPen);
        p.setBrush(colors[ci++ % colors.size()]);
        p.drawRoundedRect(rect.left(), y, qMin(it.value() * 20, rect.width()), 14, 3, 3);
        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + 4, y + 12, QString("%1: %2").arg(it.key()).arg(it.value()));
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperCollabHub::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Members: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Leads: %1").arg(leadCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Avg Contribution: %1%").arg(static_cast<int>(avgContribution() * 100)));
    y += 16;
    int totalPapers = 0;
    for (const auto& e : entries_) totalPapers += e.papers;
    p.drawText(rect.left(), y, QString("Total Papers: %1").arg(totalPapers));
}

void PaperCollabHub::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Add members to collaboration hub"); return; }
    int totalPapers = 0;
    for (const auto& e : entries_) totalPapers += e.papers;
    infoLabel_->setText(QString("Members: %1 | Leads: %2 | Avg: %3% | Papers: %4")
        .arg(entries_.size()).arg(leadCount())
        .arg(static_cast<int>(avgContribution() * 100))
        .arg(totalPapers));
}

void PaperCollabHub::loadSettings() {
    settings_.beginGroup("CollabHub");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        CollabEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.member = settings_.value(QString("member_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.role = settings_.value(QString("role_%1").arg(i)).toString();
        e.contribution = settings_.value(QString("contribution_%1").arg(i)).toDouble();
        e.papers = settings_.value(QString("papers_%1").arg(i)).toInt();
        e.lead = settings_.value(QString("lead_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperCollabHub::saveSettings() {
    settings_.beginGroup("CollabHub");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("member_%1").arg(i), e.member);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("role_%1").arg(i), e.role);
        settings_.setValue(QString("contribution_%1").arg(i), e.contribution);
        settings_.setValue(QString("papers_%1").arg(i), e.papers);
        settings_.setValue(QString("lead_%1").arg(i), e.lead);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
