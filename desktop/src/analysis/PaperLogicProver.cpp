#include "analysis/PaperLogicProver.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>
#include <QtMath>
#include <numeric>

PaperLogicProver::PaperLogicProver(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "LogicProver")
{
    setupUI();
    loadSettings();

    if (entries_.isEmpty()) {
        static const QStringList axioms = {
            "Modus Ponens", "Contradiction", "Universal Instantiation",
            "Existential Generalization", "Necessitation Rule", "Temporal Induction",
            "Disjunction Elimination", "Intuitionistic Implication"
        };
        static const QStringList categories = {
            "Propositional", "Predicate", "Modal", "Temporal", "Intuitionistic"
        };
        static const QMap<QString, QColor> categoryColor = {
            {"Propositional",    QColor(59, 130, 246)},   // #3b82f6
            {"Predicate",        QColor(22, 163, 74)},    // #16a34a
            {"Modal",            QColor(124, 58, 237)},   // #7c3aed
            {"Temporal",         QColor(217, 119, 6)},    // #d97706
            {"Intuitionistic",   QColor(220, 38, 38)}     // #dc2626
        };
        static const QStringList proofs = {
            "Direct derivation from premises",
            "Proof by refutation: assume negation leads to contradiction",
            "Substitution of universal quantifier with ground term",
            "Existential witness introduction from instance",
            "Derived via necessitation from theorem in Kripke frame",
            "Inductive step verified across temporal transitions",
            "Case analysis on each disjunct yields shared conclusion",
            "Constructive proof via lambda term witness"
        };

        for (int i = 0; i < 8; ++i) {
            LogicProverEntry e;
            e.id = i + 1;
            e.axiom = axioms[i];
            e.category = categories[i % categories.size()];
            e.proof = proofs[i];
            e.confidence = 0.45 + QRandomGenerator::global()->bounded(55) / 100.0;
            e.steps = 2 + QRandomGenerator::global()->bounded(7);
            e.valid = e.confidence >= 0.6 && e.steps >= 3;
            e.color = categoryColor.value(e.category, QColor(100, 116, 139));
            entries_.append(e);
        }
        saveSettings();
        updateInfo();
        update();
    }
}

void PaperLogicProver::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Propositional", "Predicate", "Modal", "Temporal", "Intuitionistic"});
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Search axioms...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 2);

    proveBtn_ = new QPushButton("Prove");
    proveBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(proveBtn_, &QPushButton::clicked, this, &PaperLogicProver::onProve);
    toolbar->addWidget(proveBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperLogicProver::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Logic Prover ready");
    infoLabel_->setAlignment(Qt::AlignRight);
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(640, 540);
}

void PaperLogicProver::addEntry(const LogicProverEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit proofCompleted(entry.id, entry.confidence);
    update();
}

QList<LogicProverEntry> PaperLogicProver::entries() const { return entries_; }

int PaperLogicProver::validCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.valid) ++c;
    return c;
}

qreal PaperLogicProver::avgConfidence() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.confidence;
    return sum / entries_.size();
}

QMap<QString, int> PaperLogicProver::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperLogicProver::onProve() {
    static const QStringList axioms = {
        "Modus Ponens", "Contradiction", "Universal Instantiation",
        "Existential Generalization", "Necessitation Rule", "Temporal Induction",
        "Disjunction Elimination", "Intuitionistic Implication",
        "Modus Tollens", "Hypothetical Syllogism", "Constructive Dilemma",
        "Absorption", "Distribution", "Box-Distribution", "Future Necessity",
        "Brouwer's Fixed Point"
    };
    static const QStringList categories = {
        "Propositional", "Predicate", "Modal", "Temporal", "Intuitionistic"
    };
    static const QMap<QString, QColor> categoryColor = {
        {"Propositional",    QColor(59, 130, 246)},
        {"Predicate",        QColor(22, 163, 74)},
        {"Modal",            QColor(124, 58, 237)},
        {"Temporal",         QColor(217, 119, 6)},
        {"Intuitionistic",   QColor(220, 38, 38)}
    };

    int cIdx = categoryCombo_->currentIndex();
    QString category = (cIdx == 0)
        ? categories[QRandomGenerator::global()->bounded(categories.size())]
        : categories[cIdx - 1];

    LogicProverEntry e;
    e.id = entries_.size() + 1;
    e.axiom = axioms[QRandomGenerator::global()->bounded(axioms.size())];
    e.category = category;
    e.proof = "Automated proof derivation step " + QString::number(e.id);
    e.confidence = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
    e.steps = 2 + QRandomGenerator::global()->bounded(8);
    e.valid = e.confidence >= 0.6 && e.steps >= 3;
    e.color = categoryColor.value(e.category, QColor(100, 116, 139));
    addEntry(e);
    inputField_->clear();
}

