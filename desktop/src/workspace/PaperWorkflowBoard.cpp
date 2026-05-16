#include "workspace/PaperWorkflowBoard.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperWorkflowBoard::PaperWorkflowBoard(QWidget* parent) : QWidget(parent) { setupUI(); loadSettings(); }

void PaperWorkflowBoard::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Draft", "Review", "Revision", "Final", "Published"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Stage name...");
    updateBtn_ = new QPushButton("Update", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Stages: 0 | Active: 0 | Avg Throughput: 0.0", this);
    toolbar->addWidget(categoryCombo_); toolbar->addWidget(inputField_);
    toolbar->addWidget(updateBtn_); toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar); mainLayout->addWidget(infoLabel_);
    connect(updateBtn_, &QPushButton::clicked, this, &PaperWorkflowBoard::onUpdate);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperWorkflowBoard::onClear);
}

void PaperWorkflowBoard::addEntry(const WorkflowEntry& entry) { entries_.append(entry); updateInfo(); update(); }
QList<WorkflowEntry> PaperWorkflowBoard::entries() const { return entries_; }
int PaperWorkflowBoard::activeCount() const { int c = 0; for (const auto& e : entries_) if (e.active) c++; return c; }
qreal PaperWorkflowBoard::avgThroughput() const { if (entries_.isEmpty()) return 0.0; qreal s = 0; for (const auto& e : entries_) s += e.throughput; return s / entries_.size(); }
QMap<QString, int> PaperWorkflowBoard::categoryCounts() const { QMap<QString, int> m; for (const auto& e : entries_) m[e.category]++; return m; }

void PaperWorkflowBoard::onUpdate() {
    WorkflowEntry e;
    e.id = entries_.size() + 1;
    e.stage = inputField_->text().trimmed();
    if (e.stage.isEmpty()) e.stage = QString("Stage_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    QStringList statuses = {"Pending", "In Progress", "Blocked", "Done", "Archived"};
    e.status = statuses[QRandomGenerator::global()->bounded(statuses.size())];
    e.items = QRandomGenerator::global()->bounded(0, 50);
    e.throughput = QRandomGenerator::global()->bounded(0.0, 20.0);
    QStringList owners = {"Alice", "Bob", "Carol", "Dave", "Team"};
    e.owner = owners[QRandomGenerator::global()->bounded(owners.size())];
    e.active = e.status == "In Progress";
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e); updateInfo(); saveSettings();
    emit workflowUpdated(e.id, e.throughput); update();
}

void PaperWorkflowBoard::onClear() { entries_.clear(); updateInfo(); saveSettings(); update(); }

void PaperWorkflowBoard::paintEvent(QPaintEvent*) {
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(0xf8fafc));
    drawWorkflowView(p, QRect(10, 50, width() - 20, height() / 2 - 60));
    drawCategoryChart(p, QRect(10, height() / 2, width() / 2 - 10, height() / 2 - 60));
    drawStats(p, QRect(width() / 2 + 10, height() / 2, width() / 2 - 20, height() / 2 - 60));
}

void PaperWorkflowBoard::drawWorkflowView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155)); p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Workflow Board:");
    int y = rect.top() + 20;
    for (int i = 0; i < qMin(entries_.size(), 8); ++i) {
        const auto& e = entries_[i];
        int cardW = (rect.width() - 40) / qMin(entries_.size(), 5);
        int x = rect.left() + (i % 5) * (cardW + 8);
        int row = i / 5;
        int cy = y + row * 70;
        // Card
        QColor bg = e.active ? QColor(0xe0f2fe) : QColor(0xf1f5f9);
        p.setBrush(bg); p.setPen(e.color);
        p.drawRoundedRect(x, cy, cardW, 55, 4, 4);
        p.setPen(QColor(0x334155)); p.setFont(QFont("Sans", 8));
        p.drawText(x + 4, cy + 12, e.stage.left(10));
        p.drawText(x + 4, cy + 24, QString("%1 | %2").arg(e.status, e.owner));
        p.drawText(x + 4, cy + 36, QString("Items: %1 | TP: %2").arg(e.items).arg(QString::number(e.throughput, 'f', 1)));
    }
    p.setBrush(Qt::NoBrush);
}

void PaperWorkflowBoard::drawCategoryChart(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts(); int y = rect.top() + 5;
    p.setPen(QColor(0x334155)); p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Phase:"); y += 18;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    int ci = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        p.setBrush(colors[ci++ % colors.size()]);
        p.drawRoundedRect(rect.left(), y, qMin(it.value() * 20, rect.width()), 14, 3, 3);
        p.setPen(QColor(0x334155)); p.drawText(rect.left() + 4, y + 12, QString("%1: %2").arg(it.key()).arg(it.value())); y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperWorkflowBoard::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155)); p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Stages: %1").arg(entries_.size())); y += 16;
    p.drawText(rect.left(), y, QString("Active: %1").arg(activeCount())); y += 16;
    p.drawText(rect.left(), y, QString("Avg Throughput: %1").arg(QString::number(avgThroughput(), 'f', 1)));
}

void PaperWorkflowBoard::updateInfo() {
    infoLabel_->setText(QString("Stages: %1 | Active: %2 | Avg Throughput: %3")
        .arg(entries_.size()).arg(activeCount()).arg(QString::number(avgThroughput(), 'f', 1)));
}

void PaperWorkflowBoard::loadSettings() {
    settings_.beginGroup("WorkflowBoard");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        WorkflowEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.stage = settings_.value(QString("stage_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.status = settings_.value(QString("status_%1").arg(i)).toString();
        e.items = settings_.value(QString("items_%1").arg(i)).toInt();
        e.throughput = settings_.value(QString("throughput_%1").arg(i)).toDouble();
        e.owner = settings_.value(QString("owner_%1").arg(i)).toString();
        e.active = settings_.value(QString("active_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup(); updateInfo();
}

void PaperWorkflowBoard::saveSettings() {
    settings_.beginGroup("WorkflowBoard"); settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("stage_%1").arg(i), e.stage);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("status_%1").arg(i), e.status);
        settings_.setValue(QString("items_%1").arg(i), e.items);
        settings_.setValue(QString("throughput_%1").arg(i), e.throughput);
        settings_.setValue(QString("owner_%1").arg(i), e.owner);
        settings_.setValue(QString("active_%1").arg(i), e.active);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
