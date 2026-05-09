#include "analysis/PaperReproducibilityChecker.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

PaperReproducibilityChecker::PaperReproducibilityChecker(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReproducibilityChecker")
{
    setupUI();
    loadSettings();
}

void PaperReproducibilityChecker::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperReproducibilityChecker::onAdd);
    toolbar->addWidget(addBtn_);

    evaluateBtn_ = new QPushButton("Evaluate");
    connect(evaluateBtn_, &QPushButton::clicked, this, &PaperReproducibilityChecker::onEvaluate);
    toolbar->addWidget(evaluateBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReproducibilityChecker::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter reproducibility criterion...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Check paper reproducibility");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperReproducibilityChecker::addCheck(const ReproCheck& check) {
    checks_.append(check);
    saveSettings();
    updateInfo();
    update();
}

QList<ReproCheck> PaperReproducibilityChecker::checks() const { return checks_; }

QMap<QString, int> PaperReproducibilityChecker::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& c : checks_) counts[c.category]++;
    return counts;
}

qreal PaperReproducibilityChecker::reproducibilityScore() const {
    if (checks_.isEmpty()) return 0;
    qreal weightedPass = 0, totalWeight = 0;
    for (const auto& c : checks_) {
        totalWeight += c.importance;
        if (c.passed) weightedPass += c.importance;
    }
    return totalWeight > 0 ? weightedPass / totalWeight : 0;
}

int PaperReproducibilityChecker::passedCount() const {
    int c = 0;
    for (const auto& ch : checks_) if (ch.passed) c++;
    return c;
}

void PaperReproducibilityChecker::onAdd() {
    QString criterion = inputField_->text().trimmed();
    if (criterion.isEmpty()) return;

    bool ok;
    QStringList cats = {"data", "code", "method", "documentation"};
    QString cat = QInputDialog::getItem(this, "Add Check", "Category:", cats, 0, false, &ok);
    if (!ok) return;

    ReproCheck c;
    c.id = checks_.size() + 1;
    c.criterion = criterion;
    c.category = cat;
    c.passed = QRandomGenerator::global()->bounded(2) == 1;
    c.importance = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
    c.notes = c.passed ? "Available" : "Missing";

    if (c.passed) c.color = QColor(16, 185, 129);
    else if (c.importance >= 0.7) c.color = QColor(239, 68, 68);
    else c.color = QColor(245, 158, 11);

    addCheck(c);
    inputField_->clear();
}

void PaperReproducibilityChecker::onEvaluate() {
    if (checks_.isEmpty()) return;
    emit checkCompleted(passedCount(), checks_.size());
    emit scoreUpdated(reproducibilityScore());
    update();
}

void PaperReproducibilityChecker::onClear() {
    checks_.clear();
    saveSettings();
    infoLabel_->setText("Check paper reproducibility");
    update();
}

void PaperReproducibilityChecker::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (checks_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Check paper reproducibility");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reproducibility Checker");

    int w = width(), h = height();
    drawCheckList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawScoreGauge(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 10, w / 2 - 30, h / 2 - 40));
}

void PaperReproducibilityChecker::drawCheckList(QPainter& p, const QRect& rect) {
    int show = qMin(10, checks_.size());
    int itemH = qMin(36, (rect.height() - 10) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& c = checks_[i];
        int y = rect.y() + i * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(c.passed ? QColor(16,185,129).lighter(180) : c.color.lighter(180));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(c.passed ? QColor(16,185,129) : c.color);
        QString icon = c.passed ? QString::fromUtf8("✓") : QString::fromUtf8("✗");
        p.setFont(QFont("Arial", 10, QFont::Bold));
        p.drawText(rect.x() + 6, y, 20, itemH, Qt::AlignVCenter, icon);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 28, y + 4, rect.width() - 36, 16, Qt::AlignVCenter,
                   c.criterion.left(22));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 28, y + 20, rect.width() - 36, 14, Qt::AlignVCenter,
                   c.category + " | " + c.notes);
    }
}

void PaperReproducibilityChecker::drawScoreGauge(QPainter& p, const QRect& rect) {
    qreal score = reproducibilityScore();
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + rect.height() / 2;
    int radius = qMin(rect.width(), rect.height()) / 2 - 20;

    p.setPen(QPen(QColor(241, 245, 249), 10));
    p.drawArc(cx - radius, cy - radius, radius * 2, radius * 2, 0, 360 * 16);

    QColor scoreColor = score >= 0.8 ? QColor(16,185,129) : score >= 0.5 ? QColor(245,158,11) : QColor(239,68,68);
    p.setPen(QPen(scoreColor, 10));
    int span = static_cast<int>(score * 360 * 16);
    p.drawArc(cx - radius, cy - radius, radius * 2, radius * 2, 90 * 16, -span);

    p.setPen(scoreColor);
    p.setFont(QFont("Arial", 20, QFont::Bold));
    p.drawText(QRect(cx - 30, cy - 15, 60, 30), Qt::AlignCenter,
               QString::number(score * 100, 'f', 0) + "%");

    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 9));
    p.drawText(QRect(cx - 40, cy + 15, 80, 20), Qt::AlignCenter, "Reproducibility");
}

void PaperReproducibilityChecker::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Checks", QString::number(checks_.size()), QColor(59,130,246)},
        {"Passed", QString::number(passedCount()), QColor(16,185,129)},
        {"Failed", QString::number(checks_.size() - passedCount()), QColor(239,68,68)},
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

void PaperReproducibilityChecker::updateInfo() {
    if (checks_.isEmpty()) { infoLabel_->setText("Check paper reproducibility"); return; }
    infoLabel_->setText(QString("%1 checks | %2 passed | score: %3%")
        .arg(checks_.size()).arg(passedCount()).arg(reproducibilityScore() * 100, 0, 'f', 0));
}

void PaperReproducibilityChecker::loadSettings() {
    int size = settings_.beginReadArray("checks");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ReproCheck c;
        c.id = settings_.value("id").toInt();
        c.criterion = settings_.value("criterion").toString();
        c.category = settings_.value("category").toString();
        c.passed = settings_.value("passed").toBool();
        c.importance = settings_.value("importance").toDouble();
        c.notes = settings_.value("notes").toString();
        c.color = QColor(settings_.value("color").toString());
        checks_.append(c);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReproducibilityChecker::saveSettings() {
    settings_.beginWriteArray("checks");
    for (int i = 0; i < checks_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", checks_[i].id);
        settings_.setValue("criterion", checks_[i].criterion);
        settings_.setValue("category", checks_[i].category);
        settings_.setValue("passed", checks_[i].passed);
        settings_.setValue("importance", checks_[i].importance);
        settings_.setValue("notes", checks_[i].notes);
        settings_.setValue("color", checks_[i].color.name());
    }
    settings_.endArray();
}
