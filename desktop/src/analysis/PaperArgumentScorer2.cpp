#include "analysis/PaperArgumentScorer2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>

PaperArgumentScorer2::PaperArgumentScorer2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ArgumentScorer2")
{
    setupUI();
    loadSettings();
}

void PaperArgumentScorer2::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Logic", "Rhetoric", "Science", "Philosophy", "Law"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; "
        "min-width: 120px; }"
        "QComboBox::drop-down { border: none; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter argument...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    scoreBtn_ = new QPushButton("Score");
    scoreBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 14px; "
        "border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(scoreBtn_, &QPushButton::clicked, this, &PaperArgumentScorer2::onScore);
    toolbar->addWidget(scoreBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperArgumentScorer2::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Score argument validity and soundness");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(640, 560);
}

void PaperArgumentScorer2::addEntry(const ArgumentScorer2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit argumentScored(entry.id, entry.score);
    update();
}

QList<ArgumentScorer2Entry> PaperArgumentScorer2::entries() const {
    return entries_;
}

int PaperArgumentScorer2::validCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.valid) ++c;
    return c;
}

qreal PaperArgumentScorer2::avgScore() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.score;
    return sum / entries_.size();
}

QMap<QString, int> PaperArgumentScorer2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperArgumentScorer2::onScore() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    static const QStringList types = {"Deductive", "Inductive", "Abductive", "Analogical"};
    static const QStringList categories = {"Logic", "Rhetoric", "Science", "Philosophy", "Law"};
    static const QMap<QString, QColor> typeColors = {
        {"Deductive",  QColor(59, 130, 246)},
        {"Inductive",  QColor(22, 163, 74)},
        {"Abductive",  QColor(217, 119, 6)},
        {"Analogical", QColor(124, 58, 237)}
    };

    int catIdx = categoryCombo_->currentIndex();
    int count = 2 + QRandomGenerator::global()->bounded(4);

    for (int i = 0; i < count; ++i) {
        ArgumentScorer2Entry e;
        e.id = entries_.size() + 1;
        e.argument = text.left(16) + " arg" + QString::number(i);
        e.type = types[QRandomGenerator::global()->bounded(types.size())];
        e.category = catIdx == 0
            ? categories[QRandomGenerator::global()->bounded(categories.size())]
            : categories[catIdx - 1];
        e.score = 0.15 + QRandomGenerator::global()->bounded(85) / 100.0;
        e.premises = 1 + QRandomGenerator::global()->bounded(9);
        e.valid = e.score >= 0.55 && e.premises >= 2;
        e.color = typeColors.value(e.type, QColor(59, 130, 246));
        addEntry(e);
    }

    inputField_->clear();
}

void PaperArgumentScorer2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Score argument validity and soundness");
    update();
}

void PaperArgumentScorer2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Score argument validity and soundness");
        return;
    }
    infoLabel_->setText(
        QString("%1 scored | %2 valid | %3 avg score")
            .arg(entries_.size())
            .arg(validCount())
            .arg(avgScore() * 100, 0, 'f', 0) + "%");
}

void PaperArgumentScorer2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Score argument validity and soundness");
        return;
    }

    int w = width();
    int h = height();

    drawScorerView(p, QRect(10, 10, w - 20, h / 2 - 10));
    drawCategoryChart(p, QRect(10, h / 2 + 10, w / 2 - 20, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 10, w / 2 - 20, h / 2 - 30));
}

