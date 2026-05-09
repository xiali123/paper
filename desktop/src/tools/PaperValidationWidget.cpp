#include "tools/PaperValidationWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

PaperValidationWidget::PaperValidationWidget(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ValidationWidget")
{
    setupUI();
    loadSettings();
}

void PaperValidationWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Rule");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperValidationWidget::onAdd);
    toolbar->addWidget(addBtn_);

    validateBtn_ = new QPushButton("Validate");
    connect(validateBtn_, &QPushButton::clicked, this, &PaperValidationWidget::onValidate);
    toolbar->addWidget(validateBtn_);

    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Pass", "Warning", "Fail"});
    connect(filterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperValidationWidget::onFilterChanged);
    toolbar->addWidget(filterCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperValidationWidget::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Validate paper metadata");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperValidationWidget::addRule(const ValidationRule& rule) {
    rules_.append(rule);
    saveSettings();
    updateInfo();
    update();
}

QList<ValidationRule> PaperValidationWidget::rules() const { return rules_; }

QMap<QString, int> PaperValidationWidget::statusCounts() const {
    QMap<QString, int> counts;
    for (const auto& r : rules_) counts[r.status]++;
    return counts;
}

qreal PaperValidationWidget::passRate() const {
    if (rules_.isEmpty()) return 0;
    int passed = 0;
    for (const auto& r : rules_) if (r.status == "pass") passed++;
    return static_cast<qreal>(passed) / rules_.size();
}

int PaperValidationWidget::totalChecks() const {
    int t = 0;
    for (const auto& r : rules_) t += r.checksRun;
    return t;
}

void PaperValidationWidget::onAdd() {
    bool ok;
    QString name = QInputDialog::getText(this, "Add Rule", "Rule name:", QLineEdit::Normal, "", &ok);
    if (!ok || name.isEmpty()) return;
    QStringList fields = {"title", "authors", "abstract", "references", "data"};
    QString field = QInputDialog::getItem(this, "Add Rule", "Field:", fields, 0, false, &ok);
    if (!ok) return;
    QStringList types = {"required", "format", "range", "custom"};
    QString type = QInputDialog::getItem(this, "Add Rule", "Type:", types, 0, false, &ok);
    if (!ok) return;

    ValidationRule r;
    r.id = rules_.size() + 1;
    r.name = name;
    r.field = field;
    r.type = type;
    r.checksRun = 1 + QRandomGenerator::global()->bounded(10);
    r.checksPassed = QRandomGenerator::global()->bounded(r.checksRun + 1);

    qreal ratio = static_cast<qreal>(r.checksPassed) / r.checksRun;
    if (ratio >= 0.9) { r.status = "pass"; r.color = QColor(16,185,129); }
    else if (ratio >= 0.6) { r.status = "warning"; r.color = QColor(245,158,11); }
    else { r.status = "fail"; r.color = QColor(239,68,68); }
    r.message = r.status == "pass" ? "All checks passed" : r.status == "warning" ? "Some issues found" : "Check failed";

    addRule(r);
}

void PaperValidationWidget::onValidate() {
    for (auto& r : rules_) {
        r.checksRun += 1 + QRandomGenerator::global()->bounded(5);
        r.checksPassed += QRandomGenerator::global()->bounded(3);
        r.checksPassed = qMin(r.checksPassed, r.checksRun);
        qreal ratio = static_cast<qreal>(r.checksPassed) / r.checksRun;
        if (ratio >= 0.9) { r.status = "pass"; r.color = QColor(16,185,129); }
        else if (ratio >= 0.6) { r.status = "warning"; r.color = QColor(245,158,11); }
        else { r.status = "fail"; r.color = QColor(239,68,68); }
    }
    saveSettings();
    updateInfo();
    emit validationComplete(
        [this]{ int c=0; for(const auto& r: rules_) if(r.status=="pass") c++; return c; }(),
        totalChecks());
    update();
}

void PaperValidationWidget::onFilterChanged(int) { update(); }

void PaperValidationWidget::onClear() {
    rules_.clear();
    saveSettings();
    infoLabel_->setText("Validate paper metadata");
    update();
}

void PaperValidationWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (rules_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Validate paper metadata");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Paper Validation");

    int w = width(), h = height();
    drawRuleList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawResultChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperValidationWidget::drawRuleList(QPainter& p, const QRect& rect) {
    int filterIdx = filterCombo_->currentIndex();
    int show = 0;
    int maxShow = 10;
    int itemH = qMin(36, (rect.height() - 10) / maxShow);

    for (int i = rules_.size() - 1; i >= 0 && show < maxShow; --i) {
        const auto& r = rules_[i];
        if (filterIdx == 1 && r.status != "pass") continue;
        if (filterIdx == 2 && r.status != "warning") continue;
        if (filterIdx == 3 && r.status != "fail") continue;

        int y = rect.y() + show * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(r.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(r.color);
        p.drawEllipse(rect.x() + 8, y + itemH / 2 - 4, 8, 8);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 22, y + 4, rect.width() - 30, 16, Qt::AlignVCenter,
                   r.name.left(20));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 22, y + 20, rect.width() / 2, 14, Qt::AlignVCenter,
                   r.field + " | " + r.type);
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight, r.message);
        show++;
    }
}

