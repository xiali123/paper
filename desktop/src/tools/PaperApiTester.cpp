#include "tools/PaperApiTester.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperApiTester::PaperApiTester(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ApiTester")
{
    setupUI();
    loadSettings();
}

void PaperApiTester::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    testBtn_ = new QPushButton("Test");
    testBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(testBtn_, &QPushButton::clicked, this, &PaperApiTester::onTest);
    toolbar->addWidget(testBtn_);

    toolbar->addWidget(new QLabel("Method:"));
    methodCombo_ = new QComboBox();
    methodCombo_->addItems({"GET", "POST", "PUT", "DELETE", "PATCH"});
    toolbar->addWidget(methodCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperApiTester::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter API endpoint to test...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Test API endpoints");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperApiTester::addEntry(const ApiTestEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit testCompleted(entry.id, entry.responseTime);
    update();
}

QList<ApiTestEntry> PaperApiTester::entries() const { return entries_; }

int PaperApiTester::passingCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.passing) c++;
    return c;
}

qreal PaperApiTester::avgResponseTime() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.responseTime;
    return sum / entries_.size();
}

QMap<QString, int> PaperApiTester::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperApiTester::onTest() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList methods = {"GET", "POST", "PUT", "DELETE", "PATCH"};
    QStringList categories = {"search", "paper", "citation", "user", "analytics"};
    QStringList endpoints = {"/api/search", "/api/papers", "/api/citations", "/api/users", "/api/stats"};

    int mIdx = methodCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        ApiTestEntry e;
        e.id = entries_.size() + 1;
        e.endpoint = text.left(15) + endpoints[i % endpoints.size()];
        e.method = methods[mIdx];
        e.statusCode = QRandomGenerator::global()->bounded(2) == 0 ? 200 :
                       (200 + QRandomGenerator::global()->bounded(4) * 100);
        e.responseTime = 10 + QRandomGenerator::global()->bounded(2000);
        e.status = e.statusCode == 200 ? "pass" : (e.statusCode < 500 ? "warn" : "fail");
        e.retries = e.statusCode >= 500 ? 1 + QRandomGenerator::global()->bounded(3) : 0;
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.papersAffected = QRandomGenerator::global()->bounded(50);
        e.successRate = e.statusCode == 200 ? 0.9 + QRandomGenerator::global()->bounded(10) / 100.0 :
                        QRandomGenerator::global()->bounded(90) / 100.0;
        e.passing = e.statusCode == 200 && e.responseTime < 1000;
        e.color = e.passing ? QColor(16,185,129) : (e.statusCode < 500 ? QColor(245,158,11) : QColor(239,68,68));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperApiTester::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Test API endpoints");
    update();
}

void PaperApiTester::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Test API endpoints");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "API Tester");

    int w = width(), h = height();
    drawTestList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperApiTester::drawTestList(QPainter& p, const QRect& rect) {
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
                   e.method + " " + e.endpoint.left(18));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + QString::number(e.retries) + " retries | " + QString::number(e.papersAffected) + " papers");
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.statusCode) + " | " + QString::number(e.responseTime) + "ms");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.status + " | " + QString::number(e.successRate * 100, 'f', 0) + "% success");
    }
}

void PaperApiTester::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"search", "paper", "citation", "user", "analytics"};
    QString labels[] = {"Search", "Paper", "Cite", "User", "Stats"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68), QColor(139,92,246)};

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

void PaperApiTester::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Tests", QString::number(entries_.size()), QColor(59,130,246)},
        {"Passing", QString::number(passingCount()), QColor(16,185,129)},
        {"Avg Time", QString::number(avgResponseTime(), 'f', 0) + "ms", QColor(245,158,11)},
        {"Endpoints", QString::number(categoryCounts().size()), QColor(139,92,246)}
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

void PaperApiTester::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Test API endpoints"); return; }
    infoLabel_->setText(QString("%1 tests | %2 passing | %3ms avg")
        .arg(entries_.size()).arg(passingCount()).arg(avgResponseTime(), 0, 'f', 0));
}

void PaperApiTester::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ApiTestEntry e;
        e.id = settings_.value("id").toInt();
        e.endpoint = settings_.value("endpoint").toString();
        e.method = settings_.value("method").toString();
        e.statusCode = settings_.value("statusCode").toInt();
        e.responseTime = settings_.value("responseTime").toDouble();
        e.status = settings_.value("status").toString();
        e.retries = settings_.value("retries").toInt();
        e.category = settings_.value("category").toString();
        e.papersAffected = settings_.value("papersAffected").toInt();
        e.successRate = settings_.value("successRate").toDouble();
        e.passing = settings_.value("passing").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperApiTester::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("endpoint", entries_[i].endpoint);
        settings_.setValue("method", entries_[i].method);
        settings_.setValue("statusCode", entries_[i].statusCode);
        settings_.setValue("responseTime", entries_[i].responseTime);
        settings_.setValue("status", entries_[i].status);
        settings_.setValue("retries", entries_[i].retries);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("papersAffected", entries_[i].papersAffected);
        settings_.setValue("successRate", entries_[i].successRate);
        settings_.setValue("passing", entries_[i].passing);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