void PaperLogicProver::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Logic Prover ready");
    update();
}

void PaperLogicProver::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No proofs yet. Click Prove to begin.");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Logic Prover");

    int w = width(), h = height();
    int topH = static_cast<int>(h * 0.60);
    int bottomH = static_cast<int>(h * 0.25);
    int midY = 50;

    // Left 60% height: prover view
    drawProverView(p, QRect(20, midY, w - 40, topH - midY));
    // Right 40% area: category chart (top-right beside prover view)
    drawCategoryChart(p, QRect(w / 2 + 10, midY, w / 2 - 30, topH - midY));
    // Bottom 25%: stats
    drawStats(p, QRect(20, topH + 10, w - 40, bottomH));
}

void PaperLogicProver::drawProverView(QPainter& p, const QRect& rect) {
    // Filter by category combo
    QString filter = categoryCombo_->currentText();
    QList<const LogicProverEntry*> visible;
    for (const auto& e : entries_) {
        if (filter == "All" || e.category == filter)
            visible.append(&e);
    }

    // Also filter by search text
    QString search = inputField_->text().trimmed().toLower();
    if (!search.isEmpty()) {
        QList<const LogicProverEntry*> filtered;
        for (auto* e : visible) {
            if (e->axiom.toLower().contains(search) ||
                e->category.toLower().contains(search))
                filtered.append(e);
        }
        visible = filtered;
    }

    int show = qMin(6, visible.size());
    int itemH = qMin(56, (rect.height() - 10) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& e = *visible[i];
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

        // Axiom name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        int textX = x + 12;
        int textW = w / 2 - 20;
        p.drawText(textX, y + 2, textW, 18, Qt::AlignVCenter, e.axiom);

        // Category label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(textX, y + 20, textW, 14, Qt::AlignVCenter, e.category);

        // Proof preview
        p.drawText(textX, y + 34, textW, 14, Qt::AlignVCenter,
                   e.proof.left(30) + (e.proof.length() > 30 ? "..." : ""));

        // Confidence gauge (right side)
        int gaugeX = x + w / 2 + 10;
        int gaugeW = w / 3;
        int gaugeY = y + 8;
        int gaugeH = 10;

        // Gauge background track
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        QPainterPath gaugeBg;
        gaugeBg.addRoundedRect(gaugeX, gaugeY, gaugeW, gaugeH, 5, 5);
        p.drawPath(gaugeBg);

        // Gauge filled portion - green >0.8, amber >0.5, red <=0.5
        int filledW = static_cast<int>(e.confidence * gaugeW);
        QColor gaugeColor = e.confidence > 0.8 ? QColor(22, 163, 74) :
                            e.confidence > 0.5 ? QColor(245, 158, 11) :
                                                 QColor(239, 68, 68);
        p.setBrush(gaugeColor);
        QPainterPath gaugeFill;
        gaugeFill.addRoundedRect(gaugeX, gaugeY, qMax(filledW, 4), gaugeH, 5, 5);
        p.drawPath(gaugeFill);

        // Confidence percentage
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(gaugeX, gaugeY + gaugeH + 2, gaugeW, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.confidence * 100, 'f', 0) + "%");

        // Steps count badge
        int badgeX = gaugeX;
        int badgeY = y + 30;
        int badgeW = 48;
        int badgeH = 16;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(59, 130, 246));
        QPainterPath stepsBadge;
        stepsBadge.addRoundedRect(badgeX, badgeY, badgeW, badgeH, 8, 8);
        p.drawPath(stepsBadge);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(badgeX, badgeY, badgeW, badgeH,
                   Qt::AlignCenter, QString::number(e.steps) + " steps");

        // Valid/Invalid indicator
        int validX = badgeX + badgeW + 8;
        int validW = 52;
        p.setPen(Qt::NoPen);
        p.setBrush(e.valid ? QColor(22, 163, 74) : QColor(239, 68, 68));
        QPainterPath validBadge;
        validBadge.addRoundedRect(validX, badgeY, validW, badgeH, 8, 8);
        p.drawPath(validBadge);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(validX, badgeY, validW, badgeH,
                   Qt::AlignCenter, e.valid ? "VALID" : "INVALID");
    }
}

