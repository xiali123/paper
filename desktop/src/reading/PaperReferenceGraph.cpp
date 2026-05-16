#include "reading/PaperReferenceGraph.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReferenceGraph::PaperReferenceGraph(QWidget* parent)
    : QWidget(parent) {
    setupUI();
    loadSettings();
}

void PaperReferenceGraph::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Citation", "Co-author", "Topic", "Method", "Dataset"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Paper title...");
    addBtn_ = new QPushButton("Add Reference", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("References: 0 | Seminal: 0 | Avg Relevance: 0.00", this);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(addBtn_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);
    connect(addBtn_, &QPushButton::clicked, this, &PaperReferenceGraph::onAdd);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReferenceGraph::onClear);
}

void PaperReferenceGraph::addEntry(const RefEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<RefEntry> PaperReferenceGraph::entries() const { return entries_; }

int PaperReferenceGraph::seminalCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.seminal) c++;
    return c;
}

qreal PaperReferenceGraph::avgRelevance() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.relevance;
    return sum / entries_.size();
}

QMap<QString, int> PaperReferenceGraph::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperReferenceGraph::onAdd() {
    RefEntry e;
    e.id = entries_.size() + 1;
    e.paper = inputField_->text().trimmed();
    if (e.paper.isEmpty()) e.paper = QString("Paper_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    e.relation = categoryCombo_->currentText();
    e.citations = QRandomGenerator::global()->bounded(0, 10000);
    e.relevance = QRandomGenerator::global()->bounded(0.0, 1.0);
    e.year = QRandomGenerator::global()->bounded(1990, 2026);
    e.seminal = e.citations > 5000;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    updateInfo();
    saveSettings();
    emit referenceAdded(e.id, e.relevance);
    update();
}

void PaperReferenceGraph::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperReferenceGraph::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawRefList(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryChart(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperReferenceGraph::drawRefList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Reference Graph:");
    int y = rect.top() + 20;
    for (int i = 0; i < qMin(entries_.size(), 8); ++i) {
        const auto& e = entries_[i];
        p.setPen(e.color);
        p.setBrush(e.color);
        p.drawEllipse(rect.left(), y, 8, 8);
        p.setPen(QColor(0x334155));
        QString text = QString("%1 | %2 | Cites: %3 | Rel: %4 | %5")
            .arg(e.paper, e.category)
            .arg(e.citations)
            .arg(QString::number(e.relevance, 'f', 2))
            .arg(e.seminal ? "Seminal" : "");
        p.drawText(rect.left() + 14, y + 9, text);
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperReferenceGraph::drawCategoryChart(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts();
    int y = rect.top() + 5;
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Category:");
    y += 18;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    int ci = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        p.setBrush(colors[ci++ % colors.size()]);
        p.drawRoundedRect(rect.left(), y, qMin(it.value() * 20, rect.width()), 14, 3, 3);
        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + 4, y + 12, QString("%1: %2").arg(it.key()).arg(it.value()));
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperReferenceGraph::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Seminal: %1").arg(seminalCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Avg Relevance: %1").arg(QString::number(avgRelevance(), 'f', 3)));
}

void PaperReferenceGraph::updateInfo() {
    infoLabel_->setText(QString("References: %1 | Seminal: %2 | Avg Relevance: %3")
        .arg(entries_.size()).arg(seminalCount())
        .arg(QString::number(avgRelevance(), 'f', 2)));
}

void PaperReferenceGraph::loadSettings() {
    settings_.beginGroup("ReferenceGraph");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        RefEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.paper = settings_.value(QString("paper_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.relation = settings_.value(QString("relation_%1").arg(i)).toString();
        e.citations = settings_.value(QString("citations_%1").arg(i)).toInt();
        e.relevance = settings_.value(QString("relevance_%1").arg(i)).toDouble();
        e.year = settings_.value(QString("year_%1").arg(i)).toInt();
        e.seminal = settings_.value(QString("seminal_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperReferenceGraph::saveSettings() {
    settings_.beginGroup("ReferenceGraph");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("paper_%1").arg(i), e.paper);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("relation_%1").arg(i), e.relation);
        settings_.setValue(QString("citations_%1").arg(i), e.citations);
        settings_.setValue(QString("relevance_%1").arg(i), e.relevance);
        settings_.setValue(QString("year_%1").arg(i), e.year);
        settings_.setValue(QString("seminal_%1").arg(i), e.seminal);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
