#include "workspace/PaperContractVault.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QDateTime>
#include <QPainterPath>

PaperContractVault::PaperContractVault(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ContractVault")
{
    setupUI();
    loadSettings();
}

void PaperContractVault::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "NDA", "SLA", "License", "Service"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; "
        "min-width: 90px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter contract...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    signBtn_ = new QPushButton("Sign");
    signBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 14px; "
        "border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(signBtn_, &QPushButton::clicked, this, &PaperContractVault::onSign);
    toolbar->addWidget(signBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperContractVault::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Contract vault empty");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(620, 500);
}

void PaperContractVault::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Sign contracts to populate the vault");
        return;
    }

    int w = width(), h = height();

    // Top half: vault view of contract cards
    drawVaultView(p, QRect(10, 10, w - 20, h / 2 - 10));

    // Bottom-left: category bar chart
    drawCategoryChart(p, QRect(10, h / 2 + 10, w / 2 - 10, h / 2 - 20));

    // Bottom-right: statistics
    drawStats(p, QRect(w / 2 + 10, h / 2 + 10, w / 2 - 20, h / 2 - 20));
}

void PaperContractVault::drawVaultView(QPainter& p, const QRect& rect) {
    // Section title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 12, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 16, "Contract Vault");

    // Filter by category combo selection
    QString filter = categoryCombo_->currentText();
    QList<const ContractVaultEntry*> visible;
    for (const auto& e : entries_) {
        if (filter == "All" || e.category == filter)
            visible.append(&e);
    }

    if (visible.isEmpty()) {
        p.setPen(QColor(148, 163, 184));
        p.setFont(QFont("Arial", 10));
        p.drawText(rect.adjusted(0, 30, 0, 0), Qt::AlignHCenter | Qt::AlignTop,
                   "No contracts in this category");
        return;
    }

    int cardsAreaTop = rect.y() + 26;
    int cardsAreaH = rect.height() - 30;
    int maxCards = qMin(visible.size(), 6);
    int cardH = qMin(52, (cardsAreaH - (maxCards - 1) * 4) / qMax(maxCards, 1));
    int cardW = rect.width();

    for (int i = 0; i < maxCards; ++i) {
        const auto& e = *visible[i];
        int y = cardsAreaTop + i * (cardH + 4);

        // Card background
        QPainterPath cardPath;
        cardPath.addRoundedRect(rect.x(), y, cardW, cardH, 6, 6);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(190));
        p.drawPath(cardPath);

        // Left accent stripe
        QPainterPath stripe;
        stripe.addRoundedRect(QRectF(rect.x(), y, 5, cardH), 2, 2);
        p.setBrush(e.color);
        p.drawPath(stripe);

        int leftX = rect.x() + 14;
        int rightX = rect.x() + cardW - 14;
        int midY1 = y + cardH / 2 - 6;
        int midY2 = y + cardH / 2 + 8;

        // Contract name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(QRect(leftX, midY1, cardW / 2, 16), Qt::AlignVCenter | Qt::AlignLeft,
                   e.contract);

        // Party
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 8));
        p.drawText(QRect(leftX, midY2, cardW / 3, 14), Qt::AlignVCenter | Qt::AlignLeft,
                   e.party);

        // Value badge
        QString valStr = "$" + QString::number(static_cast<int>(e.value));
        QFontMetrics fm(p.font());
        int badgeW = fm.horizontalAdvance(valStr) + 16;
        int badgeX = rightX - badgeW;
        QPainterPath badge;
        badge.addRoundedRect(badgeX, midY1, badgeW, 18, 9, 9);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawPath(badge);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(QRect(badgeX, midY1, badgeW, 18), Qt::AlignCenter, valStr);

        // Days-remaining countdown
        p.setPen(e.daysRemaining <= 30 ? QColor(220, 38, 38) : QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        QString daysText = QString::number(e.daysRemaining) + " days left";
        p.drawText(QRect(rightX - badgeW - 90, midY2, 80, 14),
                   Qt::AlignVCenter | Qt::AlignRight, daysText);

        // Active indicator dot
        if (e.active) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(34, 197, 94));
            p.drawEllipse(rightX - badgeW - 8, midY2 + 2, 8, 8);
        } else {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(203, 213, 225));
            p.drawEllipse(rightX - badgeW - 8, midY2 + 2, 8, 8);
        }
    }

    if (visible.size() > maxCards) {
        p.setPen(QColor(148, 163, 184));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), cardsAreaTop + maxCards * (cardH + 4) + 2,
                   "+" + QString::number(visible.size() - maxCards) + " more");
    }
}

