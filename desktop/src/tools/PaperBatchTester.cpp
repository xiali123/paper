#include "tools/PaperBatchTester.hpp"
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperBatchTester::PaperBatchTester(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "BatchTester")
{
    setupUI();
    loadSettings();
}

void PaperBatchTester::setupUI() {
    auto* layout = new QHBoxLayout(this);

    auto* left = new QHBoxLayout();
    left->setSpacing(6);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"Unit", "Integration", "E2E", "Performance", "Regression"});
    categoryCombo_->setStyleSheet("QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    left->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Suite name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    left->addWidget(inputField_, 1);

    runBtn_ = new QPushButton("Run");
    runBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(runBtn_, &QPushButton::clicked, this, &PaperBatchTester::onRun);
    left->addWidget(runBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperBatchTester::onClear);
    left->addWidget(clearBtn_);

    infoLabel_ = new QLabel("Suites: 0 | Passed: 0 | Avg Rate: 0.0%");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    left->addWidget(infoLabel_);

    layout->addLayout(left, 1);
    layout->addStretch();

    setMinimumSize(680, 520);
}

void PaperBatchTester::addEntry(const BatchTestEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit suiteComplete(entry.id, entry.passRate);
    update();
}

QList<BatchTestEntry> PaperBatchTester::entries() const { return entries_; }

int PaperBatchTester::passedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.passed) c++;
    return c;
}

qreal PaperBatchTester::avgPassRate() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.passRate;
    return sum / entries_.size();
}

QMap<QString, int> PaperBatchTester::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperBatchTester::onRun() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"Unit", "Integration", "E2E", "Performance", "Regression"};
    QString category = categories[categoryCombo_->currentIndex()];

    BatchTestEntry e;
    e.id = entries_.size() + 1;
    e.suite = text;
    e.category = category;
    e.passRate = 0.5 + QRandomGenerator::global()->bounded(500) / 1000.0;
    e.total = 10 + QRandomGenerator::global()->bounded(491);
    e.failed = static_cast<int>(e.total * (1.0 - e.passRate));
    e.passed = e.passRate >= 0.95;

    QStringList statuses = {"passed", "failed", "flaky", "skipped"};
    e.status = e.passed ? "passed" : statuses[QRandomGenerator::global()->bounded(statuses.size())];

    if (e.status == "passed") e.color = QColor(22, 163, 106);
    else if (e.status == "failed") e.color = QColor(220, 38, 38);
    else if (e.status == "flaky") e.color = QColor(217, 119, 6);
    else e.color = QColor(124, 58, 237);

    addEntry(e);
    inputField_->clear();
}

void PaperBatchTester::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
    repaint();
}

void PaperBatchTester::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Run batch test suites");
        return;
    }

    int w = width(), h = height();
    int colW = (w - 40) / 3;

    drawTestList(p, QRect(10, 10, colW, h - 20));
    drawCategoryChart(p, QRect(20 + colW, 10, colW, h - 20));
    drawStats(p, QRect(30 + colW * 2, 10, colW - 10, h - 20));
}

void PaperBatchTester::drawTestList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x() + 8, rect.y() + 20, "Batch Tests");

    int show = qMin(12, entries_.size());
    int topMargin = 30;
    int itemH = qMin(36, (rect.height() - topMargin - 10) / qMax(show, 1));
    int barMaxW = rect.width() - 16;

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + topMargin + i * (itemH + 3);

        // Background bar (pass rate track)
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(241, 245, 249));
        p.drawRoundedRect(rect.x() + 8, y, barMaxW, itemH, 4, 4);

        // Pass rate fill bar
        int fillW = static_cast<int>(e.passRate * barMaxW);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x() + 8, y, fillW, itemH, 4, 4);

        // Suite name
        p.setPen(QColor(255, 255, 255));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 14, y + 2, barMaxW / 2, 16, Qt::AlignVCenter,
                   e.suite.left(20));

        // Stats line
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 14, y + 18, barMaxW / 2, 14, Qt::AlignVCenter,
                   e.status + " | " + QString::number(e.failed) + " failed");

        // Pass rate percentage
        p.setPen(QColor(255, 255, 255));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + barMaxW / 2, y + 2, barMaxW / 2 - 14, 18,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.passRate * 100, 'f', 1) + "%");

        // Total count
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + barMaxW / 2, y + 18, barMaxW / 2 - 14, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.total) + " tests");
    }
}

void PaperBatchTester::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x() + 8, rect.y() + 20, "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"Unit", "Integration", "E2E", "Performance", "Regression"};
    QColor colors[] = {
        QColor(59, 130, 246), QColor(22, 163, 106),
        QColor(217, 119, 6), QColor(220, 38, 38), QColor(124, 58, 237)
    };

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int topMargin = 35;
    int barH = qMin(24, (rect.height() - topMargin - 10) / 5);

    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + topMargin + i * (barH + 4);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 8, y, 70, barH, Qt::AlignVCenter | Qt::AlignRight, cats[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i].lighter(170));
        p.drawRoundedRect(rect.x() + 80, y, rect.width() - 110, barH, 3, 3);

        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 80, y, barW, barH, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 84 + barW, y + barH / 2 - 6, QString::number(count));
    }
}

void PaperBatchTester::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Suites", QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Passed", QString::number(passedCount()), QColor(22, 163, 106)},
        {"Avg Pass Rate", QString::number(avgPassRate() * 100, 'f', 1) + "%", QColor(217, 119, 6)}
    };

    int boxH = qMin(48, (rect.height() - 60) / 3);

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x() + 8, rect.y() + 20, "Stats");

    int topMargin = 32;
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + topMargin + i * (boxH + 6);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x() + 8, y, rect.width() - 16, boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 16, QFont::Bold));
        p.drawText(rect.x() + 16, y + 4, rect.width() - 32, 26, Qt::AlignVCenter,
                   stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 16, y + 28, rect.width() - 32, 16, Qt::AlignVCenter,
                   stats[i].label);
    }
}

void PaperBatchTester::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Suites: 0 | Passed: 0 | Avg Rate: 0.0%");
        return;
    }
    infoLabel_->setText(QString("Suites: %1 | Passed: %2 | Avg Rate: %3%")
        .arg(entries_.size())
        .arg(passedCount())
        .arg(avgPassRate() * 100, 0, 'f', 1));
}

void PaperBatchTester::loadSettings() {
    settings_.beginGroup("BatchTester");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        BatchTestEntry e;
        e.id = settings_.value("id").toInt();
        e.suite = settings_.value("suite").toString();
        e.category = settings_.value("category").toString();
        e.status = settings_.value("status").toString();
        e.passRate = settings_.value("passRate").toDouble();
        e.total = settings_.value("total").toInt();
        e.failed = settings_.value("failed").toInt();
        e.passed = settings_.value("passed").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperBatchTester::saveSettings() {
    settings_.beginGroup("BatchTester");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("suite", entries_[i].suite);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("status", entries_[i].status);
        settings_.setValue("passRate", entries_[i].passRate);
        settings_.setValue("total", entries_[i].total);
        settings_.setValue("failed", entries_[i].failed);
        settings_.setValue("passed", entries_[i].passed);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
