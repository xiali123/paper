#include "workspace/PaperProjectBoard.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperProjectBoard::PaperProjectBoard(QWidget* parent)
    : QWidget(parent) {
    setupUI();
    loadSettings();
}

void PaperProjectBoard::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Research", "Writing", "Review", "Submission", "Revision"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Project name...");
    updateBtn_ = new QPushButton("Update", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Projects: 0 | On Track: 0 | Avg Completion: 0%", this);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(updateBtn_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);
    connect(updateBtn_, &QPushButton::clicked, this, &PaperProjectBoard::onUpdate);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperProjectBoard::onClear);
}

void PaperProjectBoard::addEntry(const BoardEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<BoardEntry> PaperProjectBoard::entries() const { return entries_; }

int PaperProjectBoard::onTrackCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.onTrack) c++;
    return c;
}

qreal PaperProjectBoard::avgCompletion() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.completion;
    return sum / entries_.size();
}

QMap<QString, int> PaperProjectBoard::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperProjectBoard::onUpdate() {
    BoardEntry e;
    e.id = entries_.size() + 1;
    e.project = inputField_->text().trimmed();
    if (e.project.isEmpty()) e.project = QString("Project_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    QStringList statuses = {"Backlog", "In Progress", "Review", "Done", "Blocked"};
    e.status = statuses[QRandomGenerator::global()->bounded(statuses.size())];
    e.tasks = QRandomGenerator::global()->bounded(5, 50);
    e.completion = QRandomGenerator::global()->bounded(0.0, 1.0);
    e.deadline = QString("2026-%1-%2")
        .arg(QRandomGenerator::global()->bounded(1, 13), 2, 10, QChar('0'))
        .arg(QRandomGenerator::global()->bounded(1, 29), 2, 10, QChar('0'));
    e.onTrack = e.completion > 0.5;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    updateInfo();
    saveSettings();
    emit boardUpdated(e.id, e.completion);
    update();
}

void PaperProjectBoard::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperProjectBoard::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawBoardList(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryChart(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperProjectBoard::drawBoardList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Project Board:");
    int y = rect.top() + 20;
    for (int i = 0; i < qMin(entries_.size(), 8); ++i) {
        const auto& e = entries_[i];
        // Draw progress bar
        int barW = static_cast<int>(e.completion * (rect.width() - 200));
        p.setBrush(QColor(0xe2e8f0));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(rect.left(), y, rect.width() - 200, 12, 3, 3);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.left(), y, barW, 12, 3, 3);
        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + rect.width() - 195, y + 11, QString("%1 | %2 | %3% | %4")
            .arg(e.project.left(12), e.status)
            .arg(static_cast<int>(e.completion * 100))
            .arg(e.deadline));
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperProjectBoard::drawCategoryChart(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts();
    int y = rect.top() + 5;
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Phase:");
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

void PaperProjectBoard::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("On Track: %1").arg(onTrackCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Avg Completion: %1%").arg(static_cast<int>(avgCompletion() * 100)));
}

void PaperProjectBoard::updateInfo() {
    infoLabel_->setText(QString("Projects: %1 | On Track: %2 | Avg Completion: %3%")
        .arg(entries_.size()).arg(onTrackCount())
        .arg(static_cast<int>(avgCompletion() * 100)));
}

void PaperProjectBoard::loadSettings() {
    settings_.beginGroup("ProjectBoard");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        BoardEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.project = settings_.value(QString("project_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.status = settings_.value(QString("status_%1").arg(i)).toString();
        e.tasks = settings_.value(QString("tasks_%1").arg(i)).toInt();
        e.completion = settings_.value(QString("completion_%1").arg(i)).toDouble();
        e.deadline = settings_.value(QString("deadline_%1").arg(i)).toString();
        e.onTrack = settings_.value(QString("onTrack_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperProjectBoard::saveSettings() {
    settings_.beginGroup("ProjectBoard");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("project_%1").arg(i), e.project);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("status_%1").arg(i), e.status);
        settings_.setValue(QString("tasks_%1").arg(i), e.tasks);
        settings_.setValue(QString("completion_%1").arg(i), e.completion);
        settings_.setValue(QString("deadline_%1").arg(i), e.deadline);
        settings_.setValue(QString("onTrack_%1").arg(i), e.onTrack);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
