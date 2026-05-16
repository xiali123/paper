#include "tools/PaperDataTransformWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

PaperDataTransformWidget::PaperDataTransformWidget(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "DataTransform")
{
    setupUI();
    loadSettings();
}

void PaperDataTransformWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Rule");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperDataTransformWidget::onAdd);
    toolbar->addWidget(addBtn_);

    runAllBtn_ = new QPushButton("Run All");
    connect(runAllBtn_, &QPushButton::clicked, this, &PaperDataTransformWidget::onRunAll);
    toolbar->addWidget(runAllBtn_);

    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Convert", "Normalize", "Merge", "Filter"});
    connect(filterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperDataTransformWidget::onFilterChanged);
    toolbar->addWidget(filterCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperDataTransformWidget::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Transform research data");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperDataTransformWidget::addRule(const TransformRule& rule) {
    rules_.append(rule);
    saveSettings();
    updateInfo();
    update();
}

QList<TransformRule> PaperDataTransformWidget::rules() const { return rules_; }

QMap<QString, int> PaperDataTransformWidget::operationCounts() const {
    QMap<QString, int> counts;
    for (const auto& r : rules_) counts[r.operation]++;
    return counts;
}

int PaperDataTransformWidget::activeRules() const {
    int c = 0;
    for (const auto& r : rules_) if (r.active) c++;
    return c;
}

int PaperDataTransformWidget::totalProcessed() const {
    int t = 0;
    for (const auto& r : rules_) t += r.recordsProcessed;
    return t;
}

void PaperDataTransformWidget::onAdd() {
    bool ok;
    QString name = QInputDialog::getText(this, "Add Rule", "Rule name:", QLineEdit::Normal, "", &ok);
    if (!ok || name.isEmpty()) return;
    QStringList ops = {"convert", "normalize", "merge", "filter"};
    QString op = QInputDialog::getItem(this, "Add Rule", "Operation:", ops, 0, false, &ok);
    if (!ok) return;
    QString input = QInputDialog::getText(this, "Add Rule", "Input format:", QLineEdit::Normal, "CSV", &ok);
    if (!ok) return;

    TransformRule r;
    r.id = rules_.size() + 1;
    r.name = name;
    r.inputFormat = input;
    r.outputFormat = "JSON";
    r.operation = op;
    r.recordsProcessed = QRandomGenerator::global()->bounded(1000);
    r.active = QRandomGenerator::global()->bounded(2) == 1;

    QColor opColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int oIdx = ops.indexOf(op);
    r.color = opColors[qBound(0, oIdx, 3)];
    addRule(r);
}

void PaperDataTransformWidget::onRunAll() {
    for (auto& r : rules_) {
        r.active = true;
        r.recordsProcessed += QRandomGenerator::global()->bounded(500);
    }
    saveSettings();
    updateInfo();
    emit transformComplete(totalProcessed());
    update();
}

void PaperDataTransformWidget::onFilterChanged(int) { update(); }

void PaperDataTransformWidget::onClear() {
    rules_.clear();
    saveSettings();
    infoLabel_->setText("Transform research data");
    update();
}

void PaperDataTransformWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (rules_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Transform research data");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Data Transform");

    int w = width(), h = height();
    drawRuleList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawOperationChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperDataTransformWidget::drawRuleList(QPainter& p, const QRect& rect) {
    int filterIdx = filterCombo_->currentIndex();
    int show = 0;
    int maxShow = 10;
    int itemH = qMin(36, (rect.height() - 10) / maxShow);

    for (int i = rules_.size() - 1; i >= 0 && show < maxShow; --i) {
        const auto& r = rules_[i];
        if (filterIdx == 1 && r.operation != "convert") continue;
        if (filterIdx == 2 && r.operation != "normalize") continue;
        if (filterIdx == 3 && r.operation != "merge") continue;
        if (filterIdx == 4 && r.operation != "filter") continue;

        int y = rect.y() + show * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(r.active ? r.color.lighter(190) : QColor(248, 250, 252));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(r.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        if (r.active) {
            p.setBrush(QColor(16,185,129));
            p.drawEllipse(rect.x() + rect.width() - 14, y + 6, 8, 8);
        }

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() - 20, 16, Qt::AlignVCenter,
                   r.name.left(22));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2, 14, Qt::AlignVCenter,
                   r.operation + " | " + r.inputFormat + " → " + r.outputFormat);
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(r.recordsProcessed) + " records");
        show++;
    }
}

void PaperDataTransformWidget::drawOperationChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "By Operation");

    auto counts = operationCounts();
    QStringList ops = {"convert", "normalize", "merge", "filter"};
    QString labels[] = {"Convert", "Normalize", "Merge", "Filter"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 4);
        int count = counts.contains(ops[i]) ? counts[ops[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x(), y + barH - 3, 65, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 70, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 73 + barW, y + barH - 3, QString::number(count));
    }
}

void PaperDataTransformWidget::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Rules", QString::number(rules_.size()), QColor(59,130,246)},
        {"Active", QString::number(activeRules()), QColor(16,185,129)},
        {"Processed", QString::number(totalProcessed()), QColor(245,158,11)},
        {"Operations", QString::number(operationCounts().size()), QColor(139,92,246)}
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

void PaperDataTransformWidget::updateInfo() {
    if (rules_.isEmpty()) { infoLabel_->setText("Transform research data"); return; }
    infoLabel_->setText(QString("%1 rules | %2 active | %3 processed")
        .arg(rules_.size()).arg(activeRules()).arg(totalProcessed()));
}

void PaperDataTransformWidget::loadSettings() {
    int size = settings_.beginReadArray("rules");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        TransformRule r;
        r.id = settings_.value("id").toInt();
        r.name = settings_.value("name").toString();
        r.inputFormat = settings_.value("inputFormat").toString();
        r.outputFormat = settings_.value("outputFormat").toString();
        r.operation = settings_.value("operation").toString();
        r.recordsProcessed = settings_.value("recordsProcessed").toInt();
        r.active = settings_.value("active").toBool();
        r.color = QColor(settings_.value("color").toString());
        rules_.append(r);
    }
    settings_.endArray();
    updateInfo();
}

void PaperDataTransformWidget::saveSettings() {
    settings_.beginWriteArray("rules");
    for (int i = 0; i < rules_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", rules_[i].id);
        settings_.setValue("name", rules_[i].name);
        settings_.setValue("inputFormat", rules_[i].inputFormat);
        settings_.setValue("outputFormat", rules_[i].outputFormat);
        settings_.setValue("operation", rules_[i].operation);
        settings_.setValue("recordsProcessed", rules_[i].recordsProcessed);
        settings_.setValue("active", rules_[i].active);
        settings_.setValue("color", rules_[i].color.name());
    }
    settings_.endArray();
}
