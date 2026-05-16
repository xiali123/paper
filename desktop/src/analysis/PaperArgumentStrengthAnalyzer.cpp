#include "analysis/PaperArgumentStrengthAnalyzer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperArgumentStrengthAnalyzer::PaperArgumentStrengthAnalyzer(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ArgumentStrengthAnalyzer")
{
    setupUI();
    loadSettings();
}

void PaperArgumentStrengthAnalyzer::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    analyzeBtn_ = new QPushButton("Analyze");
    analyzeBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(analyzeBtn_, &QPushButton::clicked, this, &PaperArgumentStrengthAnalyzer::onAnalyze);
    toolbar->addWidget(analyzeBtn_);

    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Strong", "Moderate", "Weak"});
    toolbar->addWidget(filterCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperArgumentStrengthAnalyzer::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter argument or claim to analyze...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Analyze argument strength");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperArgumentStrengthAnalyzer::addArgument(const ArgumentEntry& entry) {
    arguments_.append(entry);
    saveSettings();
    updateInfo();
    emit argumentAnalyzed(entry.id, entry.strength);
    update();
}

QList<ArgumentEntry> PaperArgumentStrengthAnalyzer::arguments() const { return arguments_; }

QMap<QString, int> PaperArgumentStrengthAnalyzer::typeCounts() const {
    QMap<QString, int> counts;
    for (const auto& a : arguments_) counts[a.type]++;
    return counts;
}

qreal PaperArgumentStrengthAnalyzer::avgStrength() const {
    if (arguments_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& a : arguments_) sum += a.strength;
    return sum / arguments_.size();
}

int PaperArgumentStrengthAnalyzer::strongCount() const {
    int c = 0;
    for (const auto& a : arguments_) if (a.strength >= 0.7) c++;
    return c;
}

void PaperArgumentStrengthAnalyzer::onAnalyze() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList types = {"deductive", "inductive", "abductive", "analogical"};
    QColor typeColors[] = {QColor(16,185,129), QColor(59,130,246), QColor(245,158,11), QColor(139,92,246)};
    QStringList evidence = {"empirical", "statistical", "theoretical", "experimental"};

    int count = 2 + QRandomGenerator::global()->bounded(3);
    for (int i = 0; i < count; ++i) {
        ArgumentEntry a;
        a.id = arguments_.size() + 1;
        a.claim = text.left(15) + " claim " + QString::number(a.id);
        int tIdx = QRandomGenerator::global()->bounded(types.size());
        a.type = types[tIdx];
        a.evidence = evidence[QRandomGenerator::global()->bounded(evidence.size())];
        a.strength = 0.2 + QRandomGenerator::global()->bounded(80) / 100.0;
        a.confidence = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
        a.supportingPoints = 1 + QRandomGenerator::global()->bounded(8);
        a.counterArgument = "Counter to " + text.left(8);
        a.color = a.strength >= 0.7 ? QColor(16,185,129) : (a.strength >= 0.4 ? QColor(245,158,11) : QColor(239,68,68));
        addArgument(a);
    }
    inputField_->clear();
}

void PaperArgumentStrengthAnalyzer::onClear() {
    arguments_.clear();
    saveSettings();
    infoLabel_->setText("Analyze argument strength");
    update();
}

void PaperArgumentStrengthAnalyzer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (arguments_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Analyze argument strength");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Argument Strength Analyzer");

    int w = width(), h = height();
    drawArgumentList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawStrengthChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperArgumentStrengthAnalyzer::drawArgumentList(QPainter& p, const QRect& rect) {
    int filterIdx = filterCombo_->currentIndex();
    int show = 0;
    int maxShow = 10;
    int itemH = qMin(34, (rect.height() - 10) / maxShow);

    for (int i = arguments_.size() - 1; i >= 0 && show < maxShow; --i) {
        const auto& a = arguments_[i];
        if (filterIdx == 1 && a.strength < 0.7) continue;
        if (filterIdx == 2 && (a.strength < 0.4 || a.strength >= 0.7)) continue;
        if (filterIdx == 3 && a.strength >= 0.4) continue;

        int y = rect.y() + show * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(a.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(a.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   a.claim.left(18));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   a.type + " | " + QString::number(a.supportingPoints) + " pts");
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(a.strength * 100, 'f', 0) + "% str");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(a.confidence * 100, 'f', 0) + "% conf");
        show++;
    }
}

void PaperArgumentStrengthAnalyzer::drawStrengthChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Strength Distribution");

    auto counts = typeCounts();
    QStringList types = {"deductive", "inductive", "abductive", "analogical"};
    QString labels[] = {"Deductive", "Inductive", "Abductive", "Analogical"};
    QColor colors[] = {QColor(16,185,129), QColor(59,130,246), QColor(245,158,11), QColor(139,92,246)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(types[i]) ? counts[types[i]] : 0;
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

void PaperArgumentStrengthAnalyzer::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Arguments", QString::number(arguments_.size()), QColor(59,130,246)},
        {"Strong", QString::number(strongCount()), QColor(16,185,129)},
        {"Avg Strength", QString::number(avgStrength() * 100, 'f', 0) + "%", QColor(245,158,11)},
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

void PaperArgumentStrengthAnalyzer::updateInfo() {
    if (arguments_.isEmpty()) { infoLabel_->setText("Analyze argument strength"); return; }
    infoLabel_->setText(QString("%1 arguments | %2 strong | %3% avg")
        .arg(arguments_.size()).arg(strongCount()).arg(avgStrength() * 100, 0, 'f', 0));
}

void PaperArgumentStrengthAnalyzer::loadSettings() {
    int size = settings_.beginReadArray("arguments");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ArgumentEntry a;
        a.id = settings_.value("id").toInt();
        a.claim = settings_.value("claim").toString();
        a.evidence = settings_.value("evidence").toString();
        a.strength = settings_.value("strength").toDouble();
        a.type = settings_.value("type").toString();
        a.confidence = settings_.value("confidence").toDouble();
        a.supportingPoints = settings_.value("supportingPoints").toInt();
        a.counterArgument = settings_.value("counterArgument").toString();
        a.color = QColor(settings_.value("color").toString());
        arguments_.append(a);
    }
    settings_.endArray();
    updateInfo();
}

void PaperArgumentStrengthAnalyzer::saveSettings() {
    settings_.beginWriteArray("arguments");
    for (int i = 0; i < arguments_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", arguments_[i].id);
        settings_.setValue("claim", arguments_[i].claim);
        settings_.setValue("evidence", arguments_[i].evidence);
        settings_.setValue("strength", arguments_[i].strength);
        settings_.setValue("type", arguments_[i].type);
        settings_.setValue("confidence", arguments_[i].confidence);
        settings_.setValue("supportingPoints", arguments_[i].supportingPoints);
        settings_.setValue("counterArgument", arguments_[i].counterArgument);
        settings_.setValue("color", arguments_[i].color.name());
    }
    settings_.endArray();
}
