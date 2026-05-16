#include "tools/PaperCertManager.hpp"
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperCertManager::PaperCertManager(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CertManager")
{
    setupUI();
    loadSettings();
}

void PaperCertManager::setupUI() {
    auto* layout = new QHBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    toolbar->setSpacing(6);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Web", "Email", "API", "Internal", "Test"});
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Domain name...");
    toolbar->addWidget(inputField_, 1);

    checkBtn_ = new QPushButton("Check");
    checkBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(checkBtn_, &QPushButton::clicked, this, &PaperCertManager::onCheck);
    toolbar->addWidget(checkBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCertManager::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Certificate Manager");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    layout->addStretch(1);

    setMinimumSize(640, 480);
}

void PaperCertManager::onCheck() {
    QString domain = inputField_->text().trimmed();
    if (domain.isEmpty()) return;

    int daysLeft = QRandomGenerator::global()->bounded(1, 366);
    int chainDepth = QRandomGenerator::global()->bounded(1, 6);
    bool valid = daysLeft > 30;

    QStringList issuers = {
        "Let's Encrypt Authority X3", "DigiCert SHA2 Extended Validation",
        "GlobalSign RSA OV SSL CA", "Sectigo RSA Domain Validation",
        "Cloudflare Inc ECC CA-3", "Amazon RSA 2048 M02",
        "Google Trust Services GTS R4", "Microsoft RSA TLS CA 02"
    };
    QString issuer = issuers.at(QRandomGenerator::global()->bounded(issuers.size()));

    QColor color;
    if (daysLeft > 90)       color = QColor(22, 163, 74);   // green
    else if (daysLeft > 30)  color = QColor(217, 119, 6);   // amber
    else                     color = QColor(220, 38, 38);   // red

    CertEntry entry;
    entry.id = entries_.size() + 1;
    entry.domain = domain;
    entry.category = categoryCombo_->currentText() == "All"
                         ? "Web" : categoryCombo_->currentText();
    entry.issuer = issuer;
    entry.daysLeft = daysLeft;
    entry.chainDepth = chainDepth;
    entry.valid = valid;
    entry.color = color;

    addEntry(entry);

    inputField_->clear();
}

void PaperCertManager::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    repaint();
}

void PaperCertManager::addEntry(const CertEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit certChecked(entry.id, entry.daysLeft);
    update();
}

QList<CertEntry> PaperCertManager::entries() const { return entries_; }

int PaperCertManager::validCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.valid) ++c;
    return c;
}

qreal PaperCertManager::avgDaysLeft() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.daysLeft;
    return sum / entries_.size();
}

QMap<QString, int> PaperCertManager::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperCertManager::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No certificates - enter a domain and click Check");
        return;
    }

    int w = width(), h = height();
    int colW = w / 3;

    drawCertList(p, QRect(10, 10, colW - 15, h - 20));
    drawCategoryChart(p, QRect(colW + 5, 10, colW - 15, h / 2 - 10));
    drawStats(p, QRect(colW + 5, h / 2 + 10, colW - 15, h / 2 - 20));
}

void PaperCertManager::drawCertList(QPainter& p, const QRect& rect) {
    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 18, "Certificate Manager");

    int startY = rect.y() + 34;
    int maxShow = qMin(static_cast<int>(entries_.size()), 12);
    int itemH = qMin(42, (rect.height() - startY + rect.y() - 10) / qMax(maxShow, 1));
    int barMaxW = rect.width() - 20;

    int shown = 0;
    for (int i = entries_.size() - 1; i >= 0 && shown < maxShow; --i) {
        const auto& e = entries_[i];
        int y = startY + shown * (itemH + 3);

        // Background
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        // Expiry bar
        int barW = static_cast<int>((e.daysLeft / 365.0) * barMaxW);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x() + 10, y + itemH - 8, qMax(barW, 4), 5, 2, 2);

        // Domain name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 10, y + 2, rect.width() - 20, 16, Qt::AlignVCenter,
                   e.domain.left(28));

        // Issuer / chain info
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 17, rect.width() - 20, 14, Qt::AlignVCenter,
                   e.issuer.left(30) + " | Chain: " + QString::number(e.chainDepth));

        // Days left badge
        p.setPen(e.valid ? QColor(22, 163, 74) : QColor(220, 38, 38));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + rect.width() - 60, y + 3, 54, 14, Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(static_cast<int>(e.daysLeft)) + "d");

        ++shown;
    }
}

void PaperCertManager::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    if (counts.isEmpty()) return;

    int maxVal = 1;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        maxVal = qMax(maxVal, it.value());

    QColor palette[] = {
        QColor(59, 130, 246), QColor(22, 163, 74), QColor(217, 119, 6),
        QColor(220, 38, 38), QColor(124, 58, 237)
    };
    int nCats = counts.size();
    int barH = qMin(24, (rect.height() - 30) / qMax(nCats, 1));
    int colorIdx = 0;

    int i = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        int y = rect.y() + 22 + i * (barH + 4);
        int count = it.value();
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 120));

        // Label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x(), y + barH - 3, 70, barH, Qt::AlignRight | Qt::AlignVCenter,
                   it.key());

        // Bar
        p.setPen(Qt::NoPen);
        p.setBrush(palette[colorIdx % 5]);
        p.drawRoundedRect(rect.x() + 75, y, qMax(barW, 4), barH - 2, 3, 3);

        // Count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 80 + barW, y + barH - 3, QString::number(count));

        ++i;
        ++colorIdx;
    }
}

void PaperCertManager::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Certs",  QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Valid",        QString::number(validCount()),    QColor(22, 163, 74)},
        {"Avg Days",     QString::number(avgDaysLeft(), 'f', 1), QColor(217, 119, 6)},
    };

    int boxH = qMin(48, (rect.height() - 10) / qMax(stats.size(), 1));
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 16, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() - 20, 26, Qt::AlignVCenter,
                   stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 30, rect.width() - 20, 14, Qt::AlignVCenter,
                   stats[i].label);
    }
}

void PaperCertManager::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Certificate Manager");
        return;
    }
    infoLabel_->setText(QString("Certs: %1 | Valid: %2 | Avg Days: %3")
        .arg(entries_.size())
        .arg(validCount())
        .arg(avgDaysLeft(), 0, 'f', 1));
}

void PaperCertManager::loadSettings() {
    settings_.beginGroup("CertManager");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        CertEntry e;
        e.id = settings_.value("id").toInt();
        e.domain = settings_.value("domain").toString();
        e.category = settings_.value("category").toString();
        e.issuer = settings_.value("issuer").toString();
        e.daysLeft = settings_.value("daysLeft").toReal();
        e.chainDepth = settings_.value("chainDepth").toInt();
        e.valid = settings_.value("valid").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperCertManager::saveSettings() {
    settings_.beginGroup("CertManager");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("domain", entries_[i].domain);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("issuer", entries_[i].issuer);
        settings_.setValue("daysLeft", entries_[i].daysLeft);
        settings_.setValue("chainDepth", entries_[i].chainDepth);
        settings_.setValue("valid", entries_[i].valid);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