void PaperArgumentScorer2::drawScorerView(QPainter& p, const QRect& rect) {
    int show = qMin(8, entries_.size());
    int itemH = qMin(48, (rect.height() - 30) / qMax(show, 1));

    // Section title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 12, QFont::Bold));
    p.drawText(rect.x() + 6, rect.y() + 18, "Argument Cards");

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + 28 + i * (itemH + 3);

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(248, 250, 252));
        QPainterPath cardBg;
        cardBg.addRoundedRect(rect.x(), y, rect.width(), itemH, 6, 6);
        p.drawPath(cardBg);

        // Left color accent strip
        p.setBrush(e.color);
        QPainterPath strip;
        strip.addRoundedRect(rect.x(), y, 5, itemH, 2, 2);
        p.drawPath(strip);

        // Type badge
        static const QMap<QString, QColor> badgeColors = {
            {"Deductive",  QColor(59, 130, 246)},
            {"Inductive",  QColor(22, 163, 74)},
            {"Abductive",  QColor(217, 119, 6)},
            {"Analogical", QColor(124, 58, 237)}
        };
        QColor badgeColor = badgeColors.value(e.type, QColor(59, 130, 246));
        int badgeX = rect.x() + 12;
        int badgeY = y + 4;
        int badgeW = 68;
        int badgeH = 16;
        p.setBrush(badgeColor.lighter(160));
        QPainterPath badgePath;
        badgePath.addRoundedRect(badgeX, badgeY, badgeW, badgeH, 8, 8);
        p.drawPath(badgePath);
        p.setPen(badgeColor);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(badgeX, badgeY, badgeW, badgeH, Qt::AlignCenter, e.type);

        // Argument text
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 86, y + 4, rect.width() * 0.35, 16, Qt::AlignVCenter,
                   e.argument.left(24));

        // Premise count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 12, y + 22, rect.width() * 0.4, 14, Qt::AlignVCenter,
                   e.category + " | premises: " + QString::number(e.premises));

        // Score gauge arc (right side)
        int gaugeCX = rect.x() + rect.width() - 68;
        int gaugeCY = y + itemH / 2;
        int gaugeR = 16;

        // Background arc
        p.setPen(QPen(QColor(226, 232, 240), 3));
        p.setBrush(Qt::NoBrush);
        p.drawArc(gaugeCX - gaugeR, gaugeCY - gaugeR, gaugeR * 2, gaugeR * 2,
                  30 * 16, 120 * 16);

        // Score arc
        int sweepAngle = static_cast<int>(e.score * 120) * 16;
        QColor gaugeColor = e.score >= 0.7 ? QColor(22, 163, 74)
                          : e.score >= 0.4 ? QColor(217, 119, 6)
                          : QColor(220, 38, 38);
        p.setPen(QPen(gaugeColor, 3));
        p.drawArc(gaugeCX - gaugeR, gaugeCY - gaugeR, gaugeR * 2, gaugeR * 2,
                  30 * 16, -sweepAngle);

        // Score percentage inside gauge
        p.setPen(gaugeColor);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(gaugeCX - gaugeR, gaugeCY - gaugeR, gaugeR * 2, gaugeR * 2,
                   Qt::AlignCenter, QString::number(e.score * 100, 'f', 0) + "%");

        // Valid / Invalid indicator
        QString indicator = e.valid ? "Valid" : "Invalid";
        QColor indicatorColor = e.valid ? QColor(22, 163, 74) : QColor(220, 38, 38);
        int indX = rect.x() + rect.width() - 36;
        int indW = 30;
        int indH = 16;
        int indY = y + (itemH - indH) / 2;
        p.setPen(Qt::NoPen);
        p.setBrush(indicatorColor.lighter(165));
        QPainterPath indPath;
        indPath.addRoundedRect(indX, indY, indW, indH, 8, 8);
        p.drawPath(indPath);
        p.setPen(indicatorColor);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(indX, indY, indW, indH, Qt::AlignCenter, indicator);
    }
}

void PaperArgumentScorer2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(rect.x() + 4, rect.y() + 16, "Category Distribution");

    auto counts = categoryCounts();
    static const QStringList categories = {"Logic", "Rhetoric", "Science", "Philosophy", "Law"};
    static const QColor colors[] = {
        QColor(59, 130, 246), QColor(22, 163, 74),
        QColor(217, 119, 6), QColor(220, 38, 38), QColor(124, 58, 237)
    };

    int maxVal = 1;
    for (int i = 0; i < 5; ++i) {
        int c = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        maxVal = qMax(maxVal, c);
    }

    int barH = qMin(22, (rect.height() - 35) / 5);
    int chartLeft = rect.x() + 80;
    int chartW = rect.width() - 100;

    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 28 + i * (barH + 6);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * chartW);

        // Category label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y, 74, barH, Qt::AlignRight | Qt::AlignVCenter, categories[i]);

        // Horizontal bar
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        QPainterPath bar;
        bar.addRoundedRect(chartLeft, y, qMax(barW, 2), barH - 2, 3, 3);
        p.drawPath(bar);

        // Count label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(chartLeft + barW + 6, y, 40, barH, Qt::AlignVCenter | Qt::AlignLeft,
                   QString::number(count));
    }
}

void PaperArgumentScorer2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat {
        QString label;
        QString value;
        QColor color;
    };

    QList<Stat> stats = {
        {"Total Scored",  QString::number(entries_.size()),          QColor(59, 130, 246)},
        {"Valid",         QString::number(validCount()),             QColor(22, 163, 74)},
        {"Avg Score",     QString::number(avgScore() * 100, 'f', 0) + "%", QColor(217, 119, 6)},
        {"Categories",    QString::number(categoryCounts().size()),  QColor(124, 58, 237)}
    };

    int boxH = qMin(44, (rect.height() - 20) / 4);

    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + 5 + i * (boxH + 6);

        // Box background
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        QPainterPath box;
        box.addRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);
        p.drawPath(box);

        // Color accent on left
        p.setBrush(stats[i].color);
        QPainterPath accent;
        accent.addRoundedRect(rect.x(), y, 4, boxH, 2, 2);
        p.drawPath(accent);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 13, QFont::Bold));
        p.drawText(rect.x() + 12, y + 4, rect.width() - 24, 20,
                   Qt::AlignVCenter, stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 12, y + 24, rect.width() - 24, 14,
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperArgumentScorer2::loadSettings() {
    settings_.beginGroup("ArgumentScorer2");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ArgumentScorer2Entry e;
        e.id       = settings_.value("id").toInt();
        e.argument = settings_.value("argument").toString();
        e.category = settings_.value("category").toString();
        e.type     = settings_.value("type").toString();
        e.score    = settings_.value("score").toDouble();
        e.premises = settings_.value("premises").toInt();
        e.valid    = settings_.value("valid").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperArgumentScorer2::saveSettings() {
    settings_.beginGroup("ArgumentScorer2");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",       entries_[i].id);
        settings_.setValue("argument", entries_[i].argument);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("type",     entries_[i].type);
        settings_.setValue("score",    entries_[i].score);
        settings_.setValue("premises", entries_[i].premises);
        settings_.setValue("valid",    entries_[i].valid);
        settings_.setValue("color",    entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
