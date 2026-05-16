#include "analysis/PaperClusterValidator.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperClusterValidator::PaperClusterValidator(QWidget* parent)
    : QWidget(parent) {
    setupUI();
    loadSettings();
}

void PaperClusterValidator::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "K-Means", "DBSCAN", "Hierarchical", "Spectral", "Gaussian"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Cluster name...");
    validateBtn_ = new QPushButton("Validate", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Clusters: 0 | Valid: 0 | Avg Silhouette: 0.00", this);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(validateBtn_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);
    connect(validateBtn_, &QPushButton::clicked, this, &PaperClusterValidator::onValidate);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperClusterValidator::onClear);
}

void PaperClusterValidator::addEntry(const ClusterEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<ClusterEntry> PaperClusterValidator::entries() const { return entries_; }

int PaperClusterValidator::validCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.valid) c++;
    return c;
}

qreal PaperClusterValidator::avgSilhouette() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.silhouette;
    return sum / entries_.size();
}

QMap<QString, int> PaperClusterValidator::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperClusterValidator::onValidate() {
    ClusterEntry e;
    e.id = entries_.size() + 1;
    e.cluster = inputField_->text().trimmed();
    if (e.cluster.isEmpty()) e.cluster = QString("Cluster_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    e.method = categoryCombo_->currentText();
    e.members = QRandomGenerator::global()->bounded(10, 500);
    e.silhouette = QRandomGenerator::global()->bounded(0.0, 1.0);
    e.cohesion = QRandomGenerator::global()->bounded(0.0, 1.0);
    e.valid = e.silhouette > 0.5;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    updateInfo();
    saveSettings();
    emit clusterValidated(e.id, e.silhouette);
    update();
}

void PaperClusterValidator::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperClusterValidator::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawClusterList(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryChart(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperClusterValidator::drawClusterList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Cluster Validation Results:");
    int y = rect.top() + 20;
    for (int i = 0; i < qMin(entries_.size(), 8); ++i) {
        const auto& e = entries_[i];
        p.setPen(e.color);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.left(), y, 8, 8, 2, 2);
        p.setPen(QColor(0x334155));
        QString text = QString("%1 | %2 | Members: %3 | Sil: %4 | %5")
            .arg(e.cluster, e.category)
            .arg(e.members)
            .arg(QString::number(e.silhouette, 'f', 2))
            .arg(e.valid ? "Valid" : "Invalid");
        p.drawText(rect.left() + 14, y + 9, text);
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperClusterValidator::drawCategoryChart(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts();
    int y = rect.top() + 5;
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Method:");
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

void PaperClusterValidator::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Valid: %1").arg(validCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Avg Silhouette: %1").arg(QString::number(avgSilhouette(), 'f', 3)));
}

void PaperClusterValidator::updateInfo() {
    infoLabel_->setText(QString("Clusters: %1 | Valid: %2 | Avg Silhouette: %3")
        .arg(entries_.size()).arg(validCount())
        .arg(QString::number(avgSilhouette(), 'f', 2)));
}

void PaperClusterValidator::loadSettings() {
    settings_.beginGroup("ClusterValidator");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        ClusterEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.cluster = settings_.value(QString("cluster_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.method = settings_.value(QString("method_%1").arg(i)).toString();
        e.members = settings_.value(QString("members_%1").arg(i)).toInt();
        e.silhouette = settings_.value(QString("silhouette_%1").arg(i)).toDouble();
        e.cohesion = settings_.value(QString("cohesion_%1").arg(i)).toDouble();
        e.valid = settings_.value(QString("valid_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperClusterValidator::saveSettings() {
    settings_.beginGroup("ClusterValidator");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("cluster_%1").arg(i), e.cluster);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("method_%1").arg(i), e.method);
        settings_.setValue(QString("members_%1").arg(i), e.members);
        settings_.setValue(QString("silhouette_%1").arg(i), e.silhouette);
        settings_.setValue(QString("cohesion_%1").arg(i), e.cohesion);
        settings_.setValue(QString("valid_%1").arg(i), e.valid);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