void PaperValidationWidget::drawResultChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Results");

    auto counts = statusCounts();
    QStringList statuses = {"pass", "warning", "fail"};
    QString labels[] = {"Pass", "Warning", "Fail"};
    QColor colors[] = {QColor(16,185,129), QColor(245,158,11), QColor(239,68,68)};

    int barW = (rect.width() - 30) / 3;
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    for (int i = 0; i < 3; ++i) {
        int x = rect.x() + 10 + i * barW;
        int count = counts.contains(statuses[i]) ? counts[statuses[i]] : 0;
        qreal h = (static_cast<qreal>(count) / maxVal) * (rect.height() - 55);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(x + 4, rect.bottom() - 25 - static_cast<int>(h), barW - 8, static_cast<int>(h), 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(x, rect.bottom() - 8, barW, 14, Qt::AlignCenter, labels[i]);
    }
}

void PaperValidationWidget::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Rules", QString::number(rules_.size()), QColor(59,130,246)},
        {"Pass Rate", QString::number(passRate() * 100, 'f', 0) + "%", QColor(16,185,129)},
        {"Checks", QString::number(totalChecks()), QColor(245,158,11)},
        {"Failed", QString::number(statusCounts().value("fail", 0)), QColor(239,68,68)}
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

void PaperValidationWidget::updateInfo() {
    if (rules_.isEmpty()) { infoLabel_->setText("Validate paper metadata"); return; }
    infoLabel_->setText(QString("%1 rules | %2% pass | %3 checks")
        .arg(rules_.size()).arg(passRate() * 100, 0, 'f', 0).arg(totalChecks()));
}

void PaperValidationWidget::loadSettings() {
    int size = settings_.beginReadArray("rules");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ValidationRule r;
        r.id = settings_.value("id").toInt();
        r.name = settings_.value("name").toString();
        r.field = settings_.value("field").toString();
        r.type = settings_.value("type").toString();
        r.status = settings_.value("status").toString();
        r.checksRun = settings_.value("checksRun").toInt();
        r.checksPassed = settings_.value("checksPassed").toInt();
        r.message = settings_.value("message").toString();
        r.color = QColor(settings_.value("color").toString());
        rules_.append(r);
    }
    settings_.endArray();
    updateInfo();
}

void PaperValidationWidget::saveSettings() {
    settings_.beginWriteArray("rules");
    for (int i = 0; i < rules_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", rules_[i].id);
        settings_.setValue("name", rules_[i].name);
        settings_.setValue("field", rules_[i].field);
        settings_.setValue("type", rules_[i].type);
        settings_.setValue("status", rules_[i].status);
        settings_.setValue("checksRun", rules_[i].checksRun);
        settings_.setValue("checksPassed", rules_[i].checksPassed);
        settings_.setValue("message", rules_[i].message);
        settings_.setValue("color", rules_[i].color.name());
    }
    settings_.endArray();
}
