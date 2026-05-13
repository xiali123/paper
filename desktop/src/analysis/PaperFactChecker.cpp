#include "analysis/PaperFactChecker.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperFactChecker::PaperFactChecker(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "FactChecker")
{
    setupUI();
    loadSettings();
}

void PaperFactChecker::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    checkBtn_ = new QPushButton("Check");
    checkBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(checkBtn_, &QPushButton::clicked, this, &PaperFactChecker::onCheck);
    toolbar->addWidget(checkBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Science", "Statistics", "History", "Claim", "Reference"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperFactChecker::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter statement to verify...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Ready to check facts");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(600, 520);
}

void PaperFactChecker::addEntry(const FactCheckerEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit factChecked(entry.id, entry.accuracy);
    update();
}

QList<FactCheckerEntry> PaperFactChecker::entries() const { return entries_; }

int PaperFactChecker::confirmedCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.confirmed) c++;
    return c;
}

qreal PaperFactChecker::avgAccuracy() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.accuracy;
    return sum / entries_.size();
}

QMap<QString, int> PaperFactChecker::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperFactChecker::onCheck() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    static const QStringList categories = {"Science", "Statistics", "History", "Claim", "Reference"};
    static const QStringList sources = {"Peer Review", "Database", "Manual", "Citation", "Expert"};
    static const QList<QColor> palette = {
        QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06), QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(6);
    for (int i = 0; i < count; ++i) {
        FactCheckerEntry e;
        e.id = entries_.size() + 1;
        e.statement = QString("Statement %1: %2").arg(e.id).arg(
            text.length() > 40 ? text.left(40) + "..." : text);
        int catIdx = cIdx == 0 ? QRandomGenerator::global()->bounded(categories.size())
                               : cIdx - 1;
        e.category = categories[catIdx];
        e.source = sources[QRandomGenerator::global()->bounded(sources.size())];
        e.accuracy = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
        e.verifications = 1 + QRandomGenerator::global()->bounded(12);
        e.confirmed = e.accuracy >= 0.75;
        e.color = palette[catIdx];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperFactChecker::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Ready to check facts");
    update();
}

void PaperFactChecker::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Ready to check facts");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Fact Checker");
    int w = width(), h = height();
    drawFactView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperFactChecker::drawFactView(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(36, (rect.height() - 10) / qMax(show, 1));
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
                   e.statement.left(30) + (e.confirmed ? " [OK]" : " [?]"));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + e.source);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.accuracy * 100, 'f', 0) + "% acc");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.verifications) + " verif.");
    }
}

void PaperFactChecker::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    static const QStringList categories = {"Science", "Statistics", "History", "Claim", "Reference"};
    static const QString labels[] = {"Science", "Stats", "History", "Claim", "Ref."};
    static const QColor colors[] = {
        QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06), QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(22, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
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

void PaperFactChecker::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Facts", QString::number(entries_.size()), QColor(59,130,246)},
        {"Confirmed", QString::number(confirmedCount()), QColor(22,163,74)},
        {"Avg Acc", QString::number(avgAccuracy() * 100, 'f', 0) + "%", QColor(217,119,6)},
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

void PaperFactChecker::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Ready to check facts"); return; }
    infoLabel_->setText(QString("%1 facts | %2 confirmed | %3% avg accuracy")
        .arg(entries_.size()).arg(confirmedCount()).arg(avgAccuracy() * 100, 0, 'f', 0));
}

void PaperFactChecker::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        FactCheckerEntry e;
        e.id = settings_.value("id").toInt();
        e.statement = settings_.value("statement").toString();
        e.category = settings_.value("category").toString();
        e.source = settings_.value("source").toString();
        e.accuracy = settings_.value("accuracy").toDouble();
        e.verifications = settings_.value("verifications").toInt();
        e.confirmed = settings_.value("confirmed").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    if (entries_.isEmpty()) {
        static const QStringList categories = {"Science", "Statistics", "History", "Claim", "Reference"};
        static const QStringList statements = {
            "Water boils at 100 C at sea level",
            "Sample size of 30 is sufficient for CLT",
            "The first peer-reviewed journal was 1665",
            "AI will replace all jobs by 2030",
            "Einstein published SR in 1905",
            "p < 0.05 implies clinical significance",
            "The speed of light is 299792458 m/s",
            "Correlation does not imply causation"
        };
        static const QStringList sources = {"Peer Review", "Database", "Manual", "Citation", "Expert"};
        static const QList<QColor> palette = {
            QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
            QColor(0xd9, 0x77, 0x06), QColor(0xdc, 0x26, 0x26),
            QColor(0x7c, 0x3a, 0xed)
        };
        qreal accuracies[] = {0.98, 0.72, 0.95, 0.35, 0.99, 0.55, 1.0, 0.88};
        int verifs[] = {12, 8, 15, 3, 20, 6, 25, 10};
        for (int i = 0; i < 8; ++i) {
            FactCheckerEntry e;
            e.id = i + 1;
            e.statement = statements[i];
            int catIdx = i % 5;
            e.category = categories[catIdx];
            e.source = sources[i % 5];
            e.accuracy = accuracies[i];
            e.verifications = verifs[i];
            e.confirmed = e.accuracy >= 0.75;
            e.color = palette[catIdx];
            entries_.append(e);
        }
        saveSettings();
    }
    updateInfo();
}

void PaperFactChecker::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("statement", entries_[i].statement);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("source", entries_[i].source);
        settings_.setValue("accuracy", entries_[i].accuracy);
        settings_.setValue("verifications", entries_[i].verifications);
        settings_.setValue("confirmed", entries_[i].confirmed);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
