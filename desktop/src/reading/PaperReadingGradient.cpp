#include "reading/PaperReadingGradient.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperReadingGradient::PaperReadingGradient(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingGradient")
{
    setupUI();
    loadSettings();
}

void PaperReadingGradient::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    measureBtn_ = new QPushButton("Measure");
    measureBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(measureBtn_, &QPushButton::clicked, this, &PaperReadingGradient::onMeasure);
    toolbar->addWidget(measureBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Beginner", "Intermediate", "Advanced", "Expert", "Master"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingGradient::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter paper title...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Measure reading progress gradients");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperReadingGradient::addEntry(const GradientEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit gradientMeasured(entry.id, entry.delta);
    update();
}

QList<GradientEntry> PaperReadingGradient::entries() const { return entries_; }

int PaperReadingGradient::advancedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.delta > 0.3) c++;
    return c;
}

qreal PaperReadingGradient::avgDelta() const {
    if (entries_.isEmpty()) return 0.0;
    qreal total = 0;
    for (const auto& e : entries_) total += e.delta;
    return total / entries_.size();
}

QMap<QString, int> PaperReadingGradient::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperReadingGradient::onMeasure() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"Beginner", "Intermediate", "Advanced", "Expert", "Master"};
    int cIdx = categoryCombo_->currentIndex();
    entries_.clear();

    int count = 10 + QRandomGenerator::global()->bounded(20);
    for (int i = 0; i < count; ++i) {
        GradientEntry e;
        e.id = entries_.size() + 1;
        e.paper = text.left(20) + " p" + QString::number(i);
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.level = categories.indexOf(e.category);
        e.startProgress = QRandomGenerator::global()->bounded(500) / 10.0;
        e.endProgress = e.startProgress + QRandomGenerator::global()->bounded(-200, 800) / 10.0;
        e.endProgress = qBound(0.0, e.endProgress, 100.0);
        e.delta = e.endProgress - e.startProgress;
        e.advanced = (e.delta > 0.3);
        QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
        e.color = colors[e.level % colors.size()];
        entries_.append(e);
    }

    saveSettings();
    updateInfo();
    emit gradientMeasured(entries_.size(), entries_.last().delta);
    update();
    inputField_->clear();
}

void PaperReadingGradient::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Measure reading progress gradients");
    update();
}

void PaperReadingGradient::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Measure reading progress gradients");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Gradient");

    int w = width(), h = height();
    drawGradientView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReadingGradient::drawGradientView(QPainter& p, const QRect& rect) {
    int margin = 10;
    int barH = qMin(16, (rect.height() - 30) / qMax(entries_.size(), 1));
    int visibleCount = qMin(entries_.size(), (rect.height() - 30) / qMax(barH + 4, 1));
    int plotW = rect.width() - 2 * margin;

    // Draw header
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 8));
    p.drawText(rect.x() + margin, rect.y() + 10, "0%");
    p.drawText(rect.x() + margin + plotW - 20, rect.y() + 10, "100%");

    // Draw background grid line at 50%
    p.setPen(QColor(226, 232, 240));
    int midX = rect.x() + margin + plotW / 2;
    p.drawLine(midX, rect.y() + 18, midX, rect.y() + 18 + visibleCount * (barH + 4));

    int y = rect.y() + 22;
    for (int i = 0; i < visibleCount; ++i) {
        const auto& e = entries_[i];

        // Draw start progress marker
        int startX = rect.x() + margin + static_cast<int>((e.startProgress / 100.0) * plotW);
        int endX = rect.x() + margin + static_cast<int>((e.endProgress / 100.0) * plotW);

        // Draw gradient bar from start to end
        QColor barColor = e.color;
        if (e.delta > 0) {
            QLinearGradient gradient(startX, y, endX, y);
            gradient.setColorAt(0, barColor.lighter(160));
            gradient.setColorAt(1, barColor);
            p.setPen(Qt::NoPen);
            p.setBrush(gradient);
            p.drawRoundedRect(qMin(startX, endX), y, qAbs(endX - startX) + 2, barH, 3, 3);
        } else {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(0xdc2626).lighter(150));
            p.drawRoundedRect(qMin(startX, endX), y, qAbs(endX - startX) + 2, barH, 3, 3);
        }

        // Draw start marker dot
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(15, 23, 42));
        p.drawEllipse(startX - 2, y + barH / 2 - 2, 4, 4);

        // Draw label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        QString label = QString("%1 (%2)").arg(e.paper.left(12)).arg(QString::number(e.delta, 'f', 1));
        p.drawText(rect.x() + margin, y + barH - 1, label);

        y += barH + 4;
    }
}

void PaperReadingGradient::drawCategoryChart(QPainter& p, const QRect& rect) {
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

void PaperReadingGradient::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries", QString::number(entries_.size()), QColor(0x3b82f6)},
        {"Advanced", QString::number(advancedCount()), QColor(0x16a34a)},
        {"Avg Delta", QString::number(avgDelta(), 'f', 2), QColor(0xd97706)},
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

void PaperReadingGradient::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Measure reading progress gradients");
        return;
    }
    infoLabel_->setText(QString("Entries: %1 | Advanced: %2 | Avg Delta: %3")
        .arg(entries_.size())
        .arg(advancedCount())
        .arg(QString::number(avgDelta(), 'f', 2)));
}

void PaperReadingGradient::loadSettings() {
    settings_.beginGroup("ReadingGradient");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        GradientEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.paper = settings_.value(QString("paper_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.level = settings_.value(QString("level_%1").arg(i)).toInt();
        e.startProgress = settings_.value(QString("startProgress_%1").arg(i)).toDouble();
        e.endProgress = settings_.value(QString("endProgress_%1").arg(i)).toDouble();
        e.delta = settings_.value(QString("delta_%1").arg(i)).toDouble();
        e.advanced = settings_.value(QString("advanced_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperReadingGradient::saveSettings() {
    settings_.beginGroup("ReadingGradient");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("paper_%1").arg(i), e.paper);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("level_%1").arg(i), e.level);
        settings_.setValue(QString("startProgress_%1").arg(i), e.startProgress);
        settings_.setValue(QString("endProgress_%1").arg(i), e.endProgress);
        settings_.setValue(QString("delta_%1").arg(i), e.delta);
        settings_.setValue(QString("advanced_%1").arg(i), e.advanced);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
