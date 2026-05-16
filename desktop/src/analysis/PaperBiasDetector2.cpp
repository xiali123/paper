#include "analysis/PaperBiasDetector2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperBiasDetector2::PaperBiasDetector2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "BiasDetector2")
{
    setupUI();
    loadSettings();
}

void PaperBiasDetector2::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    detectBtn_ = new QPushButton("Detect");
    detectBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(detectBtn_, &QPushButton::clicked, this, &PaperBiasDetector2::onDetect);
    toolbar->addWidget(detectBtn_);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Selection", "Confirmation", "Reporting", "Language"});
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter claim text...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperBiasDetector2::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Detect biases in claims");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperBiasDetector2::addEntry(const Bias2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit biasFound(entry.id, entry.severity);
    update();
}

QList<Bias2Entry> PaperBiasDetector2::entries() const { return entries_; }

int PaperBiasDetector2::confirmedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.confirmed) c++;
    return c;
}

qreal PaperBiasDetector2::avgSeverity() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.severity;
    return sum / entries_.size();
}

QMap<QString, int> PaperBiasDetector2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperBiasDetector2::onDetect() {
    QStringList categories = {"Selection", "Confirmation", "Reporting", "Language"};
    QStringList biasTypes = {"sampling", "cherry-pick", "p-hack", "framing", "anchor"};
    QStringList claims = {
        "Sample not representative",
        "Only supportive data shown",
        "Selective reporting of outcomes",
        "Loaded language in abstract",
        "Overreliance on initial findings",
        "Insufficient control group",
        "Correlation presented as causation"
    };
    QColor palette[] = {QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"), QColor("#dc2626"), QColor("#7c3aed")};
    int cIdx = categoryCombo_->currentIndex();
    int count = 4 + QRandomGenerator::global()->bounded(4);

    for (int i = 0; i < count; ++i) {
        Bias2Entry e;
        e.id = entries_.size() + 1;
        e.claim = claims[QRandomGenerator::global()->bounded(claims.size())];
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.biasType = biasTypes[QRandomGenerator::global()->bounded(biasTypes.size())];
        e.severity = 0.2 + QRandomGenerator::global()->bounded(80) / 100.0;
        e.instances = 1 + QRandomGenerator::global()->bounded(10);
        e.confirmed = e.severity >= 0.7;
        e.color = palette[QRandomGenerator::global()->bounded(5)];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperBiasDetector2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Detect biases in claims");
    update();
}

void PaperBiasDetector2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Detect biases in claims");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Bias Detector 2");

    int w = width(), h = height();
    drawBiasMap(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperBiasDetector2::drawBiasMap(QPainter& p, const QRect& rect) {
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
                   e.biasType + (e.confirmed ? " [!]" : ""));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.claim.left(28) + " | " + e.category);

        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.severity * 100, 'f', 0) + "% sev");

        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.instances) + "x");
    }
}

void PaperBiasDetector2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList categories = {"Selection", "Confirmation", "Reporting", "Language"};
    QString labels[] = {"Select", "Confirm", "Report", "Language"};
    QColor colors[] = {QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"), QColor("#dc2626")};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
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

void PaperBiasDetector2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Biases", QString::number(entries_.size()), QColor("#3b82f6")},
        {"Confirmed", QString::number(confirmedCount()), QColor("#dc2626")},
        {"Avg Severity", QString::number(avgSeverity() * 100, 'f', 0) + "%", QColor("#d97706")},
        {"Categories", QString::number(categoryCounts().size()), QColor("#7c3aed")}
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

void PaperBiasDetector2::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Detect biases in claims"); return; }
    infoLabel_->setText(QString("%1 biases | %2 confirmed | %3% avg sev")
        .arg(entries_.size()).arg(confirmedCount()).arg(avgSeverity() * 100, 0, 'f', 0));
}

void PaperBiasDetector2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        Bias2Entry e;
        e.id = settings_.value("id").toInt();
        e.claim = settings_.value("claim").toString();
        e.category = settings_.value("category").toString();
        e.biasType = settings_.value("biasType").toString();
        e.severity = settings_.value("severity").toDouble();
        e.instances = settings_.value("instances").toInt();
        e.confirmed = settings_.value("confirmed").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperBiasDetector2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("claim", entries_[i].claim);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("biasType", entries_[i].biasType);
        settings_.setValue("severity", entries_[i].severity);
        settings_.setValue("instances", entries_[i].instances);
        settings_.setValue("confirmed", entries_[i].confirmed);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
