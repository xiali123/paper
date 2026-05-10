#include "workspace/PaperVendorComparison.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperVendorComparison::PaperVendorComparison(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "VendorComparison")
{
    setupUI();
    loadSettings();
}

void PaperVendorComparison::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    compareBtn_ = new QPushButton("Compare");
    compareBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(compareBtn_, &QPushButton::clicked, this, &PaperVendorComparison::onCompare);
    toolbar->addWidget(compareBtn_);
    toolbar->addWidget(new QLabel("Service:"));
    serviceCombo_ = new QComboBox();
    serviceCombo_->addItems({"All", "Hosting", "API", "Storage", "Compute"});
    toolbar->addWidget(serviceCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperVendorComparison::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter vendor name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Compare vendors");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperVendorComparison::addEntry(const VendorEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit vendorCompared(entry.id, entry.price);
    update();
}

QList<VendorEntry> PaperVendorComparison::entries() const { return entries_; }

qreal PaperVendorComparison::avgRating() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.rating;
    return sum / entries_.size();
}

int PaperVendorComparison::preferredCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.preferred) c++;
    return c;
}

QMap<QString, int> PaperVendorComparison::serviceCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.service]++;
    return counts;
}

void PaperVendorComparison::onCompare() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList services = {"hosting", "api", "storage", "compute"};
    QStringList reliability = {"high", "medium", "low"};
    int sIdx = serviceCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        VendorEntry e;
        e.id = entries_.size() + 1;
        e.vendorName = text.left(8) + " vendor" + QString::number(i);
        e.service = sIdx == 0 ? services[QRandomGenerator::global()->bounded(services.size())] : services[sIdx - 1];
        e.price = 50 + QRandomGenerator::global()->bounded(5000);
        e.rating = 1 + QRandomGenerator::global()->bounded(40) / 10.0;
        e.reliability = reliability[QRandomGenerator::global()->bounded(reliability.size())];
        e.contracts = 1 + QRandomGenerator::global()->bounded(50);
        e.savings = QRandomGenerator::global()->bounded(30) / 10.0;
        e.preferred = e.rating >= 4.0 && e.reliability == "high";
        e.color = e.preferred ? QColor(16,185,129) : (e.rating >= 3.0 ? QColor(59,130,246) : QColor(245,158,11));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperVendorComparison::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Compare vendors");
    update();
}

void PaperVendorComparison::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Compare vendors");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Vendor Comparison");
    int w = width(), h = height();
    drawVendorList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawServiceChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperVendorComparison::drawVendorList(QPainter& p, const QRect& rect) {
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
                   e.vendorName.left(14) + (e.preferred ? " [PREF]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.service + " | " + e.reliability + " | " + QString::number(e.contracts) + " contracts");
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "$" + QString::number(e.price, 'f', 0));
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.rating, 'f', 1) + "/5 | " + QString::number(e.savings, 'f', 0) + "% save");
    }
}

void PaperVendorComparison::drawServiceChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Services");
    auto counts = serviceCounts();
    QStringList services = {"hosting", "api", "storage", "compute"};
    QString labels[] = {"Hosting", "API", "Storage", "Compute"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(services[i]) ? counts[services[i]] : 0;
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

void PaperVendorComparison::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Vendors", QString::number(entries_.size()), QColor(59,130,246)},
        {"Preferred", QString::number(preferredCount()), QColor(16,185,129)},
        {"Avg Rating", QString::number(avgRating(), 'f', 1) + "/5", QColor(245,158,11)},
        {"Services", QString::number(serviceCounts().size()), QColor(139,92,246)}
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

void PaperVendorComparison::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Compare vendors"); return; }
    infoLabel_->setText(QString("%1 vendors | %2 preferred | %3/5 avg")
        .arg(entries_.size()).arg(preferredCount()).arg(avgRating(), 0, 'f', 1));
}

void PaperVendorComparison::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        VendorEntry e;
        e.id = settings_.value("id").toInt();
        e.vendorName = settings_.value("vendorName").toString();
        e.service = settings_.value("service").toString();
        e.price = settings_.value("price").toDouble();
        e.rating = settings_.value("rating").toDouble();
        e.reliability = settings_.value("reliability").toString();
        e.contracts = settings_.value("contracts").toInt();
        e.savings = settings_.value("savings").toDouble();
        e.preferred = settings_.value("preferred").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperVendorComparison::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("vendorName", entries_[i].vendorName);
        settings_.setValue("service", entries_[i].service);
        settings_.setValue("price", entries_[i].price);
        settings_.setValue("rating", entries_[i].rating);
        settings_.setValue("reliability", entries_[i].reliability);
        settings_.setValue("contracts", entries_[i].contracts);
        settings_.setValue("savings", entries_[i].savings);
        settings_.setValue("preferred", entries_[i].preferred);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
