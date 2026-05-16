#include "analysis/PaperArgumentScorer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>

PaperArgumentScorer::PaperArgumentScorer(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ArgumentScorer")
{
    setupUI();
    loadSettings();
}

void PaperArgumentScorer::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Deductive", "Inductive", "Abductive", "Analogical"});
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
    connect(scoreBtn_, &QPushButton::clicked, this, &PaperArgumentScorer::onScore);
    toolbar->addWidget(scoreBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperArgumentScorer::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Score argument strength and soundness");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(620, 520);
}

void PaperArgumentScorer::addEntry(const ArgumentScoreEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit argumentScored(entry.id, entry.strength);
    update();
}

QList<ArgumentScoreEntry> PaperArgumentScorer::entries() const {
    return entries_;
}

int PaperArgumentScorer::soundCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.sound) c++;
    return c;
}

qreal PaperArgumentScorer::avgStrength() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.strength;
    return sum / entries_.size();
}

QMap<QString, int> PaperArgumentScorer::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperArgumentScorer::onScore() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    static const QStringList categories = {"Deductive", "Inductive", "Abductive", "Analogical"};
    static const QStringList stances = {"Support", "Neutral", "Oppose"};
    static const QMap<QString, QColor> catColors = {
        {"Deductive",  QColor(59, 130, 246)},
        {"Inductive",  QColor(22, 163, 74)},
        {"Abductive",  QColor(124, 58, 237)},
        {"Analogical", QColor(217, 119, 6)}
    };

    int catIdx = categoryCombo_->currentIndex();
    int count = 2 + QRandomGenerator::global()->bounded(4);

    for (int i = 0; i < count; ++i) {
        ArgumentScoreEntry e;
        e.id = entries_.size() + 1;
        e.argument = text.left(16) + " arg" + QString::number(i);
        e.category = catIdx == 0
            ? categories[QRandomGenerator::global()->bounded(categories.size())]
            : categories[catIdx - 1];
        e.stance = stances[QRandomGenerator::global()->bounded(stances.size())];
        e.strength = 0.25 + QRandomGenerator::global()->bounded(75) / 100.0;
        e.evidence = QRandomGenerator::global()->bounded(15);
        e.sound = e.strength >= 0.6 && e.evidence >= 3;
        e.color = catColors.value(e.category, QColor(59, 130, 246));
        addEntry(e);
    }

    inputField_->clear();
}

void PaperArgumentScorer::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Score argument strength and soundness");
    update();
}

void PaperArgumentScorer::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Score argument strength and soundness");
        return;
    }
    infoLabel_->setText(
        QString("%1 scored | %2 sound | %3 avg strength")
            .arg(entries_.size())
            .arg(soundCount())
            .arg(avgStrength() * 100, 0, 'f', 0) + "%");
}

void PaperArgumentScorer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Score argument strength and soundness");
        return;
    }

    int w = width();
    int h = height();

    drawScoreView(p, QRect(10, 10, w - 20, h / 2 - 10));
    drawCategoryChart(p, QRect(10, h / 2 + 10, w / 2 - 20, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 10, w / 2 - 20, h / 2 - 30));
}

void PaperArgumentScorer::drawScoreView(QPainter& painter, const QRect& rect) {
    int show = qMin(8, entries_.size());
    int itemH = qMin(42, (rect.height() - 30) / qMax(show, 1));

    painter.setPen(QColor(15, 23, 42));
    painter.setFont(QFont("Arial", 12, QFont::Bold));
    painter.drawText(rect.x() + 6, rect.y() + 18, "Scored Arguments");

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + 28 + i * (itemH + 3);

        // Background bar (lighter shade)
        painter.setPen(Qt::NoPen);
        painter.setBrush(e.color.lighter(185));
        QPainterPath bg;
        bg.addRoundedRect(rect.x(), y, rect.width(), itemH, 6, 6);
        painter.drawPath(bg);

        // Left color accent strip
        painter.setBrush(e.color);
        QPainterPath strip;
        strip.addRoundedRect(rect.x(), y, 5, itemH, 2, 2);
        painter.drawPath(strip);

        // Strength gauge fill (proportional width)
        int gaugeW = static_cast<int>(e.strength * (rect.width() - 16));
        painter.setBrush(e.color);
        painter.setOpacity(0.35);
        QPainterPath gauge;
        gauge.addRoundedRect(rect.x() + 8, y + 3, gaugeW, itemH - 6, 4, 4);
        painter.drawPath(gauge);
        painter.setOpacity(1.0);

        // Argument text
        painter.setPen(QColor(15, 23, 42));
        painter.setFont(QFont("Arial", 9, QFont::Bold));
        painter.drawText(rect.x() + 14, y + 3, rect.width() * 0.5, 18, Qt::AlignVCenter,
                         e.argument.left(28));

        // Evidence count
        painter.setPen(QColor(100, 116, 139));
        painter.setFont(QFont("Arial", 8));
        painter.drawText(rect.x() + 14, y + 20, rect.width() * 0.35, 16, Qt::AlignVCenter,
                         e.category + " | " + e.stance + " | evid:" + QString::number(e.evidence));

        // Strength percentage on right
        painter.setPen(QColor(15, 23, 42));
        painter.setFont(QFont("Arial", 10, QFont::Bold));
        painter.drawText(rect.x() + rect.width() * 0.6, y + 3, rect.width() * 0.22, 18,
                         Qt::AlignVCenter | Qt::AlignRight,
                         QString::number(e.strength * 100, 'f', 0) + "%");

        // Sound badge
        QString badge = e.sound ? "Sound" : "Weak";
        QColor badgeColor = e.sound ? QColor(22, 163, 74) : QColor(239, 68, 68);
        int badgeX = rect.x() + rect.width() * 0.62;
        int badgeW = 50;
        int badgeH = 18;
        int badgeY = y + 21;
        painter.setPen(Qt::NoPen);
        painter.setBrush(badgeColor.lighter(160));
        QPainterPath badgePath;
        badgePath.addRoundedRect(badgeX, badgeY, badgeW, badgeH, 9, 9);
        painter.drawPath(badgePath);
        painter.setPen(badgeColor);
        painter.setFont(QFont("Arial", 8, QFont::Bold));
        painter.drawText(badgeX, badgeY, badgeW, badgeH, Qt::AlignCenter, badge);
    }
}

