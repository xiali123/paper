#include "workspace/PaperRiskMatrix.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperRiskMatrix::PaperRiskMatrix(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "RiskMatrix")
{
    setupUI();
    loadSettings();
}

void PaperRiskMatrix::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    assessBtn_ = new QPushButton("Assess");
    assessBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(assessBtn_, &QPushButton::clicked, this, &PaperRiskMatrix::onAssess);
    toolbar->addWidget(assessBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Technical", "Schedule", "Resource", "Scope"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperRiskMatrix::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter risk name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Assess risks");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperRiskMatrix::addEntry(const RiskEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit riskIdentified(entry.id, entry.score);
    update();
}

QList<RiskEntry> PaperRiskMatrix::entries() const { return entries_; }

int PaperRiskMatrix::criticalCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.critical) c++;
    return c;
}

qreal PaperRiskMatrix::avgScore() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.score;
    return sum / entries_.size();
}

QMap<QString, int> PaperRiskMatrix::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperRiskMatrix::onAssess() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"technical", "schedule", "resource", "scope"};
    QStringList likelihoods = {"rare", "unlikely", "possible", "likely", "certain"};
    QStringList impacts = {"negligible", "minor", "moderate", "major", "severe"};
    QStringList mitigations = {"accept", "mitigate", "transfer", "avoid"};

    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        RiskEntry e;
        e.id = entries_.size() + 1;
        e.risk = text.left(12) + " R" + QString::number(i);
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        int lIdx = 1 + QRandomGenerator::global()->bounded(5);
        int iIdx = 1 + QRandomGenerator::global()->bounded(5);
        e.likelihood = likelihoods[lIdx - 1];
        e.impact = impacts[iIdx - 1];
        e.score = lIdx * iIdx;
        e.mitigation = mitigations[QRandomGenerator::global()->bounded(mitigations.size())];
        e.critical = e.score >= 15;

        if (e.score >= 15)
            e.color = QColor(239, 68, 68);
        else if (e.score >= 9)
            e.color = QColor(245, 158, 11);
        else
            e.color = QColor(16, 185, 129);

        addEntry(e);
    }
    inputField_->clear();
}

void PaperRiskMatrix::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Assess risks");
    update();
}

void PaperRiskMatrix::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Assess risks");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Risk Matrix");

    int w = width(), h = height();
    drawRiskList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawMatrixView(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperRiskMatrix::drawRiskList(QPainter& p, const QRect& rect) {
    int filterIdx = categoryCombo_->currentIndex();
    int show = 0;
    int maxShow = 10;
    int itemH = qMin(34, (rect.height() - 10) / maxShow);

    for (int i = entries_.size() - 1; i >= 0 && show < maxShow; --i) {
        const auto& e = entries_[i];
        if (filterIdx == 1 && e.category != "technical") continue;
        if (filterIdx == 2 && e.category != "schedule") continue;
        if (filterIdx == 3 && e.category != "resource") continue;
        if (filterIdx == 4 && e.category != "scope") continue;

        int y = rect.y() + show * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   (e.critical ? QString("[!!] ") : QString("[OK] ")) + e.risk.left(14));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.likelihood + " | " + e.impact);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "Score: " + QString::number(e.score));
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.mitigation);
        show++;
    }
}

void PaperRiskMatrix::drawMatrixView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Risk Matrix (Likelihood x Impact)");

    int gridX = rect.x() + 30;
    int gridY = rect.y() + 22;
    int gridW = rect.width() - 40;
    int gridH = rect.height() - 40;
    int cellW = gridW / 5;
    int cellH = gridH / 5;

    // Color mapping for cells: (likelihood_idx, impact_idx) -> risk level
    // Rows: likelihood (top=rare=1, bottom=certain=5)
    // Cols: impact (left=negligible=1, right=severe=5)
    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 5; ++col) {
            int lVal = 5 - row;  // top row = rare(1), bottom = certain(5)
            int iVal = col + 1;  // left col = negligible(1), right = severe(5)
            int score = lVal * iVal;

            QColor cellColor;
            if (score >= 15)
                cellColor = QColor(239, 68, 68, 60);
            else if (score >= 9)
                cellColor = QColor(245, 158, 11, 60);
            else if (score >= 4)
                cellColor = QColor(234, 179, 8, 40);
            else
                cellColor = QColor(16, 185, 129, 40);

            p.setPen(QColor(203, 213, 225));
            p.setBrush(cellColor);
            p.drawRect(gridX + col * cellW, gridY + row * cellH, cellW, cellH);
        }
    }

    // Axis labels
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 6));
    QStringList impactLabels = {"Negl", "Minor", "Mod", "Major", "Severe"};
    for (int col = 0; col < 5; ++col) {
        p.drawText(gridX + col * cellW, gridY + 5 * cellH + 2, cellW, 12,
                   Qt::AlignCenter, impactLabels[col]);
    }

    p.save();
    p.translate(gridX - 2, gridY);
    QStringList likelihoodLabels = {"Certain", "Likely", "Poss", "Unlik", "Rare"};
    for (int row = 0; row < 5; ++row) {
        p.drawText(-28, row * cellH + cellH / 2 + 4, 26, 12, Qt::AlignRight | Qt::AlignVCenter,
                   likelihoodLabels[row]);
    }
    p.restore();

    // Plot risk entries as dots
    QStringList likelihoods = {"rare", "unlikely", "possible", "likely", "certain"};
    QStringList impacts = {"negligible", "minor", "moderate", "major", "severe"};

    for (const auto& e : entries_) {
        int lIdx = likelihoods.indexOf(e.likelihood);
        int iIdx = impacts.indexOf(e.impact);
        if (lIdx < 0 || iIdx < 0) continue;

        // Row for likelihood: rare=0(top) -> certain=4(bottom), but our row 0 is top
        int row = 4 - lIdx;
        int col = iIdx;

        qreal cx = gridX + col * cellW + cellW / 2;
        qreal cy = gridY + row * cellH + cellH / 2;

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawEllipse(QPointF(cx, cy), 5, 5);
    }
}

void PaperRiskMatrix::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Risks", QString::number(entries_.size()), QColor(59,130,246)},
        {"Critical", QString::number(criticalCount()), QColor(239,68,68)},
        {"Avg Score", QString::number(avgScore(), 'f', 1), QColor(245,158,11)},
        {"Categories", QString::number(categoryCounts().size()), QColor(139,92,246)}
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

void PaperRiskMatrix::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Assess risks"); return; }
    infoLabel_->setText(QString("%1 risks | %2 critical | %3 avg score")
        .arg(entries_.size()).arg(criticalCount()).arg(avgScore(), 0, 'f', 1));
}

void PaperRiskMatrix::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        RiskEntry e;
        e.id = settings_.value("id").toInt();
        e.risk = settings_.value("risk").toString();
        e.category = settings_.value("category").toString();
        e.likelihood = settings_.value("likelihood").toString();
        e.impact = settings_.value("impact").toString();
        e.score = settings_.value("score").toInt();
        e.mitigation = settings_.value("mitigation").toString();
        e.critical = settings_.value("critical").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperRiskMatrix::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("risk", entries_[i].risk);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("likelihood", entries_[i].likelihood);
        settings_.setValue("impact", entries_[i].impact);
        settings_.setValue("score", entries_[i].score);
        settings_.setValue("mitigation", entries_[i].mitigation);
        settings_.setValue("critical", entries_[i].critical);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
