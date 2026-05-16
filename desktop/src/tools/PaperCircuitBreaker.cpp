#include "tools/PaperCircuitBreaker.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperCircuitBreaker::PaperCircuitBreaker(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CircuitBreaker")
{
    setupUI();
    loadSettings();
}

void PaperCircuitBreaker::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    monitorBtn_ = new QPushButton("Monitor");
    monitorBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(monitorBtn_, &QPushButton::clicked, this, &PaperCircuitBreaker::onMonitor);
    toolbar->addWidget(monitorBtn_);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "API", "Database", "Network", "Auth"});
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Service name...");
    toolbar->addWidget(inputField_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCircuitBreaker::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Circuit breaker monitor");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 460);
}

void PaperCircuitBreaker::addEntry(const CircuitEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit breakerTripped(entry.id, entry.failureRate);
    update();
}

QList<CircuitEntry> PaperCircuitBreaker::entries() const { return entries_; }

int PaperCircuitBreaker::openCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.open) c++;
    return c;
}

qreal PaperCircuitBreaker::avgFailureRate() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.failureRate;
    return sum / entries_.size();
}

QMap<QString, int> PaperCircuitBreaker::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperCircuitBreaker::onMonitor() {
    QStringList services = {"SearchAPI", "CiteGraph", "PDFParser", "RankEngine",
                           "ExportSvc", "AuthGateway", "CacheLayer", "IndexWorker"};
    QStringList categories = {"API", "Database", "Network", "Auth"};
    QStringList states = {"Closed", "Open", "Half-Open"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,106), QColor(217,119,6),
                       QColor(220,38,38), QColor(124,58,237)};

    int count = 4 + QRandomGenerator::global()->bounded(5);
    for (int i = 0; i < count; ++i) {
        CircuitEntry e;
        e.id = entries_.size() + 1;
        e.service = services[QRandomGenerator::global()->bounded(services.size())];
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.state = states[QRandomGenerator::global()->bounded(states.size())];
        e.failureRate = QRandomGenerator::global()->bounded(100) / 100.0;
        e.requests = QRandomGenerator::global()->bounded(500) + 10;
        e.open = e.state == "Open";
        e.color = colors[QRandomGenerator::global()->bounded(5)];
        addEntry(e);
    }
}

void PaperCircuitBreaker::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Circuit breaker monitor");
    update();
}

void PaperCircuitBreaker::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Circuit breaker monitor");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Circuit Breaker");

    int w = width(), h = height();
    drawCircuitView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperCircuitBreaker::drawCircuitView(QPainter& p, const QRect& rect) {
    int filterIdx = categoryCombo_->currentIndex();
    int show = 0;
    int maxShow = 10;
    int itemH = qMin(36, (rect.height() - 10) / maxShow);

    for (int i = entries_.size() - 1; i >= 0 && show < maxShow; --i) {
        const auto& e = entries_[i];
        if (filterIdx > 0) {
            QStringList cats = {"All", "API", "Database", "Network", "Auth"};
            if (e.category != cats[filterIdx]) continue;
        }

        int y = rect.y() + show * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(e.open ? e.color.lighter(190) : Qt::white);
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(e.open ? QColor(220,38,38) : QColor(22,163,106));
        p.drawEllipse(rect.x() + 6, y + itemH / 2 - 4, 8, 8);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 20, y + 2, rect.width() - 50, 16, Qt::AlignVCenter,
                   e.service.left(24));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 20, y + 18, rect.width() - 50, 14, Qt::AlignVCenter,
                   e.category + " | " + e.state);

        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + rect.width() - 60, y + 2, 56, 14, Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.failureRate * 100, 'f', 1) + "%");
        show++;
    }
}

void PaperCircuitBreaker::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "By Category");

    auto counts = categoryCounts();
    QStringList cats = {"API", "Database", "Network", "Auth"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,106), QColor(217,119,6), QColor(220,38,38)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(28, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 4);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x(), y + barH - 3, 70, barH, Qt::AlignRight | Qt::AlignVCenter, cats[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 75, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 78 + barW, y + barH - 3, QString::number(count));
    }
}

void PaperCircuitBreaker::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total", QString::number(entries_.size()), QColor(59,130,246)},
        {"Open", QString::number(openCount()), QColor(220,38,38)},
        {"Avg Fail", QString::number(avgFailureRate() * 100, 'f', 1) + "%", QColor(217,119,6)},
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

void PaperCircuitBreaker::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Circuit breaker monitor"); return; }
    infoLabel_->setText(QString("%1 services | %2 open | %3 avg fail")
        .arg(entries_.size()).arg(openCount())
        .arg(QString::number(avgFailureRate() * 100, 'f', 1) + "%"));
}

void PaperCircuitBreaker::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        CircuitEntry e;
        e.id = settings_.value("id").toInt();
        e.service = settings_.value("service").toString();
        e.category = settings_.value("category").toString();
        e.state = settings_.value("state").toString();
        e.failureRate = settings_.value("failureRate").toReal();
        e.requests = settings_.value("requests").toInt();
        e.open = settings_.value("open").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperCircuitBreaker::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("service", entries_[i].service);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("state", entries_[i].state);
        settings_.setValue("failureRate", entries_[i].failureRate);
        settings_.setValue("requests", entries_[i].requests);
        settings_.setValue("open", entries_[i].open);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
