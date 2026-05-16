#include "reading/PaperReadingConstellation.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperReadingConstellation::PaperReadingConstellation(QWidget* parent)
    : QWidget(parent), settings_("PaperCrawler", "ReadingConstellation") {
    setupUI();
    loadSettings();
}

void PaperReadingConstellation::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    mapBtn_ = new QPushButton("Map", this);
    mapBtn_->setStyleSheet("QPushButton{background:#3b82f6;color:white;border:none;border-radius:4px;padding:6px 14px;}");
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Theory", "Empirical", "Review", "Methods", "Survey"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Star name...");
    clearBtn_ = new QPushButton("Clear", this);
    clearBtn_->setStyleSheet("QPushButton{background:#dc2626;color:white;border:none;border-radius:4px;padding:6px 14px;}");
    infoLabel_ = new QLabel("Stars: 0 | Core: 0 | Avg Brightness: 0.00", this);
    toolbar->addWidget(mapBtn_);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);
    connect(mapBtn_, &QPushButton::clicked, this, &PaperReadingConstellation::onMap);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingConstellation::onClear);
}

void PaperReadingConstellation::addEntry(const ConstellationEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<ConstellationEntry> PaperReadingConstellation::entries() const { return entries_; }

int PaperReadingConstellation::coreCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.core) c++;
    return c;
}

qreal PaperReadingConstellation::avgBrightness() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.brightness;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingConstellation::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperReadingConstellation::onMap() {
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    int count = QRandomGenerator::global()->bounded(3, 7);
    for (int i = 0; i < count; ++i) {
        ConstellationEntry e;
        e.id = entries_.size() + 1;
        e.star = inputField_->text().trimmed().isEmpty()
            ? QString("Star_%1").arg(e.id)
            : QString("%1_%2").arg(inputField_->text().trimmed()).arg(e.id);
        QStringList categories = {"Theory", "Empirical", "Review", "Methods", "Survey"};
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        QStringList clusters = {"Alpha", "Beta", "Gamma", "Delta"};
        e.cluster = clusters[QRandomGenerator::global()->bounded(clusters.size())];
        e.brightness = QRandomGenerator::global()->bounded(0.2, 1.0);
        e.connections = QRandomGenerator::global()->bounded(1, 8);
        e.core = QRandomGenerator::global()->bounded(0, 100) < 30;
        e.color = colors[e.id % colors.size()];
        entries_.append(e);
        emit starMapped(e.id, e.brightness);
    }
    updateInfo();
    saveSettings();
    update();
}

void PaperReadingConstellation::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperReadingConstellation::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawConstellationView(p, QRect(10, 50, w / 2, h - 60));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 20, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperReadingConstellation::drawConstellationView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Constellation Map:");
    if (entries_.isEmpty()) return;
    int cx = rect.left() + rect.width() / 2;
    int cy = rect.top() + rect.height() / 2 + 10;
    int radius = qMin(rect.width(), rect.height()) / 2 - 30;
    QList<QPointF> positions;
    for (int i = 0; i < entries_.size(); ++i) {
        qreal angle = 2 * M_PI * i / entries_.size() - M_PI / 2;
        qreal r = entries_[i].brightness * radius;
        QPointF pos(cx + r * qCos(angle), cy + r * qSin(angle));
        positions << pos;
        int starSize = entries_[i].core ? 8 : 5;
        p.setPen(Qt::NoPen);
        p.setBrush(entries_[i].color);
        p.drawEllipse(pos, starSize, starSize);
        p.setPen(QColor(0x334155));
        p.setFont(QFont("Sans", 7));
        p.drawText(static_cast<int>(pos.x()) + 8, static_cast<int>(pos.y()) + 4, entries_[i].star.left(10));
    }
    p.setPen(QPen(QColor(0x94a3b8), 1, Qt::DashLine));
    p.setBrush(Qt::NoBrush);
    for (int i = 0; i < positions.size(); ++i) {
        for (int j = i + 1; j < qMin(i + entries_[i].connections, positions.size()); ++j) {
            p.drawLine(positions[i], positions[j]);
        }
    }
}

void PaperReadingConstellation::drawCategoryChart(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts();
    int y = rect.top() + 5;
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Category:");
    y += 18;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    int ci = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        p.setPen(Qt::NoPen);
        p.setBrush(colors[ci++ % colors.size()]);
        p.drawRoundedRect(rect.left(), y, qMin(it.value() * 20, rect.width()), 14, 3, 3);
        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + 4, y + 12, QString("%1: %2").arg(it.key()).arg(it.value()));
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperReadingConstellation::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total Stars: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Core Stars: %1").arg(coreCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Avg Brightness: %1").arg(QString::number(avgBrightness(), 'f', 3)));
}

void PaperReadingConstellation::updateInfo() {
    infoLabel_->setText(QString("Stars: %1 | Core: %2 | Avg Brightness: %3")
        .arg(entries_.size()).arg(coreCount())
        .arg(QString::number(avgBrightness(), 'f', 2)));
}

void PaperReadingConstellation::loadSettings() {
    settings_.beginGroup("Constellation");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        ConstellationEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.star = settings_.value(QString("star_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.cluster = settings_.value(QString("cluster_%1").arg(i)).toString();
        e.brightness = settings_.value(QString("brightness_%1").arg(i)).toDouble();
        e.connections = settings_.value(QString("connections_%1").arg(i)).toInt();
        e.core = settings_.value(QString("core_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperReadingConstellation::saveSettings() {
    settings_.beginGroup("Constellation");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("star_%1").arg(i), e.star);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("cluster_%1").arg(i), e.cluster);
        settings_.setValue(QString("brightness_%1").arg(i), e.brightness);
        settings_.setValue(QString("connections_%1").arg(i), e.connections);
        settings_.setValue(QString("core_%1").arg(i), e.core);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
