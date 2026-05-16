#include "workspace/PaperContradictionHunter2.hpp"
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperContradictionHunter2::PaperContradictionHunter2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ContradictionHunter2")
{
    setupUI();
    loadSettings();
}

void PaperContradictionHunter2::setupUI() {
    auto* layout = new QHBoxLayout(this);

    auto* left = new QHBoxLayout();
    left->setSpacing(6);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Science", "Medicine", "Policy", "Ethics", "Law"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    left->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter claim text...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    left->addWidget(inputField_, 1);

    huntBtn_ = new QPushButton("Hunt");
    huntBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(huntBtn_, &QPushButton::clicked, this, &PaperContradictionHunter2::onHunt);
    left->addWidget(huntBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperContradictionHunter2::onClear);
    left->addWidget(clearBtn_);

    infoLabel_ = new QLabel("Claims: 0 | Unresolved: 0 | Avg Severity: 0.00");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    left->addWidget(infoLabel_);

    layout->addLayout(left);
    layout->addStretch();

    setMinimumSize(720, 520);
}

void PaperContradictionHunter2::addEntry(const ContradictionHunter2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit contradictionFound(entry.id, entry.severity);
    update();
}

QList<ContradictionHunter2Entry> PaperContradictionHunter2::entries() const {
    return entries_;
}

int PaperContradictionHunter2::unresolvedCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (!e.resolved) ++c;
    return c;
}

qreal PaperContradictionHunter2::avgSeverity() const {
    if (entries_.isEmpty()) return 0.0;
    qreal total = 0;
    for (const auto& e : entries_) total += e.severity;
    return total / entries_.size();
}

QMap<QString, int> PaperContradictionHunter2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperContradictionHunter2::onHunt() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"Science", "Medicine", "Policy", "Ethics", "Law"};
    QStringList conflicts = {"Direct Contradiction", "Partial Overlap",
                             "Temporal Conflict", "Context Mismatch"};

    QColor palette[] = {
        QColor(59, 130, 246),   // #3b82f6
        QColor(22, 163, 74),    // #16a34a
        QColor(217, 119, 6),    // #d97706
        QColor(220, 38, 38),    // #dc2626
        QColor(124, 58, 237)    // #7c3aed
    };

    int catIdx = categoryCombo_->currentIndex();
    ContradictionHunter2Entry e;
    e.id = entries_.size() + 1;
    e.claim = text;
    e.category = catIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())]
                             : categories[catIdx - 1];
    e.conflict = conflicts[QRandomGenerator::global()->bounded(conflicts.size())];
    e.severity = QRandomGenerator::global()->generateDouble();
    e.contradictions = 1 + QRandomGenerator::global()->bounded(10);
    e.resolved = QRandomGenerator::global()->bounded(3) == 0;
    e.color = palette[QRandomGenerator::global()->bounded(5)];

    addEntry(e);
    inputField_->clear();
}

void PaperContradictionHunter2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    repaint();
}

void PaperContradictionHunter2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No contradictions found yet");
        return;
    }

    int w = width(), h = height();
    int colW = w / 3;

    drawHunterView(p, QRect(10, 10, colW - 15, h - 20));
    drawCategoryChart(p, QRect(colW + 5, 10, colW - 15, h - 20));
    drawStats(p, QRect(2 * colW + 5, 10, colW - 15, h - 20));
}

