#include "analysis/PaperBiasDetector.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

PaperBiasDetector::PaperBiasDetector(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "BiasDetector")
{
    setupUI();
    loadSettings();
}

void PaperBiasDetector::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperBiasDetector::onAdd);
    toolbar->addWidget(addBtn_);

    scanBtn_ = new QPushButton("Scan");
    connect(scanBtn_, &QPushButton::clicked, this, &PaperBiasDetector::onScan);
    toolbar->addWidget(scanBtn_);

    resolveBtn_ = new QPushButton("Resolve All");
    connect(resolveBtn_, &QPushButton::clicked, this, &PaperBiasDetector::onResolveAll);
    toolbar->addWidget(resolveBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperBiasDetector::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter text to check for bias...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Detect cognitive biases");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperBiasDetector::addBias(const BiasEntry& entry) {
    biases_.append(entry);
    saveSettings();
    updateInfo();
    emit biasDetected(entry.id, entry.biasType);
    update();
}

QList<BiasEntry> PaperBiasDetector::biases() const { return biases_; }

QMap<QString, int> PaperBiasDetector::typeCounts() const {
    QMap<QString, int> counts;
    for (const auto& b : biases_) counts[b.biasType]++;
    return counts;
}

qreal PaperBiasDetector::avgSeverity() const {
    if (biases_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& b : biases_) sum += b.severity;
    return sum / biases_.size();
}

int PaperBiasDetector::unresolvedCount() const {
    int c = 0;
    for (const auto& b : biases_) if (!b.resolved) c++;
    return c;
}

void PaperBiasDetector::onAdd() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    bool ok;
    QStringList types = {"confirmation", "selection", "reporting", "anchoring", "availability"};
    QString type = QInputDialog::getItem(this, "Add Bias", "Type:", types, 0, false, &ok);
    if (!ok) return;

    BiasEntry b;
    b.id = biases_.size() + 1;
    b.text = text;
    b.biasType = type;
    b.severity = 0.2 + QRandomGenerator::global()->bounded(80) / 100.0;
    b.resolved = false;

    QStringList suggestions = {
        "Consider alternative explanations",
        "Seek disconfirming evidence",
        "Use random sampling",
        "Report all findings",
        "Adjust for baseline",
        "Consider base rates"
    };
    b.suggestion = suggestions[QRandomGenerator::global()->bounded(suggestions.size())];

    if (b.severity >= 0.7) b.color = QColor(239, 68, 68);
    else if (b.severity >= 0.4) b.color = QColor(245, 158, 11);
    else b.color = QColor(59, 130, 246);

    addBias(b);
    inputField_->clear();
}

void PaperBiasDetector::onScan() {
    if (biases_.isEmpty()) return;
    int critical = 0;
    for (const auto& b : biases_) if (b.severity >= 0.7) critical++;
    emit scanComplete(biases_.size(), critical);
    update();
}

void PaperBiasDetector::onResolveAll() {
    for (auto& b : biases_) b.resolved = true;
    saveSettings();
    updateInfo();
    update();
}

void PaperBiasDetector::onClear() {
    biases_.clear();
    saveSettings();
    infoLabel_->setText("Detect cognitive biases");
    update();
}

void PaperBiasDetector::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (biases_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Detect cognitive biases");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Bias Detector");

    int w = width(), h = height();
    drawBiasList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawSeverityChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperBiasDetector::drawBiasList(QPainter& p, const QRect& rect) {
    int show = qMin(8, biases_.size());
    int itemH = qMin(46, (rect.height() - 10) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& b = biases_[i];
        int y = rect.y() + i * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(b.resolved ? QColor(248, 250, 252) : b.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(b.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        if (b.resolved) {
            p.setBrush(QColor(16, 185, 129));
            p.drawEllipse(rect.x() + rect.width() - 14, y + 6, 8, 8);
        }

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() - 24, 16, Qt::AlignVCenter,
                   b.text.left(24));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2, 14, Qt::AlignVCenter,
                   b.biasType + " | " + QString::number(b.severity * 100, 'f', 0) + "%");
        p.drawText(rect.x() + 10, y + 34, rect.width() - 20, 12, Qt::AlignVCenter,
                   b.suggestion.left(36));
    }
}

void PaperBiasDetector::drawSeverityChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "By Severity");

    int low = 0, med = 0, high = 0;
    for (const auto& b : biases_) {
        if (b.severity >= 0.7) high++;
        else if (b.severity >= 0.4) med++;
        else low++;
    }
    int counts[] = {low, med, high};
    QString labels[] = {"Low", "Medium", "High"};
    QColor colors[] = {QColor(59,130,246), QColor(245,158,11), QColor(239,68,68)};
    int maxVal = qMax(1, qMax(low, qMax(med, high)));

    int barH = qMin(28, (rect.height() - 30) / 3);
    for (int i = 0; i < 3; ++i) {
        int y = rect.y() + 22 + i * (barH + 4);
        int barW = static_cast<int>((static_cast<qreal>(counts[i]) / maxVal) * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x(), y + barH - 3, 60, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 65, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 68 + barW, y + barH - 3, QString::number(counts[i]));
    }
}

void PaperBiasDetector::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Biases", QString::number(biases_.size()), QColor(59,130,246)},
        {"Unresolved", QString::number(unresolvedCount()), QColor(239,68,68)},
        {"Avg Severity", QString::number(avgSeverity() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Types", QString::number(typeCounts().size()), QColor(139,92,246)}
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

void PaperBiasDetector::updateInfo() {
    if (biases_.isEmpty()) { infoLabel_->setText("Detect cognitive biases"); return; }
    infoLabel_->setText(QString("%1 biases | %2 unresolved | avg: %3%")
        .arg(biases_.size()).arg(unresolvedCount()).arg(avgSeverity() * 100, 0, 'f', 0));
}

void PaperBiasDetector::loadSettings() {
    int size = settings_.beginReadArray("biases");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        BiasEntry b;
        b.id = settings_.value("id").toInt();
        b.text = settings_.value("text").toString();
        b.biasType = settings_.value("biasType").toString();
        b.severity = settings_.value("severity").toDouble();
        b.suggestion = settings_.value("suggestion").toString();
        b.resolved = settings_.value("resolved").toBool();
        b.color = QColor(settings_.value("color").toString());
        biases_.append(b);
    }
    settings_.endArray();
    updateInfo();
}

void PaperBiasDetector::saveSettings() {
    settings_.beginWriteArray("biases");
    for (int i = 0; i < biases_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", biases_[i].id);
        settings_.setValue("text", biases_[i].text);
        settings_.setValue("biasType", biases_[i].biasType);
        settings_.setValue("severity", biases_[i].severity);
        settings_.setValue("suggestion", biases_[i].suggestion);
        settings_.setValue("resolved", biases_[i].resolved);
        settings_.setValue("color", biases_[i].color.name());
    }
    settings_.endArray();
}
