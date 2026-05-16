#include "tools/PaperRateLimiter.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperRateLimiter::PaperRateLimiter(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "RateLimiter")
{
    setupUI();
    loadSettings();
}

void PaperRateLimiter::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    checkBtn_ = new QPushButton("Check");
    checkBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(checkBtn_, &QPushButton::clicked, this, &PaperRateLimiter::onCheck);
    toolbar->addWidget(checkBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"search", "paper", "citation", "user", "analytics"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperRateLimiter::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter endpoint to rate-limit check...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Rate Limiter Monitor");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperRateLimiter::addEntry(const RateEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit rateChecked(entry.id, entry.utilization);
    update();
}

QList<RateEntry> PaperRateLimiter::entries() const { return entries_; }

int PaperRateLimiter::throttledCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.throttled) c++;
    return c;
}

qreal PaperRateLimiter::avgUtilization() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.utilization;
    return sum / entries_.size();
}

QMap<QString, int> PaperRateLimiter::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperRateLimiter::onCheck() {
    QString endpoint = inputField_->text().trimmed();
    if (endpoint.isEmpty()) return;

    QStringList categories = {"search", "paper", "citation", "user", "analytics"};
    QStringList tiers = {"free", "basic", "pro", "enterprise"};
    QColor palette[] = {
        QColor(59, 130, 246),   // #3b82f6
        QColor(22, 163, 74),    // #16a34a
        QColor(217, 119, 6),    // #d97706
        QColor(220, 38, 38),    // #dc2626
        QColor(124, 58, 237)    // #7c3aed
    };

    QString category = categoryCombo_->currentText();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        RateEntry e;
        e.id = entries_.size() + 1;
        e.endpoint = endpoint.left(20) + QString("/v%1").arg(i + 1);
        e.category = category;
        e.tier = tiers[QRandomGenerator::global()->bounded(tiers.size())];
        e.limit = 50 + QRandomGenerator::global()->bounded(950);
        e.used = QRandomGenerator::global()->bounded(e.limit + 1);
        e.utilization = static_cast<qreal>(e.used) / e.limit;
        e.throttled = e.utilization > 0.9;
        e.color = palette[QRandomGenerator::global()->bounded(5)];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperRateLimiter::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Rate Limiter Monitor");
    update();
}

void PaperRateLimiter::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Rate Limiter Monitor");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Rate Limiter");

    int w = width(), h = height();
    drawRateList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperRateLimiter::drawRateList(QPainter& p, const QRect& rect) {
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
        p.setFont(QFont("Courier", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.endpoint.left(22));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + e.tier + (e.throttled ? " | THROTTLED" : ""));

        // Utilization bar background
        int barX = rect.x() + rect.width() / 2;
        int barW = rect.width() / 2 - 50;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        p.drawRoundedRect(barX, y + 4, barW, 10, 3, 3);

        // Utilization bar fill
        p.setBrush(e.throttled ? QColor(220, 38, 38) : e.color);
        p.drawRoundedRect(barX, y + 4, static_cast<int>(barW * qMin(e.utilization, 1.0)), 10, 3, 3);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(barX + barW + 4, y + 14,
                   QString("%1%").arg(e.utilization * 100, 0, 'f', 0));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString("%1/%2").arg(e.used).arg(e.limit));
    }
}

void PaperRateLimiter::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"search", "paper", "citation", "user", "analytics"};
    QString labels[] = {"Search", "Paper", "Cite", "User", "Stats"};
    QColor colors[] = {
        QColor(59, 130, 246),   // #3b82f6
        QColor(22, 163, 74),    // #16a34a
        QColor(217, 119, 6),    // #d97706
        QColor(220, 38, 38),    // #dc2626
        QColor(124, 58, 237)    // #7c3aed
    };

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 2);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 100));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 50, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 55, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 58 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperRateLimiter::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries",     QString::number(entries_.size()),        QColor(59, 130, 246)},
        {"Throttled",   QString::number(throttledCount()),       QColor(220, 38, 38)},
        {"Avg Util",    QString::number(avgUtilization() * 100, 'f', 1) + "%", QColor(217, 119, 6)},
        {"Categories",  QString::number(categoryCounts().size()), QColor(124, 58, 237)}
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

void PaperRateLimiter::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Rate Limiter Monitor");
        return;
    }
    infoLabel_->setText(QString("%1 entries | %2 throttled | %3% avg utilization")
        .arg(entries_.size())
        .arg(throttledCount())
        .arg(avgUtilization() * 100, 0, 'f', 1));
}

void PaperRateLimiter::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        RateEntry e;
        e.id          = settings_.value("id").toInt();
        e.endpoint    = settings_.value("endpoint").toString();
        e.category    = settings_.value("category").toString();
        e.tier        = settings_.value("tier").toString();
        e.limit       = settings_.value("limit").toInt();
        e.used        = settings_.value("used").toInt();
        e.utilization = settings_.value("utilization").toDouble();
        e.throttled   = settings_.value("throttled").toBool();
        e.color       = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperRateLimiter::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",          entries_[i].id);
        settings_.setValue("endpoint",    entries_[i].endpoint);
        settings_.setValue("category",    entries_[i].category);
        settings_.setValue("tier",        entries_[i].tier);
        settings_.setValue("limit",       entries_[i].limit);
        settings_.setValue("used",        entries_[i].used);
        settings_.setValue("utilization", entries_[i].utilization);
        settings_.setValue("throttled",   entries_[i].throttled);
        settings_.setValue("color",       entries_[i].color.name());
    }
    settings_.endArray();
}
