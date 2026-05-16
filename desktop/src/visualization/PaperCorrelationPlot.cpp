#include "visualization/PaperCorrelationPlot.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperCorrelationPlot::PaperCorrelationPlot(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CorrelationPlot")
{
    setupUI();
    loadSettings();
}

void PaperCorrelationPlot::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperCorrelationPlot::onRender);
    toolbar->addWidget(renderBtn_);
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Linear", "Polynomial", "Spearman", "Kendall"});
    toolbar->addWidget(categoryCombo_, 1);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter variable prefix...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCorrelationPlot::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    infoLabel_ = new QLabel("Render correlation plot");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(600, 500);
}

void PaperCorrelationPlot::addEntry(const CorrEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit pairSelected(entry.id, entry.coefficient);
    update();
}

QList<CorrEntry> PaperCorrelationPlot::entries() const { return entries_; }

int PaperCorrelationPlot::significantCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.significant) c++;
    return c;
}

qreal PaperCorrelationPlot::maxCoefficient() const {
    qreal m = 0.0;
    for (const auto& e : entries_) m = qMax(m, qAbs(e.coefficient));
    return m;
}

QMap<QString, int> PaperCorrelationPlot::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperCorrelationPlot::onRender() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"Linear", "Polynomial", "Spearman", "Kendall"};
    int cIdx = categoryCombo_->currentIndex();
    entries_.clear();
    int count = 5 + QRandomGenerator::global()->bounded(6);
    for (int i = 0; i < count; ++i) {
        CorrEntry e;
        e.id = entries_.size() + 1;
        e.varX = text.left(4) + "X" + QString::number(i);
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.varY = text.left(4) + "Y" + QString::number(i);
        e.coefficient = (QRandomGenerator::global()->bounded(2000) - 1000) / 1000.0;
        e.samples = 20 + QRandomGenerator::global()->bounded(180);
        e.significant = qAbs(e.coefficient) > 0.5;
        QColor palette[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
        int ci = categories.indexOf(e.category);
        e.color = palette[ci >= 0 ? ci % 5 : 0];
        entries_.append(e);
    }
    saveSettings();
    updateInfo();
    emit pairSelected(entries_.last().id, entries_.last().coefficient);
    update();
    inputField_->clear();
}

void PaperCorrelationPlot::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Render correlation plot");
    update();
}

void PaperCorrelationPlot::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Render correlation plot");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Correlation Plot");
    int w = width(), h = height();
    drawCorrMatrix(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperCorrelationPlot::drawCorrMatrix(QPainter& p, const QRect& rect) {
    int margin = 10;
    int plotW = rect.width() - 2 * margin;
    int plotH = rect.height() - 2 * margin;
    p.setPen(QColor(203, 213, 225));
    p.drawLine(rect.x() + margin, rect.y() + margin, rect.x() + margin, rect.y() + margin + plotH);
    p.drawLine(rect.x() + margin, rect.y() + margin + plotH, rect.x() + margin + plotW, rect.y() + margin + plotH);
    qreal maxCoeff = maxCoefficient();
    if (maxCoeff == 0.0) maxCoeff = 1.0;
    int barMaxW = plotW / qMax(1, entries_.size()) - 4;
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        int cellW = plotW / qMax(1, entries_.size());
        int bx = rect.x() + margin + i * cellW + cellW / 2 - barMaxW / 2;
        int normH = static_cast<int>((qAbs(e.coefficient) / maxCoeff) * (plotH - 30));
        int by = rect.y() + margin + plotH - normH;
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(bx, by, barMaxW, normH, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(bx - 4, rect.y() + margin + plotH + 2, barMaxW + 8, 14, Qt::AlignCenter, e.varX);
    }
}

void PaperCorrelationPlot::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"Linear", "Polynomial", "Spearman", "Kendall"};
    QString labels[] = {"Linear", "Polynomial", "Spearman", "Kendall"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38)};
    int itemH = qMin(28, (rect.height() - 50) / 5);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (itemH + 4);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawEllipse(rect.x() + 5, y + 4, 14, 14);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18, Qt::AlignVCenter, labels[i]);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight, QString::number(count) + " pairs");
    }
    int y = rect.y() + 22 + 4 * (itemH + 4);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(124,58,237));
    p.drawEllipse(rect.x() + 5, y + 4, 14, 14);
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 9, QFont::Bold));
    p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18, Qt::AlignVCenter, "Significant");
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 8));
    p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
               Qt::AlignVCenter | Qt::AlignRight, QString::number(significantCount()));
}

void PaperCorrelationPlot::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries", QString::number(entries_.size()), QColor(59,130,246)},
        {"Significant", QString::number(significantCount()), QColor(124,58,237)},
        {"Max |r|", QString::number(maxCoefficient(), 'f', 3), QColor(22,163,74)},
        {"Categories", QString::number(categoryCounts().size()), QColor(217,119,6)}
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

void PaperCorrelationPlot::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Render correlation plot"); return; }
    infoLabel_->setText(QString("%1 pairs | %2 significant | max |r|=%3")
        .arg(entries_.size()).arg(significantCount()).arg(maxCoefficient(), 0, 'f', 3));
}

void PaperCorrelationPlot::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        CorrEntry e;
        e.id = settings_.value("id").toInt();
        e.varX = settings_.value("varX").toString();
        e.category = settings_.value("category").toString();
        e.varY = settings_.value("varY").toString();
        e.coefficient = settings_.value("coefficient").toDouble();
        e.samples = settings_.value("samples").toInt();
        e.significant = settings_.value("significant").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperCorrelationPlot::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("varX", entries_[i].varX);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("varY", entries_[i].varY);
        settings_.setValue("coefficient", entries_[i].coefficient);
        settings_.setValue("samples", entries_[i].samples);
        settings_.setValue("significant", entries_[i].significant);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