void PaperArgumentScorer::drawCategoryChart(QPainter& painter, const QRect& rect) {
    painter.setPen(QColor(15, 23, 42));
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    painter.drawText(rect.x() + 4, rect.y() + 16, "Category Distribution");

    auto counts = categoryCounts();
    static const QStringList categories = {"Deductive", "Inductive", "Abductive", "Analogical"};
    static const QColor colors[] = {
        QColor(59, 130, 246), QColor(22, 163, 74),
        QColor(124, 58, 237), QColor(217, 119, 6)
    };
    static const QString labels[] = {"Deductive", "Inductive", "Abductive", "Analogical"};

    int maxVal = 1;
    for (int i = 0; i < 4; ++i) {
        int c = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        maxVal = qMax(maxVal, c);
    }

    int barH = qMin(22, (rect.height() - 35) / 4);
    int chartLeft = rect.x() + 80;
    int chartW = rect.width() - 100;

    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 28 + i * (barH + 6);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * chartW);

        painter.setPen(QColor(100, 116, 139));
        painter.setFont(QFont("Arial", 8));
        painter.drawText(rect.x(), y, 74, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        painter.setPen(Qt::NoPen);
        painter.setBrush(colors[i]);
        QPainterPath bar;
        bar.addRoundedRect(chartLeft, y, qMax(barW, 2), barH - 2, 3, 3);
        painter.drawPath(bar);

        painter.setPen(QColor(100, 116, 139));
        painter.setFont(QFont("Arial", 8));
        painter.drawText(chartLeft + barW + 6, y, 40, barH, Qt::AlignVCenter | Qt::AlignLeft,
                         QString::number(count));
    }
}

void PaperArgumentScorer::drawStats(QPainter& painter, const QRect& rect) {
    struct Stat {
        QString label;
        QString value;
        QColor color;
    };

    QList<Stat> stats = {
        {"Total Scored", QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Sound",        QString::number(soundCount()),    QColor(22, 163, 74)},
        {"Avg Strength", QString::number(avgStrength() * 100, 'f', 0) + "%", QColor(124, 58, 237)},
        {"Categories",   QString::number(categoryCounts().size()), QColor(217, 119, 6)}
    };

    int boxH = qMin(40, (rect.height() - 20) / 4);

    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + 5 + i * (boxH + 6);

        painter.setPen(Qt::NoPen);
        painter.setBrush(stats[i].color.lighter(190));
        QPainterPath box;
        box.addRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);
        painter.drawPath(box);

        // Color accent on left
        painter.setBrush(stats[i].color);
        QPainterPath accent;
        accent.addRoundedRect(rect.x(), y, 4, boxH, 2, 2);
        painter.drawPath(accent);

        painter.setPen(stats[i].color);
        painter.setFont(QFont("Arial", 13, QFont::Bold));
        painter.drawText(rect.x() + 12, y + 4, rect.width() - 24, 20,
                         Qt::AlignVCenter, stats[i].value);

        painter.setPen(QColor(100, 116, 139));
        painter.setFont(QFont("Arial", 9));
        painter.drawText(rect.x() + 12, y + 24, rect.width() - 24, 14,
                         Qt::AlignVCenter, stats[i].label);
    }
}

void PaperArgumentScorer::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ArgumentScoreEntry e;
        e.id       = settings_.value("id").toInt();
        e.argument = settings_.value("argument").toString();
        e.category = settings_.value("category").toString();
        e.stance   = settings_.value("stance").toString();
        e.strength = settings_.value("strength").toDouble();
        e.evidence = settings_.value("evidence").toInt();
        e.sound    = settings_.value("sound").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperArgumentScorer::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",       entries_[i].id);
        settings_.setValue("argument", entries_[i].argument);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("stance",   entries_[i].stance);
        settings_.setValue("strength", entries_[i].strength);
        settings_.setValue("evidence", entries_[i].evidence);
        settings_.setValue("sound",    entries_[i].sound);
        settings_.setValue("color",    entries_[i].color.name());
    }
    settings_.endArray();
}
