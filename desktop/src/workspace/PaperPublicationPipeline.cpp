#include "workspace/PaperPublicationPipeline.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperPublicationPipeline::PaperPublicationPipeline(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "PublicationPipeline")
{
    setupUI();
    loadSettings();
}

void PaperPublicationPipeline::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    advanceBtn_ = new QPushButton("Advance");
    advanceBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(advanceBtn_, &QPushButton::clicked, this, &PaperPublicationPipeline::onAdvance);
    toolbar->addWidget(advanceBtn_);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Draft", "Review", "Revision", "Accepted", "Published"});
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter paper title...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperPublicationPipeline::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Track publication pipeline stages");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperPublicationPipeline::addEntry(const PipelineEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit stageAdvanced(entry.id, entry.progress);
    update();
}

QList<PipelineEntry> PaperPublicationPipeline::entries() const { return entries_; }

int PaperPublicationPipeline::publishedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.published) c++;
    return c;
}

qreal PaperPublicationPipeline::avgProgress() const {
    if (entries_.isEmpty()) return 0.0;
    qreal total = 0;
    for (const auto& e : entries_) total += e.progress;
    return total / entries_.size();
}

QMap<QString, int> PaperPublicationPipeline::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperPublicationPipeline::onAdvance() {
    QStringList categories = {"Draft", "Review", "Revision", "Accepted", "Published"};
    QStringList stages = {"Writing", "Internal Review", "External Review", "Rebuttal", "Camera Ready", "Online"};
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    int count = 3 + QRandomGenerator::global()->bounded(4);

    for (int i = 0; i < count; ++i) {
        PipelineEntry e;
        e.id = entries_.size() + 1;
        e.paper = QString("Paper-%1").arg(e.id);
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.stage = stages[QRandomGenerator::global()->bounded(stages.size())];
        e.progress = QRandomGenerator::global()->bounded(100);
        e.reviews = QRandomGenerator::global()->bounded(5);
        e.published = (e.progress >= 90);
        e.color = colors[QRandomGenerator::global()->bounded(colors.size())];
        entries_.append(e);
    }

    saveSettings();
    updateInfo();
    if (!entries_.isEmpty())
        emit stageAdvanced(entries_.last().id, entries_.last().progress);
    update();
    inputField_->clear();
}

void PaperPublicationPipeline::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Track publication pipeline stages");
    update();
}

void PaperPublicationPipeline::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Track publication pipeline stages");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Publication Pipeline");

    int w = width(), h = height();
    drawPipelineView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperPublicationPipeline::drawPipelineView(QPainter& p, const QRect& rect) {
    if (entries_.isEmpty()) return;

    int margin = 15;
    int boxW = qMin(90, (rect.width() - 2 * margin) / qMax(1, (entries_.size() * 2 - 1)));
    int boxH = 60;
    int spacing = boxW;
    int totalW = entries_.size() * boxW + (entries_.size() - 1) * spacing;
    int startX = rect.x() + margin + qMax(0, (rect.width() - 2 * margin - totalW) / 2);
    int centerY = rect.y() + rect.height() / 2 - boxH / 2;

    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        int x = startX + i * (boxW + spacing);

        QColor boxColor = e.published ? QColor(0x16a34a).lighter(140) : e.color.lighter(140);
        p.setPen(e.published ? QColor(0x16a34a) : e.color);
        p.setBrush(boxColor);
        p.drawRoundedRect(x, centerY, boxW, boxH, 8, 8);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(x, centerY + 2, boxW, 18, Qt::AlignCenter, e.paper);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(x, centerY + 18, boxW, 14, Qt::AlignCenter, e.stage);

        p.setFont(QFont("Arial", 7));
        p.drawText(x, centerY + 32, boxW, 14, Qt::AlignCenter,
            QString("%1%").arg(static_cast<int>(e.progress)));

        if (e.published) {
            p.setPen(QColor(0x16a34a));
            p.setFont(QFont("Arial", 6, QFont::Bold));
            p.drawText(x, centerY + 46, boxW, 12, Qt::AlignCenter, "PUBLISHED");
        }

        if (i < entries_.size() - 1) {
            int arrowStart = x + boxW + 2;
            int arrowEnd = startX + (i + 1) * (boxW + spacing) - 2;
            int arrowY = centerY + boxH / 2;

            p.setPen(QColor(148, 163, 184));
            p.drawLine(arrowStart, arrowY, arrowEnd, arrowY);
            p.drawLine(arrowEnd, arrowY, arrowEnd - 6, arrowY - 4);
            p.drawLine(arrowEnd, arrowY, arrowEnd - 6, arrowY + 4);
        }
    }
}

void PaperPublicationPipeline::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    int y = rect.y() + 22;
    int ci = 0;
    int maxCount = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it)
        maxCount = qMax(maxCount, it.value());
    if (maxCount == 0) maxCount = 1;

    for (auto it = counts.begin(); it != counts.end(); ++it) {
        QColor c = colors[ci++ % colors.size()];
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        int barW = static_cast<int>((static_cast<qreal>(it.value()) / maxCount) * (rect.width() - 100));
        p.drawRoundedRect(rect.x() + 5, y, barW, 14, 3, 3);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 11, QString("%1: %2").arg(it.key()).arg(it.value()));
        y += 22;
    }
}

void PaperPublicationPipeline::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries", QString::number(entries_.size()), QColor(0x3b82f6)},
        {"Published", QString::number(publishedCount()), QColor(0x16a34a)},
        {"Avg Progress", QString::number(avgProgress(), 'f', 1) + "%", QColor(0xd97706)},
        {"Categories", QString::number(categoryCounts().size()), QColor(0x7c3aed)}
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

void PaperPublicationPipeline::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Track publication pipeline stages");
        return;
    }
    infoLabel_->setText(QString("Entries: %1 | Published: %2 | Avg: %3%")
        .arg(entries_.size())
        .arg(publishedCount())
        .arg(QString::number(avgProgress(), 'f', 1)));
}

void PaperPublicationPipeline::loadSettings() {
    settings_.beginGroup("PublicationPipeline");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        PipelineEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.paper = settings_.value(QString("paper_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.stage = settings_.value(QString("stage_%1").arg(i)).toString();
        e.progress = settings_.value(QString("progress_%1").arg(i)).toDouble();
        e.reviews = settings_.value(QString("reviews_%1").arg(i)).toInt();
        e.published = settings_.value(QString("published_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperPublicationPipeline::saveSettings() {
    settings_.beginGroup("PublicationPipeline");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("paper_%1").arg(i), e.paper);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("stage_%1").arg(i), e.stage);
        settings_.setValue(QString("progress_%1").arg(i), e.progress);
        settings_.setValue(QString("reviews_%1").arg(i), e.reviews);
        settings_.setValue(QString("published_%1").arg(i), e.published);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
