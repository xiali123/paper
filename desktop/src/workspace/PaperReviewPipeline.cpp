#include "workspace/PaperReviewPipeline.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperReviewPipeline::PaperReviewPipeline(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReviewPipeline")
{
    setupUI();
    loadSettings();
}

void PaperReviewPipeline::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    trackBtn_ = new QPushButton("Track");
    trackBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(trackBtn_, &QPushButton::clicked, this, &PaperReviewPipeline::onTrack);
    toolbar->addWidget(trackBtn_);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Submitted", "Under Review", "Revision", "Accepted", "Rejected"});
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter paper title...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReviewPipeline::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Track review pipeline stages");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperReviewPipeline::addEntry(const ReviewPipelineEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit reviewUpdated(entry.id, entry.score);
    update();
}

QList<ReviewPipelineEntry> PaperReviewPipeline::entries() const { return entries_; }

int PaperReviewPipeline::acceptedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.accepted) c++;
    return c;
}

qreal PaperReviewPipeline::avgScore() const {
    if (entries_.isEmpty()) return 0.0;
    qreal total = 0;
    for (const auto& e : entries_) total += e.score;
    return total / entries_.size();
}

QMap<QString, int> PaperReviewPipeline::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperReviewPipeline::onTrack() {
    QStringList categories = {"Submitted", "Under Review", "Revision", "Accepted", "Rejected"};
    QStringList stages = {"Initial Check", "Editor Assignment", "Peer Review", "Rebuttal", "Decision", "Camera Ready"};
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};

    if (!entries_.isEmpty()) {
        // Add entries seeded from the existing set on subsequent clicks
        int count = 2 + QRandomGenerator::global()->bounded(4);
        for (int i = 0; i < count; ++i) {
            ReviewPipelineEntry e;
            e.id = entries_.last().id + i + 1;
            e.paper = inputField_->text().isEmpty()
                ? QString("Paper-%1").arg(e.id)
                : QString("%1-%2").arg(inputField_->text()).arg(e.id);
            e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
            e.stage = stages[QRandomGenerator::global()->bounded(stages.size())];
            e.score = QRandomGenerator::global()->bounded(100);
            e.reviewers = QRandomGenerator::global()->bounded(5) + 1;
            e.accepted = (e.score >= 70);
            e.color = colors[categories.indexOf(e.category) >= 0 ? categories.indexOf(e.category) : 0];
            entries_.append(e);
        }
    } else {
        // Seed 8 entries on first click
        for (int i = 0; i < 8; ++i) {
            ReviewPipelineEntry e;
            e.id = i + 1;
            e.paper = QString("Paper-%1").arg(e.id);
            e.category = categories[i % categories.size()];
            e.stage = stages[QRandomGenerator::global()->bounded(stages.size())];
            e.score = 30.0 + QRandomGenerator::global()->bounded(70);
            e.reviewers = QRandomGenerator::global()->bounded(5) + 1;
            e.accepted = (e.score >= 70);
            e.color = colors[categories.indexOf(e.category)];
            entries_.append(e);
        }
    }

    saveSettings();
    updateInfo();
    if (!entries_.isEmpty())
        emit reviewUpdated(entries_.last().id, entries_.last().score);
    update();
    inputField_->clear();
}

void PaperReviewPipeline::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Track review pipeline stages");
    update();
}

void PaperReviewPipeline::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Track review pipeline stages");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Review Pipeline");

    int w = width(), h = height();
    drawPipelineView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReviewPipeline::drawPipelineView(QPainter& p, const QRect& rect) {
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

        QColor boxColor = e.accepted ? QColor(0x16a34a).lighter(140) : e.color.lighter(140);
        p.setPen(e.accepted ? QColor(0x16a34a) : e.color);
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
            QString("Score: %1").arg(static_cast<int>(e.score)));

        if (e.accepted) {
            p.setPen(QColor(0x16a34a));
            p.setFont(QFont("Arial", 6, QFont::Bold));
            p.drawText(x, centerY + 46, boxW, 12, Qt::AlignCenter, "ACCEPTED");
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

void PaperReviewPipeline::drawCategoryChart(QPainter& p, const QRect& rect) {
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

void PaperReviewPipeline::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries", QString::number(entries_.size()), QColor(0x3b82f6)},
        {"Accepted", QString::number(acceptedCount()), QColor(0x16a34a)},
        {"Avg Score", QString::number(avgScore(), 'f', 1), QColor(0xd97706)},
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

void PaperReviewPipeline::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Track review pipeline stages");
        return;
    }
    infoLabel_->setText(QString("Entries: %1 | Accepted: %2 | Avg Score: %3")
        .arg(entries_.size())
        .arg(acceptedCount())
        .arg(QString::number(avgScore(), 'f', 1)));
}

void PaperReviewPipeline::loadSettings() {
    settings_.beginGroup("ReviewPipeline");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        ReviewPipelineEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.paper = settings_.value(QString("paper_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.stage = settings_.value(QString("stage_%1").arg(i)).toString();
        e.score = settings_.value(QString("score_%1").arg(i)).toDouble();
        e.reviewers = settings_.value(QString("reviewers_%1").arg(i)).toInt();
        e.accepted = settings_.value(QString("accepted_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperReviewPipeline::saveSettings() {
    settings_.beginGroup("ReviewPipeline");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("paper_%1").arg(i), e.paper);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("stage_%1").arg(i), e.stage);
        settings_.setValue(QString("score_%1").arg(i), e.score);
        settings_.setValue(QString("reviewers_%1").arg(i), e.reviewers);
        settings_.setValue(QString("accepted_%1").arg(i), e.accepted);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
