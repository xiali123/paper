#include "reading/PaperReadingVelocity.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReadingVelocity::PaperReadingVelocity(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingVelocity")
{
    setupUI();
    loadSettings();
}

void PaperReadingVelocity::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    measureBtn_ = new QPushButton("Measure");
    measureBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(measureBtn_, &QPushButton::clicked, this, &PaperReadingVelocity::onMeasure);
    toolbar->addWidget(measureBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"ML", "NLP", "CV", "Theory", "Applied"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingVelocity::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter period (e.g. Weekly, Monthly)...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Reading velocity tracker");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperReadingVelocity::addEntry(const VelocityEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit velocityMeasured(entry.id, entry.rate);
    update();
}

QList<VelocityEntry> PaperReadingVelocity::entries() const { return entries_; }

int PaperReadingVelocity::acceleratingCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.accelerating) c++;
    return c;
}

qreal PaperReadingVelocity::avgRate() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.rate;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingVelocity::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperReadingVelocity::onMeasure() {
    QString period = inputField_->text().trimmed();
    if (period.isEmpty()) return;
    QStringList categories = {"ML", "NLP", "CV", "Theory", "Applied"};
    QColor palette[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    int catIdx = categoryCombo_->currentIndex();
    int count = 2 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        VelocityEntry e;
        e.id = entries_.size() + 1;
        e.period = period;
        int ci = (catIdx + i) % categories.size();
        e.category = categories[ci];
        QStringList trends = {"up", "down", "stable"};
        e.trend = trends[QRandomGenerator::global()->bounded(3)];
        e.papers = 1.0 + QRandomGenerator::global()->bounded(200) / 10.0;
        e.pages = 10.0 + QRandomGenerator::global()->bounded(500) / 10.0;
        e.hours = 0.5 + QRandomGenerator::global()->bounded(100) / 10.0;
        e.rate = 0.2 + QRandomGenerator::global()->bounded(50) / 10.0;
        e.accelerating = e.rate > 2.0;
        e.color = palette[ci % 5];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperReadingVelocity::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Reading velocity tracker");
    update();
}

void PaperReadingVelocity::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Reading velocity tracker");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Velocity");
    int w = width(), h = height();
    drawVelocityView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReadingVelocity::drawVelocityView(QPainter& p, const QRect& rect) {
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
                   e.period.left(10) + (e.accelerating ? " [ACC]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   QString::number(e.papers, 'f', 1) + " papers | " + e.category);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.rate, 'f', 1) + " rate");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.hours, 'f', 1) + "h | " + QString::number(e.pages, 'f', 0) + "pg");
    }
}

void PaperReadingVelocity::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"ML", "NLP", "CV", "Theory", "Applied"};
    QString labels[] = {"ML", "NLP", "CV", "Theory", "Applied"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 5);
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

void PaperReadingVelocity::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries", QString::number(entries_.size()), QColor(59,130,246)},
        {"Accelerating", QString::number(acceleratingCount()), QColor(22,163,74)},
        {"Avg Rate", QString::number(avgRate(), 'f', 1) + "/w", QColor(217,119,6)},
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

void PaperReadingVelocity::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Reading velocity tracker"); return; }
    infoLabel_->setText(QString("%1 entries | %2 accelerating | %3 avg rate")
        .arg(entries_.size()).arg(acceleratingCount()).arg(avgRate(), 0, 'f', 1));
}

void PaperReadingVelocity::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        VelocityEntry e;
        e.id = settings_.value("id").toInt();
        e.period = settings_.value("period").toString();
        e.category = settings_.value("category").toString();
        e.trend = settings_.value("trend").toString();
        e.papers = settings_.value("papers").toDouble();
        e.pages = settings_.value("pages").toDouble();
        e.hours = settings_.value("hours").toDouble();
        e.rate = settings_.value("rate").toDouble();
        e.accelerating = settings_.value("accelerating").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingVelocity::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("period", entries_[i].period);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("trend", entries_[i].trend);
        settings_.setValue("papers", entries_[i].papers);
        settings_.setValue("pages", entries_[i].pages);
        settings_.setValue("hours", entries_[i].hours);
        settings_.setValue("rate", entries_[i].rate);
        settings_.setValue("accelerating", entries_[i].accelerating);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
