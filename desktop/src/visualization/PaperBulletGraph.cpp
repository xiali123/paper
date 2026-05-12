#include "visualization/PaperBulletGraph.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperBulletGraph::PaperBulletGraph(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "BulletGraph")
{
    setupUI();
    loadSettings();
}

void PaperBulletGraph::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperBulletGraph::onRender);
    toolbar->addWidget(renderBtn_);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Research", "Citation", "Impact", "Productivity"});
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter label...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperBulletGraph::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Render bullet graph");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(640, 520);
}

void PaperBulletGraph::addEntry(const BulletEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit bulletClicked(entry.id, entry.actual);
    update();
}

QList<BulletEntry> PaperBulletGraph::entries() const { return entries_; }

int PaperBulletGraph::onTrackCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.onTrack) c++;
    return c;
}

qreal PaperBulletGraph::avgActual() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.actual;
    return sum / entries_.size();
}

QMap<QString, int> PaperBulletGraph::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperBulletGraph::onRender() {
    QString text = inputField_->text().trimmed();

    QStringList categories = {"Research", "Citation", "Impact", "Productivity"};
    QStringList metrics = {"h-index", "citations", "papers", "reads", "downloads"};
    QColor colors[] = {QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"), QColor("#dc2626"), QColor("#7c3aed")};

    entries_.clear();
    int count = 5 + QRandomGenerator::global()->bounded(6);
    for (int i = 0; i < count; ++i) {
        BulletEntry e;
        e.id = entries_.size() + 1;
        e.label = text.isEmpty() ? QString("Metric %1").arg(i + 1) : text.left(12) + " " + QString(QChar('A' + i));
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.metric = metrics[QRandomGenerator::global()->bounded(metrics.size())];
        e.target = 50 + QRandomGenerator::global()->bounded(50);
        e.actual = QRandomGenerator::global()->bounded(static_cast<int>(e.target) + 30);
        e.benchmark = e.target * (0.6 + QRandomGenerator::global()->bounded(40) / 100.0);
        e.onTrack = e.actual >= e.target * 0.8;
        e.color = colors[i % 5];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperBulletGraph::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Render bullet graph");
    update();
}

void PaperBulletGraph::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Render bullet graph");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Bullet Graph");

    int w = width(), h = height();
    drawBulletChart(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperBulletGraph::drawBulletChart(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int barH = qMin(20, (rect.height() - 10) / qMax(show, 1));
    int labelW = 80;
    int barAreaW = rect.width() - labelW - 10;

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (barH + 6);
        int barX = rect.x() + labelW;

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x(), y, labelW, barH, Qt::AlignRight | Qt::AlignVCenter, e.label);

        qreal maxVal = qMax(qMax(e.actual, e.target), e.benchmark) * 1.2;
        if (maxVal < 1.0) maxVal = 1.0;

        int bgW = static_cast<int>((maxVal / maxVal) * barAreaW);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        p.drawRoundedRect(barX, y + 2, bgW, barH - 4, 3, 3);

        int benchW = static_cast<int>((e.benchmark / maxVal) * barAreaW);
        p.setBrush(QColor(191, 219, 254));
        p.drawRoundedRect(barX, y + 4, benchW, barH - 8, 3, 3);

        int targetX = barX + static_cast<int>((e.target / maxVal) * barAreaW);
        p.setPen(QPen(QColor("#dc2626"), 2));
        p.drawLine(targetX, y, targetX, y + barH);

        int actualW = static_cast<int>((e.actual / maxVal) * barAreaW);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(barX, y + 5, actualW, barH - 10, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(barX + actualW + 4, y + barH - 4,
                   QString::number(e.actual, 'f', 0) + "/" + QString::number(e.target, 'f', 0));
    }
}

void PaperBulletGraph::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"Research", "Citation", "Impact", "Productivity"};
    QColor colors[] = {QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"), QColor("#dc2626")};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 120));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 85, barH, Qt::AlignRight | Qt::AlignVCenter, cats[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 90, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 93 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperBulletGraph::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries", QString::number(entries_.size()), QColor("#3b82f6")},
        {"On Track", QString::number(onTrackCount()), QColor("#16a34a")},
        {"Avg Actual", QString::number(avgActual(), 'f', 0), QColor("#d97706")},
        {"Categories", QString::number(categoryCounts().size()), QColor("#7c3aed")}
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

void PaperBulletGraph::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Render bullet graph"); return; }
    infoLabel_->setText(QString("%1 entries | %2 on track | avg %3")
        .arg(entries_.size()).arg(onTrackCount()).arg(avgActual(), 0, 'f', 0));
}

void PaperBulletGraph::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        BulletEntry e;
        e.id = settings_.value("id").toInt();
        e.label = settings_.value("label").toString();
        e.category = settings_.value("category").toString();
        e.metric = settings_.value("metric").toString();
        e.actual = settings_.value("actual").toDouble();
        e.target = settings_.value("target").toDouble();
        e.benchmark = settings_.value("benchmark").toDouble();
        e.onTrack = settings_.value("onTrack").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperBulletGraph::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("label", entries_[i].label);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("metric", entries_[i].metric);
        settings_.setValue("actual", entries_[i].actual);
        settings_.setValue("target", entries_[i].target);
        settings_.setValue("benchmark", entries_[i].benchmark);
        settings_.setValue("onTrack", entries_[i].onTrack);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
