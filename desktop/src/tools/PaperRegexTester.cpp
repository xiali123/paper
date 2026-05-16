#include "tools/PaperRegexTester.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperRegexTester::PaperRegexTester(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "RegexTester")
{
    setupUI();
    loadSettings();
}

void PaperRegexTester::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    testBtn_ = new QPushButton("Test");
    testBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(testBtn_, &QPushButton::clicked, this, &PaperRegexTester::onTest);
    toolbar->addWidget(testBtn_);

    toolbar->addWidget(new QLabel("Filter:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Email", "URL", "Phone", "Custom"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperRegexTester::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter regex pattern...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Test regex patterns");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperRegexTester::addEntry(const RegexEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit regexTested(entry.id, entry.matches);
    update();
}

QList<RegexEntry> PaperRegexTester::entries() const { return entries_; }

int PaperRegexTester::validCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.valid) c++;
    return c;
}

int PaperRegexTester::totalMatches() const {
    int sum = 0;
    for (const auto& e : entries_) sum += e.matches;
    return sum;
}

QMap<QString, int> PaperRegexTester::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperRegexTester::onTest() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"email", "url", "phone", "custom"};
    QStringList testStrs = {
        "user@example.com",
        "https://site.org/page",
        "+1-555-0123",
        "abc123def456",
        "foo@bar.io",
        "http://test.net"
    };

    int count = 3 + QRandomGenerator::global()->bounded(3);
    for (int i = 0; i < count; ++i) {
        RegexEntry e;
        e.id = entries_.size() + 1;
        e.pattern = text.left(16) + " R" + QString::number(i);
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.testStr = testStrs[QRandomGenerator::global()->bounded(testStrs.size())];
        e.matches = QRandomGenerator::global()->bounded(10);
        e.valid = QRandomGenerator::global()->bounded(2) == 0;
        e.caseSensitive = QRandomGenerator::global()->bounded(2) == 0;
        e.color = e.valid ? QColor(16, 185, 129) : QColor(239, 68, 68);
        addEntry(e);
    }
    inputField_->clear();
}

void PaperRegexTester::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Test regex patterns");
    update();
}

void PaperRegexTester::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Test regex patterns");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Regex Tester");

    int w = width(), h = height();
    drawRegexList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperRegexTester::drawRegexList(QPainter& p, const QRect& rect) {
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
                   e.pattern.left(14) + (e.valid ? " [OK]" : " [FAIL]"));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.testStr.left(20) + (e.caseSensitive ? " | CS" : " | CI"));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.matches) + " matches");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.category);
    }
}

void PaperRegexTester::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"email", "url", "phone", "custom"};
    QString labels[] = {"Email", "URL", "Phone", "Custom"};
    QColor colors[] = {QColor(59, 130, 246), QColor(16, 185, 129), QColor(245, 158, 11), QColor(139, 92, 246)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 120));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 75, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 80, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 83 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperRegexTester::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Patterns", QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Valid", QString::number(validCount()), QColor(16, 185, 129)},
        {"Total Matches", QString::number(totalMatches()), QColor(245, 158, 11)},
        {"Categories", QString::number(categoryCounts().size()), QColor(139, 92, 246)}
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

void PaperRegexTester::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Test regex patterns"); return; }
    infoLabel_->setText(QString("%1 patterns | %2 valid | %3 matches")
        .arg(entries_.size()).arg(validCount()).arg(totalMatches()));
}

void PaperRegexTester::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        RegexEntry e;
        e.id = settings_.value("id").toInt();
        e.pattern = settings_.value("pattern").toString();
        e.category = settings_.value("category").toString();
        e.testStr = settings_.value("testStr").toString();
        e.matches = settings_.value("matches").toInt();
        e.valid = settings_.value("valid").toBool();
        e.caseSensitive = settings_.value("caseSensitive").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperRegexTester::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("pattern", entries_[i].pattern);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("testStr", entries_[i].testStr);
        settings_.setValue("matches", entries_[i].matches);
        settings_.setValue("valid", entries_[i].valid);
        settings_.setValue("caseSensitive", entries_[i].caseSensitive);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
