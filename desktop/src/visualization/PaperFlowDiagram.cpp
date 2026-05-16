#include "visualization/PaperFlowDiagram.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperFlowDiagram::PaperFlowDiagram(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "FlowDiagram")
{
    setupUI();
    loadSettings();
}

void PaperFlowDiagram::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperFlowDiagram::onGenerate);
    toolbar->addWidget(generateBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Citation", "Reference", "Influence", "Topic"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperFlowDiagram::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter flow label...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Generate flow diagram");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(600, 500);
}

void PaperFlowDiagram::addEntry(const FlowEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit flowUpdated(entry.id, entry.value);
    update();
}

QList<FlowEntry> PaperFlowDiagram::entries() const { return entries_; }

qreal PaperFlowDiagram::totalFlow() const {
    qreal t = 0;
    for (const auto& e : entries_) t += e.value;
    return t;
}

int PaperFlowDiagram::activeCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.active) c++;
    return c;
}

QMap<QString, int> PaperFlowDiagram::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperFlowDiagram::onGenerate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"citation", "reference", "influence", "topic"};
    QStringList sources = {"paper-A", "paper-B", "paper-C", "paper-D", "paper-E"};
    QStringList targets = {"topic-X", "topic-Y", "method-Z", "field-W", "author-Q"};
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(5);
    for (int i = 0; i < count; ++i) {
        FlowEntry e;
        e.id = entries_.size() + 1;
        e.source = sources[QRandomGenerator::global()->bounded(sources.size())];
        e.target = targets[QRandomGenerator::global()->bounded(targets.size())];
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.value = 10 + QRandomGenerator::global()->bounded(90);
        e.capacity = 50 + QRandomGenerator::global()->bounded(50);
        e.active = e.value > 0;
        e.color = e.value >= e.capacity * 0.7 ? QColor(239,68,68) : (e.value >= e.capacity * 0.4 ? QColor(59,130,246) : QColor(16,185,129));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperFlowDiagram::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Generate flow diagram");
    update();
}

void PaperFlowDiagram::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Generate flow diagram");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Flow Diagram");
    int w = width(), h = height();
    drawFlowView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperFlowDiagram::drawFlowView(QPainter& p, const QRect& rect) {
    int n = entries_.size();
    int show = qMin(8, n);
    int laneH = qMin(28, (rect.height() - 20) / qMax(show, 1));
    qreal maxVal = 1;
    for (int i = 0; i < show; ++i) maxVal = qMax(maxVal, entries_[i].capacity);
    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + 10 + i * (laneH + 4);
        qreal ratio = e.value / maxVal;
        int flowW = static_cast<int>(ratio * (rect.width() - 100));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 6));
        p.drawText(rect.x(), y, 45, laneH, Qt::AlignVCenter | Qt::AlignRight, e.source.left(7));
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(203, 213, 225));
        p.drawRoundedRect(rect.x() + 50, y + 4, rect.width() - 100, laneH - 8, 4, 4);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x() + 50, y + 4, flowW, laneH - 8, 4, 4);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 6));
        p.drawText(rect.x() + rect.width() - 45, y, 45, laneH, Qt::AlignVCenter | Qt::AlignLeft, e.target.left(7));
    }
}

void PaperFlowDiagram::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"citation", "reference", "influence", "topic"};
    QString labels[] = {"Citation", "Reference", "Influence", "Topic"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int itemH = qMin(28, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (itemH + 4);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 5, y + 4, 14, 14, 3, 3);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18, Qt::AlignVCenter, labels[i]);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight, QString::number(count) + " flows");
    }
}

void PaperFlowDiagram::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Flows", QString::number(entries_.size()), QColor(59,130,246)},
        {"Active", QString::number(activeCount()), QColor(16,185,129)},
        {"Total", QString::number(totalFlow(), 'f', 0), QColor(245,158,11)},
        {"Categories", QString::number(categoryCounts().size()), QColor(139,92,246)}
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

void PaperFlowDiagram::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Generate flow diagram"); return; }
    infoLabel_->setText(QString("%1 flows | %2 active | %3 total")
        .arg(entries_.size()).arg(activeCount()).arg(totalFlow(), 0, 'f', 0));
}

void PaperFlowDiagram::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        FlowEntry e;
        e.id = settings_.value("id").toInt();
        e.source = settings_.value("source").toString();
        e.target = settings_.value("target").toString();
        e.category = settings_.value("category").toString();
        e.value = settings_.value("value").toDouble();
        e.capacity = settings_.value("capacity").toDouble();
        e.active = settings_.value("active").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperFlowDiagram::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("source", entries_[i].source);
        settings_.setValue("target", entries_[i].target);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("value", entries_[i].value);
        settings_.setValue("capacity", entries_[i].capacity);
        settings_.setValue("active", entries_[i].active);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
