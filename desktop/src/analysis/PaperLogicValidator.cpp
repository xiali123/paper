#include "analysis/PaperLogicValidator.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>
#include <QtMath>

PaperLogicValidator::PaperLogicValidator(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "LogicValidator")
{
    setupUI();
    loadSettings();
}

void PaperLogicValidator::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Deductive", "Inductive", "Abductive", "Analogical", "Causal"});
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Premise=>Conclusion");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 2);

    validateBtn_ = new QPushButton("Validate");
    validateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(validateBtn_, &QPushButton::clicked, this, &PaperLogicValidator::onValidate);
    toolbar->addWidget(validateBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperLogicValidator::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Validate logic arguments");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(620, 520);
}

void PaperLogicValidator::addEntry(const LogicValidatorEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit logicValidated(entry.id, entry.validity);
    update();
}

QList<LogicValidatorEntry> PaperLogicValidator::entries() const { return entries_; }

int PaperLogicValidator::soundCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.sound) ++c;
    return c;
}

qreal PaperLogicValidator::avgValidity() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.validity;
    return sum / entries_.size();
}

QMap<QString, int> PaperLogicValidator::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperLogicValidator::onValidate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    static const QStringList categories = {"Deductive", "Inductive", "Abductive", "Analogical", "Causal"};
    static const QMap<QString, QColor> categoryColor = {
        {"Deductive",  QColor(59, 130, 246)},   // #3b82f6
        {"Inductive",  QColor(22, 163, 74)},    // #16a34a
        {"Abductive",  QColor(217, 119, 6)},    // #d97706
        {"Analogical", QColor(220, 38, 38)},    // #dc2626
        {"Causal",     QColor(124, 58, 237)}    // #7c3aed
    };

    QString premise = text;
    QString conclusion = text;
    int arrowIdx = text.indexOf("=>");
    if (arrowIdx >= 0) {
        premise = text.left(arrowIdx).trimmed();
        conclusion = text.mid(arrowIdx + 2).trimmed();
    }

    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(5);

    for (int i = 0; i < count; ++i) {
        LogicValidatorEntry e;
        e.id = entries_.size() + 1;
        e.premise = premise.left(12) + " p" + QString::number(i);
        e.category = (cIdx == 0)
            ? categories[QRandomGenerator::global()->bounded(categories.size())]
            : categories[cIdx - 1];
        e.conclusion = conclusion.left(10) + " c" + QString::number(i);
        e.validity = 0.2 + QRandomGenerator::global()->bounded(80) / 100.0;
        e.steps = 2 + QRandomGenerator::global()->bounded(6);
        e.sound = (e.validity >= 0.6 && e.steps >= 3);
        e.color = categoryColor.value(e.category, QColor(100, 116, 139));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperLogicValidator::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Validate logic arguments");
    update();
}

void PaperLogicValidator::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Validate logic arguments");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Logic Validator");

    int w = width(), h = height();
    // top half: logic view
    drawLogicView(p, QRect(20, 50, w - 40, h / 2 - 50));
    // bottom-left: category pie chart
    drawCategoryChart(p, QRect(20, h / 2 + 10, w / 2 - 20, h / 2 - 40));
    // bottom-right: stats
    drawStats(p, QRect(w / 2 + 10, h / 2 + 10, w / 2 - 30, h / 2 - 40));
}

