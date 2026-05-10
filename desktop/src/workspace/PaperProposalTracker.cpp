#include "workspace/PaperProposalTracker.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperProposalTracker::PaperProposalTracker(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ProposalTracker")
{
    setupUI();
    loadSettings();
}

void PaperProposalTracker::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Proposal");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperProposalTracker::onAdd);
    toolbar->addWidget(addBtn_);

    toolbar->addWidget(new QLabel("Status:"));
    statusCombo_ = new QComboBox();
    statusCombo_->addItems({"All", "Draft", "Submitted", "Approved", "Rejected"});
    toolbar->addWidget(statusCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperProposalTracker::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter proposal title...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Track research proposals");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperProposalTracker::addEntry(const ProposalEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit proposalTracked(entry.id, entry.amount);
    update();
}

QList<ProposalEntry> PaperProposalTracker::entries() const { return entries_; }

qreal PaperProposalTracker::totalAmount() const {
    qreal t = 0;
    for (const auto& e : entries_) t += e.amount;
    return t;
}

int PaperProposalTracker::submittedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.submitted) c++;
    return c;
}

QMap<QString, int> PaperProposalTracker::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperProposalTracker::onAdd() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList statuses = {"draft", "submitted", "approved", "rejected"};
    QStringList funders = {"NSF", "NIH", "DARPA", "EU Horizon", "SRC"};
    QStringList categories = {"basic", "applied", "translational", "exploratory"};
    QStringList deadlines = {"2026-06-01", "2026-09-15", "2027-01-01", "2027-04-01"};

    int sIdx = statusCombo_->currentIndex();
    ProposalEntry e;
    e.id = entries_.size() + 1;
    e.title = text.left(16);
    e.status = sIdx == 0 ? statuses[QRandomGenerator::global()->bounded(statuses.size())] : statuses[sIdx - 1];
    e.funder = funders[QRandomGenerator::global()->bounded(funders.size())];
    e.amount = 5000 + QRandomGenerator::global()->bounded(200000);
    e.deadline = deadlines[QRandomGenerator::global()->bounded(deadlines.size())];
    e.score = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
    e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
    e.papersPlanned = 1 + QRandomGenerator::global()->bounded(10);
    e.submitted = e.status != "draft";
    e.color = e.status == "approved" ? QColor(16,185,129) :
              (e.status == "rejected" ? QColor(239,68,68) :
              (e.status == "submitted" ? QColor(59,130,246) : QColor(245,158,11)));
    addEntry(e);
    inputField_->clear();
}

void PaperProposalTracker::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Track research proposals");
    update();
}

void PaperProposalTracker::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Track research proposals");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Proposal Tracker");

    int w = width(), h = height();
    drawProposalList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperProposalTracker::drawProposalList(QPainter& p, const QRect& rect) {
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
                   e.title.left(14));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.funder + " | " + e.deadline + " | " + QString::number(e.papersPlanned) + " papers");
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "$" + QString::number(static_cast<int>(e.amount)));
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.status + " | score:" + QString::number(e.score * 100, 'f', 0) + "%");
    }
}

void PaperProposalTracker::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Status");

    QMap<QString, int> statusMap;
    for (const auto& e : entries_) statusMap[e.status]++;
    QStringList statuses = {"draft", "submitted", "approved", "rejected"};
    QString labels[] = {"Draft", "Submitted", "Approved", "Rejected"};
    QColor colors[] = {QColor(245,158,11), QColor(59,130,246), QColor(16,185,129), QColor(239,68,68)};

    int maxVal = 1;
    for (const auto& v : statusMap) maxVal = qMax(maxVal, v);

    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = statusMap.contains(statuses[i]) ? statusMap[statuses[i]] : 0;
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

void PaperProposalTracker::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Proposals", QString::number(entries_.size()), QColor(59,130,246)},
        {"Submitted", QString::number(submittedCount()), QColor(16,185,129)},
        {"Total", "$" + QString::number(static_cast<int>(totalAmount())), QColor(245,158,11)},
        {"Categories", QString::number(categoryCounts().size()), QColor(139,92,246)}
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

void PaperProposalTracker::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Track research proposals"); return; }
    infoLabel_->setText(QString("%1 proposals | %2 submitted | $%3 total")
        .arg(entries_.size()).arg(submittedCount()).arg(static_cast<int>(totalAmount())));
}

void PaperProposalTracker::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ProposalEntry e;
        e.id = settings_.value("id").toInt();
        e.title = settings_.value("title").toString();
        e.status = settings_.value("status").toString();
        e.funder = settings_.value("funder").toString();
        e.amount = settings_.value("amount").toDouble();
        e.deadline = settings_.value("deadline").toString();
        e.score = settings_.value("score").toDouble();
        e.category = settings_.value("category").toString();
        e.papersPlanned = settings_.value("papersPlanned").toInt();
        e.submitted = settings_.value("submitted").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperProposalTracker::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("title", entries_[i].title);
        settings_.setValue("status", entries_[i].status);
        settings_.setValue("funder", entries_[i].funder);
        settings_.setValue("amount", entries_[i].amount);
        settings_.setValue("deadline", entries_[i].deadline);
        settings_.setValue("score", entries_[i].score);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("papersPlanned", entries_[i].papersPlanned);
        settings_.setValue("submitted", entries_[i].submitted);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