void PaperLogicProver::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.x(), rect.y() + 14, "Category Distribution");

    auto counts = categoryCounts();
    static const QStringList categories = {
        "Propositional", "Predicate", "Modal", "Temporal", "Intuitionistic"
    };
    static const QColor colors[] = {
        QColor(59, 130, 246),   // #3b82f6
        QColor(22, 163, 74),    // #16a34a
        QColor(124, 58, 237),   // #7c3aed
        QColor(217, 119, 6),    // #d97706
        QColor(220, 38, 38)     // #dc2626
    };

    int maxCount = 0;
    for (const auto& cat : categories)
        maxCount = qMax(maxCount, counts.value(cat, 0));
    if (maxCount == 0) maxCount = 1;

    int barH = qMin(22, (rect.height() - 40) / static_cast<int>(categories.size()));
    int labelW = 90;
    int barAreaW = rect.width() - labelW - 40;
    int startY = rect.y() + 26;

    for (int i = 0; i < categories.size(); ++i) {
        int y = startY + i * (barH + 6);
        int count = counts.value(categories[i], 0);

        // Category label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y, labelW, barH, Qt::AlignVCenter | Qt::AlignRight,
                   categories[i]);

        // Bar background
        int barX = rect.x() + labelW + 8;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(241, 245, 249));
        QPainterPath barBg;
        barBg.addRoundedRect(barX, y + 2, barAreaW, barH - 4, 4, 4);
        p.drawPath(barBg);

        // Filled bar
        if (count > 0) {
            int filledW = static_cast<int>((static_cast<qreal>(count) / maxCount) * barAreaW);
            p.setBrush(colors[i]);
            QPainterPath barFill;
            barFill.addRoundedRect(barX, y + 2, qMax(filledW, 6), barH - 4, 4, 4);
            p.drawPath(barFill);
        }

        // Count label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(barX + barAreaW + 6, y, 30, barH, Qt::AlignVCenter,
                   QString::number(count));
    }
}

void PaperLogicProver::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Proofs",  QString::number(entries_.size()),                          QColor(59, 130, 246)},
        {"Valid Rate",    validCount() > 0
                              ? QString::number(static_cast<qreal>(validCount()) /
                                                entries_.size() * 100, 'f', 0) + "%"
                              : "0%",                                                 QColor(22, 163, 74)},
        {"Avg Confidence",QString::number(avgConfidence() * 100, 'f', 0) + "%",      QColor(124, 58, 237)},
        {"Avg Steps",     entries_.isEmpty()
                              ? "0"
                              : QString::number(static_cast<qreal>(
                                    std::accumulate(entries_.begin(), entries_.end(), 0,
                                        [](int s, const LogicProverEntry& e) {
                                            return s + e.steps;
                                        })) / entries_.size(), 'f', 1),              QColor(217, 119, 6)}
    };

    int boxCount = stats.size();
    int gap = 10;
    int boxW = (rect.width() - gap * (boxCount - 1)) / boxCount;
    int boxH = qMin(56, rect.height() - 4);

    for (int i = 0; i < boxCount; ++i) {
        int x = rect.x() + i * (boxW + gap);
        int y = rect.y();

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(192));
        QPainterPath box;
        box.addRoundedRect(x, y, boxW, boxH, 6, 6);
        p.drawPath(box);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 16, QFont::Bold));
        p.drawText(x + 12, y + 4, boxW - 24, 26, Qt::AlignVCenter, stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(x + 12, y + 30, boxW - 24, 18, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperLogicProver::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Logic Prover ready");
        return;
    }
    infoLabel_->setText(QString("%1 proofs | %2 valid | %3% confidence")
        .arg(entries_.size())
        .arg(validCount())
        .arg(avgConfidence() * 100, 0, 'f', 0));
}

void PaperLogicProver::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        LogicProverEntry e;
        e.id         = settings_.value("id").toInt();
        e.axiom      = settings_.value("axiom").toString();
        e.category   = settings_.value("category").toString();
        e.proof      = settings_.value("proof").toString();
        e.confidence = settings_.value("confidence").toDouble();
        e.steps      = settings_.value("steps").toInt();
        e.valid      = settings_.value("valid").toBool();
        e.color      = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperLogicProver::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",         entries_[i].id);
        settings_.setValue("axiom",      entries_[i].axiom);
        settings_.setValue("category",   entries_[i].category);
        settings_.setValue("proof",      entries_[i].proof);
        settings_.setValue("confidence", entries_[i].confidence);
        settings_.setValue("steps",      entries_[i].steps);
        settings_.setValue("valid",      entries_[i].valid);
        settings_.setValue("color",      entries_[i].color.name());
    }
    settings_.endArray();
}
