#include "analysis/PaperArgumentVisualizer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperArgumentVisualizer::PaperArgumentVisualizer(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ArgumentVisualizer")
{
    setupUI();
    loadSettings();
}

void PaperArgumentVisualizer::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    visualizeBtn_ = new QPushButton("Visualize");
    visualizeBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(visualizeBtn_, &QPushButton::clicked, this, &PaperArgumentVisualizer::onVisualize);
    toolbar->addWidget(visualizeBtn_);
    toolbar->addWidget(new QLabel("Type:"));
    typeCombo_ = new QComboBox();
    typeCombo_->addItems({"All", "Premise", "Conclusion", "Rebuttal", "Warrant"});
    toolbar->addWidget(typeCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperArgumentVisualizer::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter argument text...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Visualize argument structure");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperArgumentVisualizer::addEntry(const ArgumentNode& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit argumentVisualized(entry.id, entry.strength);
    update();
}

QList<ArgumentNode> PaperArgumentVisualizer::entries() const { return entries_; }

qreal PaperArgumentVisualizer::avgStrength() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.strength;
    return sum / entries_.size();
}

int PaperArgumentVisualizer::supportedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.supported) c++;
    return c;
}

QMap<QString, int> PaperArgumentVisualizer::typeCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.type]++;
    return counts;
}

void PaperArgumentVisualizer::onVisualize() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList types = {"premise", "conclusion", "rebuttal", "warrant"};
    QStringList evidence = {"statistical", "empirical", "logical", "anecdotal"};
    int tIdx = typeCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(5);
    for (int i = 0; i < count; ++i) {
        ArgumentNode e;
        e.id = entries_.size() + 1;
        e.claim = text.left(10) + " claim" + QString::number(i);
        e.evidence = evidence[QRandomGenerator::global()->bounded(evidence.size())];
        e.type = tIdx == 0 ? types[QRandomGenerator::global()->bounded(types.size())] : types[tIdx - 1];
        e.strength = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
        e.depth = QRandomGenerator::global()->bounded(3);
        e.supported = e.strength >= 0.6;
        e.color = e.supported ? QColor(16,185,129) : (e.strength >= 0.4 ? QColor(245,158,11) : QColor(239,68,68));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperArgumentVisualizer::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Visualize argument structure");
    update();
}

void PaperArgumentVisualizer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Visualize argument structure");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Argument Visualizer");
    int w = width(), h = height();
    drawArgumentTree(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawTypeChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperArgumentVisualizer::drawArgumentTree(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(34, (rect.height() - 10) / qMax(show, 1));
    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 3);
        int indent = e.depth * 15;
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x() + indent, y, rect.width() - indent, itemH, 4, 4);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x() + indent, y, 4, itemH, 2, 2);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + indent + 10, y + 4, rect.width() / 2 - indent - 10, 16, Qt::AlignVCenter,
                   "[" + e.type.left(4) + "] " + e.claim.left(14) + (e.supported ? " [S]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + indent + 10, y + 20, rect.width() / 2 - indent - 10, 14, Qt::AlignVCenter,
                   e.evidence + " | depth " + QString::number(e.depth));
        int barW = static_cast<int>(e.strength * (rect.width() / 2 - 20));
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x() + rect.width() / 2, y + 10, barW, 14, 3, 3);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(rect.x() + rect.width() / 2 + barW + 4, y + 22, QString::number(e.strength * 100, 'f', 0) + "%");
    }
}

void PaperArgumentVisualizer::drawTypeChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Argument Types");
    auto counts = typeCounts();
    QStringList types = {"premise", "conclusion", "rebuttal", "warrant"};
    QString labels[] = {"Premise", "Concl.", "Rebuttal", "Warrant"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(239,68,68), QColor(139,92,246)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(types[i]) ? counts[types[i]] : 0;
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

void PaperArgumentVisualizer::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Arguments", QString::number(entries_.size()), QColor(59,130,246)},
        {"Supported", QString::number(supportedCount()), QColor(16,185,129)},
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

void PaperArgumentVisualizer::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Visualize argument structure"); return; }
    infoLabel_->setText(QString("%1 args | %2 supported | %3% str")
        .arg(entries_.size()).arg(supportedCount()).arg(avgStrength() * 100, 0, 'f', 0));
}

void PaperArgumentVisualizer::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ArgumentNode e;
        e.id = settings_.value("id").toInt();
        e.claim = settings_.value("claim").toString();
        e.evidence = settings_.value("evidence").toString();
        e.type = settings_.value("type").toString();
        e.strength = settings_.value("strength").toDouble();
        e.depth = settings_.value("depth").toInt();
        e.supported = settings_.value("supported").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperArgumentVisualizer::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("claim", entries_[i].claim);
        settings_.setValue("evidence", entries_[i].evidence);
        settings_.setValue("type", entries_[i].type);
        settings_.setValue("strength", entries_[i].strength);
        settings_.setValue("depth", entries_[i].depth);
        settings_.setValue("supported", entries_[i].supported);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