void PaperContradictionHunter2::drawHunterView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 18, "Contradiction Hunter");

    int show = qMin(10, entries_.size());
    int itemH = qMin(38, (rect.height() - 35) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + 30 + i * (itemH + 3);

        // Background
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x() + 4, y, rect.width() - 8, itemH, 4, 4);

        // Left accent stripe
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x() + 4, y, 4, itemH, 2, 2);

        // Claim text
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 12, y + 2, rect.width() / 2, 16, Qt::AlignVCenter,
                   e.claim.left(20));

        // Conflict type badge
        QFontMetrics fm(QFont("Arial", 7));
        QString badgeText = e.conflict;
        int badgeW = fm.horizontalAdvance(badgeText) + 10;

        QColor badgeColor;
        if (e.conflict == "Direct Contradiction")
            badgeColor = QColor(220, 38, 38);
        else if (e.conflict == "Partial Overlap")
            badgeColor = QColor(217, 119, 6);
        else if (e.conflict == "Temporal Conflict")
            badgeColor = QColor(59, 130, 246);
        else
            badgeColor = QColor(124, 58, 237);

        p.setPen(Qt::NoPen);
        p.setBrush(badgeColor);
        p.drawRoundedRect(rect.x() + 12, y + 18, badgeW, 14, 3, 3);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 17, y + 18, badgeW, 14, Qt::AlignVCenter, badgeText);

        // Severity bar
        int barX = rect.x() + rect.width() / 2 + 4;
        int barW = rect.width() / 4;
        int barH = 8;
        int barY = y + 6;

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        p.drawRoundedRect(barX, barY, barW, barH, 3, 3);

        QColor severityColor;
        if (e.severity > 0.7)
            severityColor = QColor(220, 38, 38);
        else if (e.severity > 0.4)
            severityColor = QColor(217, 119, 6);
        else
            severityColor = QColor(22, 163, 74);

        int fillW = static_cast<int>(e.severity * barW);
        p.setBrush(severityColor);
        p.drawRoundedRect(barX, barY, fillW, barH, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(barX, barY + barH + 2, barW, 12, Qt::AlignVCenter,
                   QString::number(e.severity, 'f', 2));

        // Contradiction count
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(barX, barY + barH + 14, barW, 12, Qt::AlignVCenter,
                   QString::number(e.contradictions) + " conflicts");

        // Resolved / Active indicator
        int indX = rect.x() + rect.width() - 60;
        int indY = y + (itemH / 2) - 7;
        int indW = 52;
        int indH = 16;

        p.setPen(Qt::NoPen);
        if (e.resolved) {
            p.setBrush(QColor(22, 163, 74).lighter(150));
            p.drawRoundedRect(indX, indY, indW, indH, 3, 3);
            p.setPen(QColor(22, 163, 74));
        } else {
            p.setBrush(QColor(220, 38, 38).lighter(150));
            p.drawRoundedRect(indX, indY, indW, indH, 3, 3);
            p.setPen(QColor(220, 38, 38));
        }
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(indX, indY, indW, indH, Qt::AlignCenter,
                   e.resolved ? "Resolved" : "Active");
    }
}

void PaperContradictionHunter2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 18, "Category Breakdown");

    auto counts = categoryCounts();
    QStringList categories = {"Science", "Medicine", "Policy", "Ethics", "Law"};
    QColor catColors[] = {
        QColor(59, 130, 246),   // #3b82f6
        QColor(22, 163, 74),    // #16a34a
        QColor(217, 119, 6),    // #d97706
        QColor(220, 38, 38),    // #dc2626
        QColor(124, 58, 237)    // #7c3aed
    };

    // Stacked bar chart: one tall bar per category, stacked by conflict type
    QStringList conflicts = {"Direct Contradiction", "Partial Overlap",
                             "Temporal Conflict", "Context Mismatch"};
    QColor conflictColors[] = {
        QColor(220, 38, 38),   // #dc2626 red
        QColor(217, 119, 6),   // #d97706 amber
        QColor(59, 130, 246),  // #3b82f6 blue
        QColor(124, 58, 237)   // #7c3aed purple
    };

    // Count entries per (category, conflict)
    QMap<QString, QMap<QString, int>> matrix;
    for (const auto& e : entries_)
        matrix[e.category][e.conflict]++;

    int maxTotal = 1;
    for (const auto& cat : categories) {
        int total = 0;
        for (const auto& c : conflicts)
            total += matrix[cat].value(c, 0);
        maxTotal = qMax(maxTotal, total);
    }

    int barAreaTop = rect.y() + 32;
    int barAreaH = rect.height() - 70;
    int barW = qMin(36, (rect.width() - 20) / categories.size() - 6);
    int gap = (rect.width() - 20 - barW * categories.size()) / qMax(categories.size() - 1, 1);

    for (int i = 0; i < categories.size(); ++i) {
        int x = rect.x() + 10 + i * (barW + gap);
        int stackY = barAreaTop + barAreaH;

        for (int j = conflicts.size() - 1; j >= 0; --j) {
            int count = matrix[categories[i]].value(conflicts[j], 0);
            if (count <= 0) continue;
            int segH = static_cast<int>((static_cast<qreal>(count) / maxTotal) * barAreaH);

            p.setPen(Qt::NoPen);
            p.setBrush(conflictColors[j]);
            p.drawRoundedRect(x, stackY - segH, barW, segH, 2, 2);
            stackY -= segH;
        }

        // Category label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(x - 4, barAreaTop + barAreaH + 12, barW + 8, 16,
                   Qt::AlignCenter, categories[i]);
    }

    // Legend
    int legY = barAreaTop + barAreaH + 30;
    int legX = rect.x() + 4;
    for (int i = 0; i < conflicts.size(); ++i) {
        p.setPen(Qt::NoPen);
        p.setBrush(conflictColors[i]);
        p.drawRoundedRect(legX, legY, 8, 8, 2, 2);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(legX + 11, legY + 8, conflicts[i].left(12));
        legX += 68;
    }
}

