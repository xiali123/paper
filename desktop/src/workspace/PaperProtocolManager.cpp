#include "workspace/PaperProtocolManager.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperProtocolManager::PaperProtocolManager(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ProtocolManager")
{
    setupUI();
    loadSettings();
}

void PaperProtocolManager::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    updateBtn_ = new QPushButton("Update");
    updateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(updateBtn_, &QPushButton::clicked, this, &PaperProtocolManager::onUpdate);
    toolbar->addWidget(updateBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Safety", "Quality", "Compliance", "Experimental", "Clinical"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperProtocolManager::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter protocol name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Manage protocols");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperProtocolManager::addEntry(const ProtocolEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit protocolUpdated(entry.id, entry.completion);
    update();
}

QList<ProtocolEntry> PaperProtocolManager::entries() const { return entries_; }

int PaperProtocolManager::approvedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.approved) c++;
    return c;
}

qreal PaperProtocolManager::avgCompletion() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.completion;
    return sum / entries_.size();
}

QMap<QString, int> PaperProtocolManager::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperProtocolManager::onUpdate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"safety", "quality", "compliance", "experimental", "clinical"};
    QStringList statuses = {"active", "draft", "review", "archived"};
    QColor palette[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        ProtocolEntry e;
        e.id = entries_.size() + 1;
        e.name = text.left(10) + " proto" + QString::number(i);
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.status = statuses[QRandomGenerator::global()->bounded(statuses.size())];
        e.steps = 5 + QRandomGenerator::global()->bounded(20);
        e.completion = QRandomGenerator::global()->bounded(100) / 100.0;
        e.version = "v" + QString::number(1 + QRandomGenerator::global()->bounded(3)) + "."
                    + QString::number(QRandomGenerator::global()->bounded(10));
        e.approved = e.completion >= 1.0;
        e.color = palette[QRandomGenerator::global()->bounded(5)];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperProtocolManager::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Manage protocols");
    update();
}

void PaperProtocolManager::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Manage protocols");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Protocol Manager");
    int w = width(), h = height();
    drawProtocolList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperProtocolManager::drawProtocolList(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(36, (rect.height() - 10) / qMax(show, 1));
    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 3);
        // background
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);
        // color accent bar
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);
        // name and status
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 2, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.name.left(14) + (e.approved ? " [OK]" : ""));
        // steps, version, status
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 18, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.status + " | " + QString::number(e.steps) + " steps | " + e.version);
        // completion bar
        int barX = rect.x() + rect.width() / 2 + 5;
        int barW = rect.width() / 2 - 50;
        p.setBrush(QColor(226, 232, 240));
        p.drawRoundedRect(barX, y + 8, barW, 8, 3, 3);
        p.setBrush(e.color);
        p.drawRoundedRect(barX, y + 8, static_cast<int>(barW * qBound(0.0, e.completion, 1.0)), 8, 3, 3);
        // percentage label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(barX + barW + 4, y + 16,
                   QString::number(e.completion * 100, 'f', 0) + "%");
    }
}

void PaperProtocolManager::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"safety", "quality", "compliance", "experimental", "clinical"};
    QString labels[] = {"Safety", "Quality", "Comply", "Exper", "Clinical"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(22, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
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

void PaperProtocolManager::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Protocols", QString::number(entries_.size()), QColor(59,130,246)},
        {"Approved", QString::number(approvedCount()), QColor(22,163,74)},
        {"Avg Complete", QString::number(avgCompletion() * 100, 'f', 0) + "%", QColor(217,119,6)},
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

void PaperProtocolManager::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Manage protocols"); return; }
    infoLabel_->setText(QString("%1 protocols | %2 approved | %3% done")
        .arg(entries_.size()).arg(approvedCount()).arg(avgCompletion() * 100, 0, 'f', 0));
}

void PaperProtocolManager::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ProtocolEntry e;
        e.id = settings_.value("id").toInt();
        e.name = settings_.value("name").toString();
        e.category = settings_.value("category").toString();
        e.status = settings_.value("status").toString();
        e.steps = settings_.value("steps").toInt();
        e.completion = settings_.value("completion").toDouble();
        e.version = settings_.value("version").toString();
        e.approved = settings_.value("approved").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperProtocolManager::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("name", entries_[i].name);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("status", entries_[i].status);
        settings_.setValue("steps", entries_[i].steps);
        settings_.setValue("completion", entries_[i].completion);
        settings_.setValue("version", entries_[i].version);
        settings_.setValue("approved", entries_[i].approved);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
