#include "analysis/PaperHypothesisGrid.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperHypothesisGrid::PaperHypothesisGrid(QWidget* parent)
    : QWidget(parent), settings_("PaperCrawler", "HypothesisGrid") {
    setupUI();
    loadSettings();
}

void PaperHypothesisGrid::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    testBtn_ = new QPushButton("Test", this);
    testBtn_->setStyleSheet("QPushButton { background-color: #3b82f6; color: white; padding: 5px 14px; border-radius: 4px; }");
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Theory", "Empirical", "Methodology", "Statistical", "Review"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Hypothesis...");
    clearBtn_ = new QPushButton("Clear", this);
    clearBtn_->setStyleSheet("QPushButton { background-color: #dc2626; color: white; padding: 5px 14px; border-radius: 4px; }");
    infoLabel_ = new QLabel("Hypotheses: 0 | Proven: 0 | Avg Confidence: 0.00", this);
    toolbar->addWidget(testBtn_);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);
    connect(testBtn_, &QPushButton::clicked, this, &PaperHypothesisGrid::onTest);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperHypothesisGrid::onClear);
}

void PaperHypothesisGrid::addEntry(const HypothesisEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<HypothesisEntry> PaperHypothesisGrid::entries() const { return entries_; }

int PaperHypothesisGrid::provenCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.proven) c++;
    return c;
}

qreal PaperHypothesisGrid::avgConfidence() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.confidence;
    return sum / entries_.size();
}

QMap<QString, int> PaperHypothesisGrid::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperHypothesisGrid::onTest() {
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    QStringList categories = {"Theory", "Empirical", "Methodology", "Statistical", "Review"};
    QStringList statuses = {"Supported", "Refuted", "Inconclusive", "Partial"};
    int count = 4 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        HypothesisEntry e;
        e.id = entries_.size() + 1;
        QString input = inputField_->text().trimmed();
        if (!input.isEmpty()) {
            e.hypothesis = QString("H%1: %2").arg(e.id).arg(input);
        } else {
            e.hypothesis = QString("Hypothesis_%1").arg(e.id);
        }
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.status = statuses[QRandomGenerator::global()->bounded(statuses.size())];
        e.confidence = QRandomGenerator::global()->bounded(0.1, 1.0);
        e.tests = QRandomGenerator::global()->bounded(1, 20);
        e.proven = e.confidence > 0.75 && e.status == "Supported";
        e.color = colors[e.id % colors.size()];
        entries_.append(e);
        emit hypothesisTested(e.id, e.confidence);
    }
    updateInfo();
    saveSettings();
    update();
}

void PaperHypothesisGrid::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperHypothesisGrid::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawGrid(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryChart(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperHypothesisGrid::drawGrid(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Hypothesis Grid:");
    int y = rect.top() + 20;
    for (int i = 0; i < qMin(entries_.size(), 10); ++i) {
        const auto& e = entries_[i];
        p.setPen(e.color);
        p.setBrush(e.color);
        p.drawEllipse(rect.left(), y + 1, 8, 8);
        p.setPen(QColor(0x334155));
        QString provenTag = e.proven ? "[PROVEN]" : "";
        QString text = QString("%1 | %2 | %3 | Conf: %4 | Tests: %5 %6")
            .arg(e.hypothesis, e.category, e.status)
            .arg(QString::number(e.confidence, 'f', 2))
            .arg(e.tests)
            .arg(provenTag);
        p.drawText(rect.left() + 14, y + 9, text);
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperHypothesisGrid::drawCategoryChart(QPainter& p, const QRect& rect) {
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
        p.drawRoundedRect(rect.left(), y, qMin(it.value() * 25, rect.width()), 14, 3, 3);
        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + 4, y + 12, QString("%1: %2").arg(it.key()).arg(it.value()));
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperHypothesisGrid::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Proven: %1").arg(provenCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Avg Confidence: %1").arg(QString::number(avgConfidence(), 'f', 2)));
}

void PaperHypothesisGrid::updateInfo() {
    infoLabel_->setText(QString("Hypotheses: %1 | Proven: %2 | Avg Confidence: %3")
        .arg(entries_.size()).arg(provenCount())
        .arg(QString::number(avgConfidence(), 'f', 2)));
}

void PaperHypothesisGrid::loadSettings() {
    settings_.beginGroup("HypothesisGrid");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        HypothesisEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.hypothesis = settings_.value(QString("hypothesis_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.status = settings_.value(QString("status_%1").arg(i)).toString();
        e.confidence = settings_.value(QString("confidence_%1").arg(i)).toDouble();
        e.tests = settings_.value(QString("tests_%1").arg(i)).toInt();
        e.proven = settings_.value(QString("proven_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperHypothesisGrid::saveSettings() {
    settings_.beginGroup("HypothesisGrid");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("hypothesis_%1").arg(i), e.hypothesis);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("status_%1").arg(i), e.status);
        settings_.setValue(QString("confidence_%1").arg(i), e.confidence);
        settings_.setValue(QString("tests_%1").arg(i), e.tests);
        settings_.setValue(QString("proven_%1").arg(i), e.proven);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
