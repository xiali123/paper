#include "workspace/PaperGrantTracker.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

PaperGrantTracker::PaperGrantTracker(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "GrantTracker")
{
    setupUI();
    loadSettings();
}

void PaperGrantTracker::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Grant");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperGrantTracker::onAdd);
    toolbar->addWidget(addBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperGrantTracker::onClear);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Track research grants");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperGrantTracker::addGrant(const GrantEntry& grant) {
    grants_.append(grant);
    saveSettings();
    updateInfo();
    update();
}

QList<GrantEntry> PaperGrantTracker::grants() const { return grants_; }

QMap<QString, int> PaperGrantTracker::statusCounts() const {
    QMap<QString, int> counts;
    for (const auto& g : grants_) counts[g.status]++;
    return counts;
}

qreal PaperGrantTracker::totalFunding() const {
    qreal t = 0;
    for (const auto& g : grants_) t += g.amount;
    return t;
}

qreal PaperGrantTracker::awardedFunding() const {
    qreal t = 0;
    for (const auto& g : grants_) if (g.status == "awarded") t += g.amount;
    return t;
}

void PaperGrantTracker::onAdd() {
    bool ok;
    QString title = QInputDialog::getText(this, "Add Grant", "Grant title:", QLineEdit::Normal, "", &ok);
    if (!ok || title.isEmpty()) return;
    QString agency = QInputDialog::getText(this, "Add Grant", "Agency:", QLineEdit::Normal, "", &ok);
    if (!ok) return;
    QStringList statuses = {"draft", "submitted", "reviewed", "awarded", "rejected"};
    QString status = QInputDialog::getItem(this, "Add Grant", "Status:", statuses, 1, false, &ok);
    if (!ok) return;
    double amount = QInputDialog::getDouble(this, "Add Grant", "Amount ($K):", 100, 0, 100000, 0, &ok);
    if (!ok) return;

    GrantEntry g;
    g.id = grants_.size() + 1;
    g.title = title;
    g.agency = agency.isEmpty() ? "TBD" : agency;
    g.status = status;
    g.amount = amount;
    g.deadline = QDate::currentDate().addDays(QRandomGenerator::global()->bounded(90));
    g.startDate = QDate::currentDate().addDays(30 + QRandomGenerator::global()->bounded(60));
    g.pi = "PI";
    g.progress = status == "awarded" ? 100 : status == "rejected" ? 0 : QRandomGenerator::global()->bounded(80);

    QColor statusColors[] = {QColor(100,116,139), QColor(59,130,246), QColor(245,158,11), QColor(16,185,129), QColor(239,68,68)};
    int sIdx = statuses.indexOf(status);
    g.color = statusColors[qBound(0, sIdx, 4)];
    addGrant(g);
}

void PaperGrantTracker::onClear() {
    grants_.clear();
    saveSettings();
    infoLabel_->setText("Track research grants");
    update();
}

void PaperGrantTracker::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (grants_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Track research grants");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Grant Tracker");

    int w = width(), h = height();
    drawGrantCards(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawFundingChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperGrantTracker::drawGrantCards(QPainter& p, const QRect& rect) {
    int show = qMin(7, grants_.size());
    int cardH = qMin(54, (rect.height() - 10) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& g = grants_[i];
        int y = rect.y() + i * (cardH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(g.color.lighter(180));
        p.drawRoundedRect(rect.x(), y, rect.width(), cardH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(g.color);
        p.drawRoundedRect(rect.x(), y, 4, cardH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   g.title.left(18));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   g.agency + " | " + g.status);
        p.drawText(rect.x() + 10, y + 34, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   "Due: " + g.deadline.toString("MM/dd"));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 10, QFont::Bold));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "$" + QString::number(g.amount, 'f', 0) + "K");

        int barX = rect.x() + rect.width() / 2;
        int barW = rect.width() / 2 - 15;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(241, 245, 249));
        p.drawRoundedRect(barX, y + 24, barW, 8, 4, 4);
        p.setBrush(g.color);
        p.drawRoundedRect(barX, y + 24, static_cast<int>(barW * g.progress / 100.0), 8, 4, 4);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(barX, y + 36, barW, 14, Qt::AlignVCenter,
                   QString::number(g.progress) + "% | " + g.pi);
    }
}

void PaperGrantTracker::drawFundingChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Funding by Status");

    auto counts = statusCounts();
    QStringList statuses = {"draft", "submitted", "reviewed", "awarded", "rejected"};
    QString labels[] = {"Draft", "Submitted", "Reviewed", "Awarded", "Rejected"};
    QColor colors[] = {QColor(100,116,139), QColor(59,130,246), QColor(245,158,11), QColor(16,185,129), QColor(239,68,68)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(20, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(statuses[i]) ? counts[statuses[i]] : 0;
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

void PaperGrantTracker::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Grants", QString::number(grants_.size()), QColor(59,130,246)},
        {"Total", "$" + QString::number(totalFunding(), 'f', 0) + "K", QColor(16,185,129)},
        {"Awarded", "$" + QString::number(awardedFunding(), 'f', 0) + "K", QColor(245,158,11)},
        {"Awarded Count", QString::number(statusCounts().value("awarded", 0)), QColor(139,92,246)}
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

void PaperGrantTracker::updateInfo() {
    if (grants_.isEmpty()) { infoLabel_->setText("Track research grants"); return; }
    infoLabel_->setText(QString("%1 grants | $%2K total | $%3K awarded")
        .arg(grants_.size()).arg(totalFunding(), 0, 'f', 0).arg(awardedFunding(), 0, 'f', 0));
}

void PaperGrantTracker::loadSettings() {
    int size = settings_.beginReadArray("grants");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        GrantEntry g;
        g.id = settings_.value("id").toInt();
        g.title = settings_.value("title").toString();
        g.agency = settings_.value("agency").toString();
        g.status = settings_.value("status").toString();
        g.amount = settings_.value("amount").toDouble();
        g.deadline = QDate::fromString(settings_.value("deadline").toString(), Qt::ISODate);
        g.startDate = QDate::fromString(settings_.value("startDate").toString(), Qt::ISODate);
        g.progress = settings_.value("progress").toDouble();
        g.pi = settings_.value("pi").toString();
        g.color = QColor(settings_.value("color").toString());
        grants_.append(g);
    }
    settings_.endArray();
    updateInfo();
}

void PaperGrantTracker::saveSettings() {
    settings_.beginWriteArray("grants");
    for (int i = 0; i < grants_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", grants_[i].id);
        settings_.setValue("title", grants_[i].title);
        settings_.setValue("agency", grants_[i].agency);
        settings_.setValue("status", grants_[i].status);
        settings_.setValue("amount", grants_[i].amount);
        settings_.setValue("deadline", grants_[i].deadline.toString(Qt::ISODate));
        settings_.setValue("startDate", grants_[i].startDate.toString(Qt::ISODate));
        settings_.setValue("progress", grants_[i].progress);
        settings_.setValue("pi", grants_[i].pi);
        settings_.setValue("color", grants_[i].color.name());
    }
    settings_.endArray();
}
