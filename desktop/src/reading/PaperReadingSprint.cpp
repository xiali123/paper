#include "reading/PaperReadingSprint.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReadingSprint::PaperReadingSprint(QWidget* parent) : QWidget(parent) { setupUI(); loadSettings(); }

void PaperReadingSprint::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Skim", "Deep", "Review", "Scan", "Critical"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Paper title...");
    recordBtn_ = new QPushButton("Record", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Sprints: 0 | Completed: 0 | Avg Pace: 0.0", this);
    toolbar->addWidget(categoryCombo_); toolbar->addWidget(inputField_);
    toolbar->addWidget(recordBtn_); toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar); mainLayout->addWidget(infoLabel_);
    connect(recordBtn_, &QPushButton::clicked, this, &PaperReadingSprint::onRecord);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingSprint::onClear);
}

void PaperReadingSprint::addEntry(const SprintEntry& entry) { entries_.append(entry); updateInfo(); update(); }
QList<SprintEntry> PaperReadingSprint::entries() const { return entries_; }

int PaperReadingSprint::completedCount() const { int c = 0; for (const auto& e : entries_) if (e.completed) c++; return c; }

qreal PaperReadingSprint::avgPace() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0; for (const auto& e : entries_) sum += e.pace; return sum / entries_.size();
}

QMap<QString, int> PaperReadingSprint::categoryCounts() const { QMap<QString, int> m; for (const auto& e : entries_) m[e.category]++; return m; }

void PaperReadingSprint::onRecord() {
    SprintEntry e;
    e.id = entries_.size() + 1;
    e.paper = inputField_->text().trimmed();
    if (e.paper.isEmpty()) e.paper = QString("Paper_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    QStringList phases = {"Start", "Middle", "End", "Review", "Notes"};
    e.phase = phases[QRandomGenerator::global()->bounded(phases.size())];
    e.pace = QRandomGenerator::global()->bounded(1.0, 50.0);
    e.pages = QRandomGenerator::global()->bounded(1.0, 30.0);
    e.focus = QRandomGenerator::global()->bounded(0.0, 1.0);
    e.completed = e.focus > 0.5;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    updateInfo(); saveSettings();
    emit sprintRecorded(e.id, e.pace);
    update();
}

void PaperReadingSprint::onClear() { entries_.clear(); updateInfo(); saveSettings(); update(); }

void PaperReadingSprint::paintEvent(QPaintEvent*) {
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawSprintView(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryChart(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperReadingSprint::drawSprintView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155)); p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Reading Sprints:");
    if (entries_.isEmpty()) return;
    int x = rect.left(); int baseY = rect.top() + rect.height() / 2 + 10;
    p.setPen(QPen(QColor(0x94a3b8), 1, Qt::DashLine));
    p.drawLine(rect.left(), baseY, rect.right(), baseY);
    int stepW = qMax(8, (rect.width() - 20) / qMax(entries_.size(), 1));
    for (int i = 0; i < qMin(entries_.size(), 15); ++i) {
        const auto& e = entries_[i];
        int barH = static_cast<int>(e.pace * 3);
        QColor c = e.completed ? QColor(0x16a34a) : QColor(0xd97706);
        p.setBrush(c); p.setPen(Qt::NoPen);
        p.drawRoundedRect(x, baseY - barH, stepW - 2, barH, 2, 2);
        p.setPen(QColor(0x334155));
        p.drawText(x, baseY + 12, QString::number(static_cast<int>(e.pages)));
        x += stepW;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperReadingSprint::drawCategoryChart(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts(); int y = rect.top() + 5;
    p.setPen(QColor(0x334155)); p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Mode:"); y += 18;
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

void PaperReadingSprint::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155)); p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total: %1").arg(entries_.size())); y += 16;
    p.drawText(rect.left(), y, QString("Completed: %1").arg(completedCount())); y += 16;
    p.drawText(rect.left(), y, QString("Avg Pace: %1 pg/h").arg(QString::number(avgPace(), 'f', 1)));
}

void PaperReadingSprint::updateInfo() {
    infoLabel_->setText(QString("Sprints: %1 | Completed: %2 | Avg Pace: %3")
        .arg(entries_.size()).arg(completedCount()).arg(QString::number(avgPace(), 'f', 1)));
}

void PaperReadingSprint::loadSettings() {
    settings_.beginGroup("ReadingSprint");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        SprintEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.paper = settings_.value(QString("paper_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.phase = settings_.value(QString("phase_%1").arg(i)).toString();
        e.pace = settings_.value(QString("pace_%1").arg(i)).toDouble();
        e.pages = settings_.value(QString("pages_%1").arg(i)).toDouble();
        e.focus = settings_.value(QString("focus_%1").arg(i)).toDouble();
        e.completed = settings_.value(QString("completed_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup(); updateInfo();
}

void PaperReadingSprint::saveSettings() {
    settings_.beginGroup("ReadingSprint"); settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("paper_%1").arg(i), e.paper);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("phase_%1").arg(i), e.phase);
        settings_.setValue(QString("pace_%1").arg(i), e.pace);
        settings_.setValue(QString("pages_%1").arg(i), e.pages);
        settings_.setValue(QString("focus_%1").arg(i), e.focus);
        settings_.setValue(QString("completed_%1").arg(i), e.completed);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