void PaperContractVault::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 14, "Categories");

    auto counts = categoryCounts();
    struct CatInfo { QString key; QString label; QColor color; };
    CatInfo cats[] = {
        {"NDA",     "NDA",     QColor(59, 130, 246)},
        {"SLA",     "SLA",     QColor(22, 163, 74)},
        {"License", "License", QColor(124, 58, 237)},
        {"Service", "Service", QColor(217, 119, 6)}
    };

    int maxVal = 1;
    for (const auto& c : cats) {
        int v = counts.contains(c.key) ? counts[c.key] : 0;
        maxVal = qMax(maxVal, v);
    }

    int chartTop = rect.y() + 24;
    int chartH = rect.height() - 30;
    int barH = qMin(26, (chartH - 12) / 4);

    for (int i = 0; i < 4; ++i) {
        int y = chartTop + i * (barH + 4);
        int count = counts.contains(cats[i].key) ? counts[cats[i].key] : 0;
        qreal ratio = static_cast<qreal>(count) / maxVal;
        int barW = static_cast<int>(ratio * (rect.width() - 120));

        // Label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(QRect(rect.x(), y, 60, barH), Qt::AlignVCenter | Qt::AlignRight,
                   cats[i].label);

        // Bar
        QPainterPath barPath;
        barPath.addRoundedRect(rect.x() + 66, y + 2, qMax(barW, 2), barH - 4, 3, 3);
        p.setPen(Qt::NoPen);
        p.setBrush(cats[i].color);
        p.drawPath(barPath);

        // Count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 70 + barW, y + barH - 6, QString::number(count));
    }
}

void PaperContractVault::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 14, "Statistics");

    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Contracts", QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Total Value",     "$" + QString::number(static_cast<int>(totalValue())),
                            QColor(217, 119, 6)},
        {"Active",          QString::number(activeCount()),   QColor(22, 163, 74)}
    };

    int boxH = qMin(48, (rect.height() - 30) / 3);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + 24 + i * (boxH + 6);

        QPainterPath box;
        box.addRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawPath(box);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(QRect(rect.x() + 10, y + 4, rect.width() - 20, 24),
                   Qt::AlignVCenter | Qt::AlignLeft, stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(QRect(rect.x() + 10, y + 28, rect.width() - 20, 16),
                   Qt::AlignVCenter | Qt::AlignLeft, stats[i].label);
    }
}

void PaperContractVault::onSign() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    int catIdx = categoryCombo_->currentIndex();
    QStringList categories = {"NDA", "SLA", "License", "Service"};
    QString cat = catIdx == 0
        ? categories[QRandomGenerator::global()->bounded(categories.size())]
        : categories[catIdx - 1];

    QColor catColor;
    if (cat == "NDA")     catColor = QColor(59, 130, 246);
    else if (cat == "SLA")     catColor = QColor(22, 163, 74);
    else if (cat == "License") catColor = QColor(124, 58, 237);
    else                        catColor = QColor(217, 119, 6);

    QStringList parties = {"Acme Corp", "Research Labs Inc", "DataSync Ltd",
                           "Uni Partners", "TechForge", "BioGenix"};

    ContractVaultEntry entry;
    entry.id = entries_.size() + 1;
    entry.contract = text;
    entry.category = cat;
    entry.party = parties[QRandomGenerator::global()->bounded(parties.size())];
    entry.value = 2000.0 + QRandomGenerator::global()->bounded(80000);
    entry.daysRemaining = 30 + QRandomGenerator::global()->bounded(335);
    entry.active = entry.daysRemaining > 0;
    entry.color = catColor;

    addEntry(entry);
    inputField_->clear();
}

void PaperContractVault::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperContractVault::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Contract vault empty");
        return;
    }
    infoLabel_->setText(
        QString("%1 contracts | %2 active | $%3 total | %4 categories")
            .arg(entries_.size())
            .arg(activeCount())
            .arg(static_cast<int>(totalValue()))
            .arg(categoryCounts().size()));
}

void PaperContractVault::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ContractVaultEntry e;
        e.id = settings_.value("id").toInt();
        e.contract = settings_.value("contract").toString();
        e.category = settings_.value("category").toString();
        e.party = settings_.value("party").toString();
        e.value = settings_.value("value").toDouble();
        e.daysRemaining = settings_.value("daysRemaining").toInt();
        e.active = settings_.value("active").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperContractVault::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("contract", entries_[i].contract);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("party", entries_[i].party);
        settings_.setValue("value", entries_[i].value);
        settings_.setValue("daysRemaining", entries_[i].daysRemaining);
        settings_.setValue("active", entries_[i].active);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}

void PaperContractVault::addEntry(const ContractVaultEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit contractSigned(entry.id, entry.value);
    update();
}

QList<ContractVaultEntry> PaperContractVault::entries() const {
    return entries_;
}

int PaperContractVault::activeCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.active) ++c;
    return c;
}

qreal PaperContractVault::totalValue() const {
    qreal t = 0;
    for (const auto& e : entries_) t += e.value;
    return t;
}

QMap<QString, int> PaperContractVault::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}