void PaperContradictionHunter2::drawStats(QPainter& p, const QRect& rect) {
    int totalContradictions = 0;
    for (const auto& e : entries_) totalContradictions += e.contradictions;

    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Claims",        QString::number(entries_.size()),
                                QColor(59, 130, 246)},
        {"Unresolved Count",    QString::number(unresolvedCount()),
                                QColor(220, 38, 38)},
        {"Avg Severity",        QString::number(avgSeverity(), 'f', 2),
                                QColor(217, 119, 6)},
        {"Total Contradictions", QString::number(totalContradictions),
                                QColor(124, 58, 237)}
    };

    int boxH = qMin(60, (rect.height() - 20) / stats.size());

    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + 10 + i * (boxH + 6);

        // Box background
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        // Left accent stripe
        p.setBrush(stats[i].color);
        p.drawRoundedRect(rect.x(), y, 4, boxH, 2, 2);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 16, QFont::Bold));
        p.drawText(rect.x() + 12, y + 6, rect.width() - 20, 28, Qt::AlignVCenter,
                   stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 12, y + 34, rect.width() - 20, 18, Qt::AlignVCenter,
                   stats[i].label);
    }
}

void PaperContradictionHunter2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Claims: 0 | Unresolved: 0 | Avg Severity: 0.00");
        return;
    }
    infoLabel_->setText(QString("Claims: %1 | Unresolved: %2 | Avg Severity: %3")
        .arg(entries_.size())
        .arg(unresolvedCount())
        .arg(avgSeverity(), 0, 'f', 2));
}

void PaperContradictionHunter2::loadSettings() {
    entries_.clear();
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ContradictionHunter2Entry e;
        e.id             = settings_.value("id").toInt();
        e.claim          = settings_.value("claim").toString();
        e.category       = settings_.value("category").toString();
        e.conflict       = settings_.value("conflict").toString();
        e.severity       = settings_.value("severity").toDouble();
        e.contradictions = settings_.value("contradictions").toInt();
        e.resolved       = settings_.value("resolved").toBool();
        e.color          = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();

    if (entries_.isEmpty()) {
        // Seed 8 entries
        QStringList claims = {
            "Vaccines cause autism",
            "Climate change is natural",
            "DNA evidence is infallible",
            "AI will replace all jobs",
            "Dietary fat causes heart disease",
            "Gun control reduces crime",
            "Animal testing is unnecessary",
            "Patent law encourages innovation"
        };
        QStringList categories = {"Science", "Medicine", "Policy", "Ethics", "Law"};
        QStringList conflicts = {"Direct Contradiction", "Partial Overlap",
                                 "Temporal Conflict", "Context Mismatch"};
        QColor palette[] = {
            QColor(59, 130, 246),
            QColor(22, 163, 74),
            QColor(217, 119, 6),
            QColor(220, 38, 38),
            QColor(124, 58, 237)
        };

        for (int i = 0; i < 8; ++i) {
            ContradictionHunter2Entry e;
            e.id = i + 1;
            e.claim = claims[i];
            e.category = categories[i % categories.size()];
            e.conflict = conflicts[i % conflicts.size()];
            e.severity = 0.2 + (i * 0.1);
            if (e.severity > 1.0) e.severity = 1.0;
            e.contradictions = 1 + i;
            e.resolved = (i % 3 == 0);
            e.color = palette[i % 5];
            entries_.append(e);
        }
        saveSettings();
    }

    updateInfo();
}

void PaperContradictionHunter2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",             entries_[i].id);
        settings_.setValue("claim",          entries_[i].claim);
        settings_.setValue("category",       entries_[i].category);
        settings_.setValue("conflict",       entries_[i].conflict);
        settings_.setValue("severity",       entries_[i].severity);
        settings_.setValue("contradictions", entries_[i].contradictions);
        settings_.setValue("resolved",       entries_[i].resolved);
        settings_.setValue("color",          entries_[i].color.name());
    }
    settings_.endArray();
}