void PaperLogicValidator::drawLogicView(QPainter& p, const QRect& rect) {
    int show = qMin(8, entries_.size());
    int itemH = qMin(52, (rect.height() - 10) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 4);
        int x = rect.x();
        int w = rect.width();

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(190));
        QPainterPath card;
        card.addRoundedRect(x, y, w, itemH, 6, 6);
        p.drawPath(card);

        // Left color accent bar
        p.setBrush(e.color);
        QPainterPath accent;
        accent.addRoundedRect(x, y, 5, itemH, 2, 2);
        p.drawPath(accent);

        // Premise => Conclusion flow arrow
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        QString premiseText = e.premise.left(16);
        QString conclusionText = e.conclusion.left(14);
        int textX = x + 12;
        int textW = w / 2 - 20;
        p.drawText(textX, y + 4, textW, 18, Qt::AlignVCenter,
                   premiseText + " => " + conclusionText);

        // Category label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(textX, y + 22, textW, 14, Qt::AlignVCenter,
                   e.category);

        // Steps count
        p.drawText(textX, y + 36, textW, 14, Qt::AlignVCenter,
                   QString::number(e.steps) + " steps");

        // Validity gauge (right side)
        int gaugeX = x + w / 2 + 10;
        int gaugeW = w / 2 - 30;
        int gaugeY = y + 8;
        int gaugeH = 10;

        // gauge background track
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        QPainterPath gaugeBg;
        gaugeBg.addRoundedRect(gaugeX, gaugeY, gaugeW, gaugeH, 5, 5);
        p.drawPath(gaugeBg);

        // gauge filled portion
        int filledW = static_cast<int>(e.validity * gaugeW);
        QColor gaugeColor = e.validity >= 0.7 ? QColor(22, 163, 74) :
                            e.validity >= 0.4 ? QColor(245, 158, 11) :
                                                QColor(239, 68, 68);
        p.setBrush(gaugeColor);
        QPainterPath gaugeFill;
        gaugeFill.addRoundedRect(gaugeX, gaugeY, qMax(filledW, 4), gaugeH, 5, 5);
        p.drawPath(gaugeFill);

        // validity percentage
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(gaugeX, gaugeY + gaugeH + 2, gaugeW, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.validity * 100, 'f', 0) + "%");

        // Sound badge
        int badgeY = y + 30;
        int badgeW = 54;
        int badgeH = 16;
        p.setPen(Qt::NoPen);
        p.setBrush(e.sound ? QColor(22, 163, 74) : QColor(239, 68, 68));
        QPainterPath badge;
        badge.addRoundedRect(gaugeX, badgeY, badgeW, badgeH, 8, 8);
        p.drawPath(badge);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(gaugeX, badgeY, badgeW, badgeH,
                   Qt::AlignCenter, e.sound ? "SOUND" : "UNSOUND");
    }
}

void PaperLogicValidator::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.x(), rect.y() + 14, "Categories");

    auto counts = categoryCounts();
    static const QStringList categories = {"Deductive", "Inductive", "Abductive", "Analogical", "Causal"};
    static const QColor colors[] = {
        QColor(59, 130, 246),   // #3b82f6 Deductive
        QColor(22, 163, 74),    // #16a34a Inductive
        QColor(217, 119, 6),    // #d97706 Abductive
        QColor(220, 38, 38),    // #dc2626 Analogical
        QColor(124, 58, 237)    // #7c3aed Causal
    };

    int total = 0;
    for (const auto& cat : categories)
        total += counts.value(cat, 0);
    if (total == 0) total = 1;

    // Pie chart
    int pieSize = qMin(rect.width() - 20, rect.height() - 50);
    int pieX = rect.x() + (rect.width() - pieSize) / 2;
    int pieY = rect.y() + 26;
    qreal startAngle = 0.0;

    for (int i = 0; i < 5; ++i) {
        int count = counts.value(categories[i], 0);
        if (count == 0) continue;
        qreal span = (static_cast<qreal>(count) / total) * 360.0;

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawPie(pieX, pieY, pieSize, pieSize,
                  static_cast<int>(startAngle * 16),
                  static_cast<int>(span * 16));
        startAngle += span;
    }

    // White center to make it a donut
    int centerR = pieSize / 3;
    p.setBrush(Qt::white);
    p.drawEllipse(pieX + pieSize / 2 - centerR, pieY + pieSize / 2 - centerR,
                  centerR * 2, centerR * 2);

    // Legend below pie
    int legendY = pieY + pieSize + 6;
    int legendX = rect.x() + 10;
    int colW = rect.width() / 2;
    p.setFont(QFont("Arial", 7));
    for (int i = 0; i < 5; ++i) {
        int lx = legendX + (i % 2) * colW;
        int ly = legendY + (i / 2) * 16;
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(lx, ly + 2, 8, 8, 2, 2);
        p.setPen(QColor(15, 23, 42));
        p.drawText(lx + 12, ly + 10,
                   categories[i] + " (" + QString::number(counts.value(categories[i], 0)) + ")");
    }
}

