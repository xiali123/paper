#include "analysis/PaperSemanticMapper.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperSemanticMapper::PaperSemanticMapper(QWidget* parent) : QWidget(parent) { setupUI(); loadSettings(); }

void PaperSemanticMapper::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Entity", "Relation", "Attribute", "Event", "Concept"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Concept name...");
    mapBtn_ = new QPushButton("Map", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Concepts: 0 | Core: 0 | Avg Similarity: 0.00", this);
    toolbar->addWidget(categoryCombo_); toolbar->addWidget(inputField_);
    toolbar->addWidget(mapBtn_); toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar); mainLayout->addWidget(infoLabel_);
    connect(mapBtn_, &QPushButton::clicked, this, &PaperSemanticMapper::onMap);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperSemanticMapper::onClear);
}

void PaperSemanticMapper::addEntry(const SemanticEntry& entry) { entries_.append(entry); updateInfo(); update(); }
QList<SemanticEntry> PaperSemanticMapper::entries() const { return entries_; }
int PaperSemanticMapper::coreCount() const { int c = 0; for (const auto& e : entries_) if (e.core) c++; return c; }
qreal PaperSemanticMapper::avgSimilarity() const { if (entries_.isEmpty()) return 0.0; qreal s = 0; for (const auto& e : entries_) s += e.similarity; return s / entries_.size(); }
QMap<QString, int> PaperSemanticMapper::categoryCounts() const { QMap<QString, int> m; for (const auto& e : entries_) m[e.category]++; return m; }

void PaperSemanticMapper::onMap() {
    SemanticEntry e;
    e.id = entries_.size() + 1;
    e.concept = inputField_->text().trimmed();
    if (e.concept.isEmpty()) e.concept = QString("Concept_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    QStringList rels = {"is-a", "part-of", "related-to", "causes", "derived-from"};
    e.relation = rels[QRandomGenerator::global()->bounded(rels.size())];
    e.similarity = QRandomGenerator::global()->bounded(0.0, 1.0);
    e.distance = 1.0 - e.similarity;
    e.neighbors = QRandomGenerator::global()->bounded(1, 50);
    e.core = e.similarity > 0.7;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e); updateInfo(); saveSettings();
    emit conceptMapped(e.id, e.similarity); update();
}

void PaperSemanticMapper::onClear() { entries_.clear(); updateInfo(); saveSettings(); update(); }

void PaperSemanticMapper::paintEvent(QPaintEvent*) {
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(0xf8fafc));
    drawSemanticList(p, QRect(10, 50, width() - 20, height() / 2 - 60));
    drawCategoryChart(p, QRect(10, height() / 2, width() / 2 - 10, height() / 2 - 60));
    drawStats(p, QRect(width() / 2 + 10, height() / 2, width() / 2 - 20, height() / 2 - 60));
}

void PaperSemanticMapper::drawSemanticList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155)); p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Semantic Map:");
    int y = rect.top() + 20;
    for (int i = 0; i < qMin(entries_.size(), 8); ++i) {
        const auto& e = entries_[i];
        p.setPen(e.color); p.setBrush(e.color);
        p.drawEllipse(rect.left(), y, 8, 8);
        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + 14, y + 9, QString("%1 | %2 | Sim: %3 | Dist: %4 | %5")
            .arg(e.concept, e.relation)
            .arg(QString::number(e.similarity, 'f', 2))
            .arg(QString::number(e.distance, 'f', 2))
            .arg(e.core ? "Core" : ""));
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperSemanticMapper::drawCategoryChart(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts(); int y = rect.top() + 5;
    p.setPen(QColor(0x334155)); p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Type:"); y += 18;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    int ci = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        p.setBrush(colors[ci++ % colors.size()]);
        p.drawRoundedRect(rect.left(), y, qMin(it.value() * 20, rect.width()), 14, 3, 3);
        p.setPen(QColor(0x334155)); p.drawText(rect.left() + 4, y + 12, QString("%1: %2").arg(it.key()).arg(it.value())); y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperSemanticMapper::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155)); p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total: %1").arg(entries_.size())); y += 16;
    p.drawText(rect.left(), y, QString("Core: %1").arg(coreCount())); y += 16;
    p.drawText(rect.left(), y, QString("Avg Similarity: %1").arg(QString::number(avgSimilarity(), 'f', 3)));
}

void PaperSemanticMapper::updateInfo() {
    infoLabel_->setText(QString("Concepts: %1 | Core: %2 | Avg Similarity: %3")
        .arg(entries_.size()).arg(coreCount()).arg(QString::number(avgSimilarity(), 'f', 2)));
}

void PaperSemanticMapper::loadSettings() {
    settings_.beginGroup("SemanticMapper");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        SemanticEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.concept = settings_.value(QString("concept_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.relation = settings_.value(QString("relation_%1").arg(i)).toString();
        e.similarity = settings_.value(QString("similarity_%1").arg(i)).toDouble();
        e.distance = settings_.value(QString("distance_%1").arg(i)).toDouble();
        e.neighbors = settings_.value(QString("neighbors_%1").arg(i)).toInt();
        e.core = settings_.value(QString("core_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup(); updateInfo();
}

void PaperSemanticMapper::saveSettings() {
    settings_.beginGroup("SemanticMapper"); settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("concept_%1").arg(i), e.concept);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("relation_%1").arg(i), e.relation);
        settings_.setValue(QString("similarity_%1").arg(i), e.similarity);
        settings_.setValue(QString("distance_%1").arg(i), e.distance);
        settings_.setValue(QString("neighbors_%1").arg(i), e.neighbors);
        settings_.setValue(QString("core_%1").arg(i), e.core);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
