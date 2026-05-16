#include "workspace/PaperResearchFund.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperResearchFund::PaperResearchFund(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ResearchFund")
{
    setupUI();
    loadSettings();
}

void PaperResearchFund::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperResearchFund::onAdd);
    toolbar->addWidget(addBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Federal", "Foundation", "Industry", "Internal"});
    toolbar->addWidget(categoryCombo_, 1);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter fund description...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperResearchFund::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    infoLabel_ = new QLabel("Research Fund");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperResearchFund::addEntry(const FundEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit fundAwarded(entry.id, entry.amount);
    update();
}

QList<FundEntry> PaperResearchFund::entries() const { return entries_; }

int PaperResearchFund::activeCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.active) c++;
    return c;
}

qreal PaperResearchFund::totalAmount() const {
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.amount;
    return sum;
}

QMap<QString, int> PaperResearchFund::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperResearchFund::onAdd() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"Federal", "Foundation", "Industry", "Internal"};
    QStringList agencies = {"NSF", "NIH", "DARPA", "DOE", "Gates Foundation", "Simons", "Google", "Microsoft", "University"};
    QStringList grants = {"R01 Award", "CAREER Grant", "Exploratory Research", "Collaborative Research",
                          "Equipment Grant", "Travel Award", "Seed Fund", "Distinguished Fellowship"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        FundEntry e;
        e.id = entries_.size() + 1;
        e.grant = grants[QRandomGenerator::global()->bounded(grants.size())];
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.agency = agencies[QRandomGenerator::global()->bounded(agencies.size())];
        e.amount = 5000 + QRandomGenerator::global()->bounded(500000);
        e.years = 1 + QRandomGenerator::global()->bounded(5);
        e.active = QRandomGenerator::global()->bounded(2) == 0;
        e.color = colors[QRandomGenerator::global()->bounded(5)];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperResearchFund::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Research Fund");
    update();
}

void PaperResearchFund::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Research Fund");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Research Fund");
    int w = width(), h = height();
    drawFundList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperResearchFund::drawFundList(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(40, (rect.height() - 10) / qMax(show, 1));
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
        p.drawText(rect.x() + 10, y + 3, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.grant + (e.active ? "" : " [CLOSED]"));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 17, rect.width() / 2 - 10, 12, Qt::AlignVCenter,
                   e.agency + " | " + e.category);

        int barY = y + 30;
        int barW = rect.width() / 2 - 10;
        qreal maxAmount = 500000;
        qreal ratio = maxAmount > 0 ? qMin(e.amount / maxAmount, 1.0) : 0;
        int fillW = static_cast<int>(ratio * barW);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        p.drawRoundedRect(rect.x() + 10, barY, barW, 6, 3, 3);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x() + 10, barY, fillW, 6, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + rect.width() / 2, y + 3, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "$" + QString::number(e.amount, 'f', 0));
        p.drawText(rect.x() + rect.width() / 2, y + 17, rect.width() / 2 - 10, 12,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.years) + " yr" + (e.years > 1 ? "s" : ""));
        p.drawText(rect.x() + rect.width() / 2, y + 30, rect.width() / 2 - 10, 12,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.active ? "Active" : "Closed");
    }
}

void PaperResearchFund::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"Federal", "Foundation", "Industry", "Internal"};
    QString labels[] = {"Federal", "Found.", "Indust.", "Intern."};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 100));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 55, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 60, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 63 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperResearchFund::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Funds", QString::number(entries_.size()), QColor(59,130,246)},
        {"Active", QString::number(activeCount()), QColor(22,163,74)},
        {"Total Amount", "$" + QString::number(totalAmount(), 'f', 0), QColor(217,119,6)},
        {"Categories", QString::number(categoryCounts().size()), QColor(124,58,237)}
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

void PaperResearchFund::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Research Fund"); return; }
    infoLabel_->setText(QString("%1 funds | %2 active | $%3 total")
        .arg(entries_.size()).arg(activeCount()).arg(totalAmount(), 0, 'f', 0));
}

void PaperResearchFund::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        FundEntry e;
        e.id = settings_.value("id").toInt();
        e.grant = settings_.value("grant").toString();
        e.category = settings_.value("category").toString();
        e.agency = settings_.value("agency").toString();
        e.amount = settings_.value("amount").toDouble();
        e.years = settings_.value("years").toInt();
        e.active = settings_.value("active").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperResearchFund::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("grant", entries_[i].grant);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("agency", entries_[i].agency);
        settings_.setValue("amount", entries_[i].amount);
        settings_.setValue("years", entries_[i].years);
        settings_.setValue("active", entries_[i].active);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