void PaperLogicValidator::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Entries", QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Sound",         QString::number(soundCount()),   QColor(22, 163, 74)},
        {"Avg Validity",  QString::number(avgValidity() * 100, 'f', 0) + "%", QColor(124, 58, 237)},
        {"Categories",    QString::number(categoryCounts().size()), QColor(217, 119, 6)}
    };

    int boxH = qMin(48, (rect.height() - 10) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(192));
        QPainterPath box;
        box.addRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);
        p.drawPath(box);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 15, QFont::Bold));
        p.drawText(rect.x() + 12, y + 4, rect.width() - 24, 24,
                   Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 12, y + 28, rect.width() - 24, 16,
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperLogicValidator::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Validate logic arguments");
        return;
    }
    infoLabel_->setText(QString("%1 entries | %2 sound | %3% validity")
        .arg(entries_.size())
        .arg(soundCount())
        .arg(avgValidity() * 100, 0, 'f', 0));
}

void PaperLogicValidator::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        LogicValidatorEntry e;
        e.id         = settings_.value("id").toInt();
        e.premise    = settings_.value("premise").toString();
        e.category   = settings_.value("category").toString();
        e.conclusion = settings_.value("conclusion").toString();
        e.validity   = settings_.value("validity").toDouble();
        e.steps      = settings_.value("steps").toInt();
        e.sound      = settings_.value("sound").toBool();
        e.color      = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();

    // Seed 8 entries if empty
    if (entries_.isEmpty()) {
        static const QStringList categories = {"Deductive", "Inductive", "Abductive", "Analogical", "Causal"};
        static const QMap<QString, QColor> categoryColor = {
            {"Deductive",  QColor(59, 130, 246)},
            {"Inductive",  QColor(22, 163, 74)},
            {"Abductive",  QColor(217, 119, 6)},
            {"Analogical", QColor(220, 38, 38)},
            {"Causal",     QColor(124, 58, 237)}
        };
        static const QStringList premises = {
            "All humans are mortal", "The sun rose today", "The grass is wet",
            "DNA is a double helix", "Birds have feathers", "Water boils at 100C",
            "The sky is blue", "Mammals are warm-blooded"
        };
        static const QStringList conclusions = {
            "Socrates is mortal", "The sun will rise tomorrow", "It rained last night",
            "Proteins are folded", "Penguins have feathers", "Steam is water vapor",
            "Light scatters in atmosphere", "Whales are warm-blooded"
        };

        for (int i = 0; i < 8; ++i) {
            LogicValidatorEntry e;
            e.id = i + 1;
            e.premise = premises[i];
            e.category = categories[i % 5];
            e.conclusion = conclusions[i];
            e.validity = 0.45 + (i * 0.07);
            if (e.validity > 1.0) e.validity = 1.0;
            e.steps = 3 + (i % 4);
            e.sound = (e.validity >= 0.6 && e.steps >= 3);
            e.color = categoryColor.value(e.category, QColor(100, 116, 139));
            entries_.append(e);
        }
        saveSettings();
    }

    updateInfo();
}

void PaperLogicValidator::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",         entries_[i].id);
        settings_.setValue("premise",    entries_[i].premise);
        settings_.setValue("category",   entries_[i].category);
        settings_.setValue("conclusion", entries_[i].conclusion);
        settings_.setValue("validity",   entries_[i].validity);
        settings_.setValue("steps",      entries_[i].steps);
        settings_.setValue("sound",      entries_[i].sound);
        settings_.setValue("color",      entries_[i].color.name());
    }
    settings_.endArray();
}
