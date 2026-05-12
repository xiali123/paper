#include "reading/PaperReadingHorizon.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperReadingHorizon::PaperReadingHorizon(QWidget* parent)
    : QWidget(parent), settings_("PaperCrawler", "ReadingHorizon") {
    setupUI();
    loadSettings();
}

void PaperReadingHorizon::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    scanBtn_ = new QPushButton("Scan", this);
    scanBtn_->setStyleSheet("background-color: #3b82f6; color: white; border: none; padding: 6px 14px; border-radius: 4px;");
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "AI/ML", "NLP", "Computer Vision", "Robotics", "Security"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Field name...");
    clearBtn_ = new QPushButton("Clear", this);
    clearBtn_->setStyleSheet("background-color: #dc2626; color: white; border: none; padding: 6px 14px; border-radius: 4px;");
    toolbar->addWidget(scanBtn_);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    infoLabel_ = new QLabel("Entries: 0 | Emerging: 0 | Avg Distance: 0.00", this);
    mainLayout->addWidget(infoLabel_);
    connect(scanBtn_, &QPushButton::clicked, this, &PaperReadingHorizon::onScan);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingHorizon::onClear);
}

void PaperReadingHorizon::addEntry(const HorizonEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<HorizonEntry> PaperReadingHorizon::entries() const { return entries_; }

int PaperReadingHorizon::emergingCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.emerging) c++;
    return c;
}

qreal PaperReadingHorizon::avgDistance() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.distance;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingHorizon::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperReadingHorizon::onScan() {
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    QStringList fields = {"Transformers", "Diffusion Models", "Multimodal Learning", "Federated Learning",
                          "Graph Neural Networks", "Reinforcement Learning", "Contrastive Learning",
                          "Neural Architecture Search", "Prompt Engineering", "Knowledge Distillation"};
    QStringList trends = {"Rising", "Stable", "Declining", "Surging"};
    int count = QRandomGenerator::global()->bounded(3, 7);
    for (int i = 0; i < count; ++i) {
        HorizonEntry e;
        e.id = entries_.size() + 1;
        e.field = inputField_->text().trimmed().isEmpty()
            ? fields.at(QRandomGenerator::global()->bounded(fields.size()))
            : inputField_->text().trimmed();
        e.category = categoryCombo_->currentText();
        e.trend = trends.at(QRandomGenerator::global()->bounded(trends.size()));
        e.distance = QRandomGenerator::global()->bounded(0.1, 1.0);
        e.papers = QRandomGenerator::global()->bounded(5, 200);
        e.emerging = QRandomGenerator::global()->bounded(0, 2) == 1;
        e.color = colors[e.id % colors.size()];
        entries_.append(e);
        emit horizonScanned(e.id, e.distance);
    }
    updateInfo();
    saveSettings();
    update();
}

void PaperReadingHorizon::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperReadingHorizon::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawHorizonView(p, QRect(10, 50, w / 2, h - 60));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 20, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperReadingHorizon::drawHorizonView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Horizon View:");
    if (entries_.isEmpty()) return;
    int cx = rect.left() + rect.width() / 2;
    int cy = rect.top() + rect.height() / 2 + 10;
    int radius = qMin(rect.width(), rect.height()) / 2 - 30;
    p.setPen(QPen(QColor(0xe2e8f0), 1));
    for (int r = 1; r <= 4; ++r) {
        p.drawEllipse(cx - radius * r / 4, cy - radius * r / 4, radius * r / 2, radius * r / 2);
    }
    p.setPen(QPen(QColor(0x94a3b8), 1, Qt::DashLine));
    p.drawLine(cx - radius, cy, cx + radius, cy);
    p.drawLine(cx, cy - radius, cx, cy + radius);
    for (const auto& e : entries_) {
        qreal angle = QRandomGenerator::global()->bounded(0.0, 2.0 * M_PI);
        qreal r = (1.0 - e.distance) * radius;
        qreal px = cx + r * qCos(angle);
        qreal py = cy + r * qSin(angle);
        int sz = qBound(4, e.papers / 10, 16);
        p.setBrush(e.color);
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(px, py), sz, sz);
        p.setPen(QColor(0x334155));
        p.setFont(QFont("Sans", 7));
        p.drawText(QPointF(px + sz + 2, py + 4), e.field.left(10));
        if (e.emerging) {
            p.setPen(QPen(QColor(0x16a34a), 1));
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(QPointF(px, py), sz + 3, sz + 3);
        }
    }
    p.setBrush(Qt::NoBrush);
}

void PaperReadingHorizon::drawCategoryChart(QPainter& p, const QRect& rect) {
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
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(rect.left(), y, qMin(it.value() * 20, rect.width()), 14, 3, 3);
        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + 4, y + 12, QString("%1: %2").arg(it.key()).arg(it.value()));
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperReadingHorizon::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Emerging: %1").arg(emergingCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Avg Distance: %1").arg(QString::number(avgDistance(), 'f', 3)));
    y += 16;
    int totalPapers = 0;
    for (const auto& e : entries_) totalPapers += e.papers;
    p.drawText(rect.left(), y, QString("Total Papers: %1").arg(totalPapers));
}

void PaperReadingHorizon::updateInfo() {
    infoLabel_->setText(QString("Entries: %1 | Emerging: %2 | Avg Distance: %3")
        .arg(entries_.size()).arg(emergingCount())
        .arg(QString::number(avgDistance(), 'f', 2)));
}

void PaperReadingHorizon::loadSettings() {
    settings_.beginGroup("ReadingHorizon");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        HorizonEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.field = settings_.value(QString("field_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.trend = settings_.value(QString("trend_%1").arg(i)).toString();
        e.distance = settings_.value(QString("distance_%1").arg(i)).toDouble();
        e.papers = settings_.value(QString("papers_%1").arg(i)).toInt();
        e.emerging = settings_.value(QString("emerging_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperReadingHorizon::saveSettings() {
    settings_.beginGroup("ReadingHorizon");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("field_%1").arg(i), e.field);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("trend_%1").arg(i), e.trend);
        settings_.setValue(QString("distance_%1").arg(i), e.distance);
        settings_.setValue(QString("papers_%1").arg(i), e.papers);
        settings_.setValue(QString("emerging_%1").arg(i), e.emerging);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
