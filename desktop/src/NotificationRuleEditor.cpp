#include "NotificationRuleEditor.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>

NotificationRuleEditor::NotificationRuleEditor(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadSettings();
}

void NotificationRuleEditor::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // Rule editor
    auto* formRow = new QHBoxLayout();
    nameEdit_ = new QLineEdit();
    nameEdit_->setPlaceholderText("Rule name");
    formRow->addWidget(nameEdit_, 1);

    eventCombo_ = new QComboBox();
    eventCombo_->addItems({"paper_added", "paper_updated", "search_done", "export_done",
                            "crawl_complete", "ai_review_done", "favorite_added", "rating_changed"});
    formRow->addWidget(new QLabel("When:"));
    formRow->addWidget(eventCombo_);

    conditionCombo_ = new QComboBox();
    conditionCombo_->addItems({"always", "contains", "equals", "starts_with", "regex", "greater_than", "less_than"});
    formRow->addWidget(new QLabel("If:"));
    formRow->addWidget(conditionCombo_);

    valueEdit_ = new QLineEdit();
    valueEdit_->setPlaceholderText("Condition value");
    formRow->addWidget(valueEdit_, 1);

    actionCombo_ = new QComboBox();
    actionCombo_->addItems({"show_toast", "play_sound", "send_email", "log", "desktop_notification", "webhook"});
    formRow->addWidget(new QLabel("Do:"));
    formRow->addWidget(actionCombo_);

    addBtn_ = new QPushButton("Add Rule");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &NotificationRuleEditor::onAdd);
    formRow->addWidget(addBtn_);

    layout->addLayout(formRow);

    // Rule list
    ruleList_ = new QListWidget();
    ruleList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 6px; }"
        "QListWidget::item:selected { background: #3b82f6; color: white; }"
    );
    layout->addWidget(ruleList_, 1);

    // Buttons
    auto* btnRow = new QHBoxLayout();
    toggleBtn_ = new QPushButton("Enable/Disable");
    connect(toggleBtn_, &QPushButton::clicked, this, &NotificationRuleEditor::onToggle);
    btnRow->addWidget(toggleBtn_);

    removeBtn_ = new QPushButton("Remove");
    removeBtn_->setStyleSheet("color: #dc2626;");
    connect(removeBtn_, &QPushButton::clicked, this, &NotificationRuleEditor::onRemove);
    btnRow->addWidget(removeBtn_);

    testBtn_ = new QPushButton("Test Rules");
    testBtn_->setStyleSheet("QPushButton { background: #8b5cf6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(testBtn_, &QPushButton::clicked, this, &NotificationRuleEditor::onTest);
    btnRow->addWidget(testBtn_);

    btnRow->addStretch();
    layout->addLayout(btnRow);

    statsLabel_ = new QLabel("0 rules");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);
}

void NotificationRuleEditor::addRule(const NotificationRule& rule) {
    NotificationRule r = rule;
    if (r.id < 0) r.id = nextId_++;
    rules_.append(r);
    nextId_ = qMax(nextId_, r.id + 1);
    refreshList();
    saveSettings();
    emit ruleCreated(r);
}

void NotificationRuleEditor::removeRule(int ruleId) {
    rules_.removeIf([ruleId](const NotificationRule& r) { return r.id == ruleId; });
    refreshList();
    saveSettings();
    emit ruleDeleted(ruleId);
}

void NotificationRuleEditor::enableRule(int ruleId, bool enabled) {
    for (auto& r : rules_) {
        if (r.id == ruleId) { r.enabled = enabled; break; }
    }
    refreshList();
    saveSettings();
}

QList<NotificationRule> NotificationRuleEditor::rules() const { return rules_; }

QList<NotificationRule> NotificationRuleEditor::matchRules(const QString& event, const QString& data) const {
    QList<NotificationRule> matched;
    for (const auto& r : rules_) {
        if (!r.enabled) continue;
        if (r.event != event && r.event != "*") continue;

        bool condMet = false;
        if (r.condition == "always") condMet = true;
        else if (r.condition == "contains") condMet = data.contains(r.conditionValue);
        else if (r.condition == "equals") condMet = (data == r.conditionValue);
        else if (r.condition == "starts_with") condMet = data.startsWith(r.conditionValue);
        else if (r.condition == "regex") condMet = data.contains(QRegularExpression(r.conditionValue));
        else if (r.condition == "greater_than") condMet = (data.toDouble() > r.conditionValue.toDouble());
        else if (r.condition == "less_than") condMet = (data.toDouble() < r.conditionValue.toDouble());

        if (condMet) matched.append(r);
    }
    return matched;
}

void NotificationRuleEditor::onAdd() {
    if (nameEdit_->text().trimmed().isEmpty()) return;
    NotificationRule rule;
    rule.name = nameEdit_->text();
    rule.event = eventCombo_->currentText();
    rule.condition = conditionCombo_->currentText();
    rule.conditionValue = valueEdit_->text();
    rule.action = actionCombo_->currentText();
    rule.enabled = true;
    addRule(rule);
    nameEdit_->clear();
    valueEdit_->clear();
}

void NotificationRuleEditor::onRemove() {
    auto* item = ruleList_->currentItem();
    if (!item) return;
    int id = item->data(Qt::UserRole).toInt();
    removeRule(id);
}

void NotificationRuleEditor::onToggle() {
    auto* item = ruleList_->currentItem();
    if (!item) return;
    int id = item->data(Qt::UserRole).toInt();
    for (auto& r : rules_) {
        if (r.id == id) { enableRule(id, !r.enabled); return; }
    }
}

void NotificationRuleEditor::onTest() {
    int triggered = 0;
    for (const auto& r : rules_) {
        auto matched = matchRules(r.event, r.conditionValue.isEmpty() ? "test" : r.conditionValue);
        triggered += matched.size();
        for (const auto& m : matched) {
            emit ruleTriggered(m.id, m.action);
        }
    }
    statsLabel_->setText(QString("Test complete: %1 rule(s) triggered").arg(triggered));
}

void NotificationRuleEditor::refreshList() {
    ruleList_->clear();
    for (const auto& r : rules_) {
        QString display = QString("%1 | %2 %3 %4 → %5")
            .arg(r.enabled ? "ON " : "OFF")
            .arg(r.event)
            .arg(r.condition == "always" ? "" : QString("(if %1 %2)").arg(r.condition, r.conditionValue))
            .arg("", 0)
            .arg(r.action);
        auto* item = new QListWidgetItem(display);
        item->setData(Qt::UserRole, r.id);
        if (!r.enabled) item->setForeground(QColor(148, 163, 184));
        ruleList_->addItem(item);
    }
    int enabled = 0;
    for (const auto& r : rules_) if (r.enabled) enabled++;
    statsLabel_->setText(QString("%1 rules (%2 enabled)").arg(rules_.size()).arg(enabled));
}

void NotificationRuleEditor::loadSettings() {
    QSettings settings("PaperCrawler", "NotificationRules");
    QByteArray data = settings.value("rules").toByteArray();
    if (data.isEmpty()) return;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        NotificationRule r;
        r.id = obj["id"].toInt();
        r.name = obj["name"].toString();
        r.event = obj["event"].toString();
        r.condition = obj["condition"].toString();
        r.conditionValue = obj["conditionValue"].toString();
        r.action = obj["action"].toString();
        r.enabled = obj["enabled"].toBool();
        rules_.append(r);
        nextId_ = qMax(nextId_, r.id + 1);
    }
    refreshList();
}

void NotificationRuleEditor::saveSettings() {
    QJsonArray arr;
    for (const auto& r : rules_) {
        QJsonObject obj;
        obj["id"] = r.id;
        obj["name"] = r.name;
        obj["event"] = r.event;
        obj["condition"] = r.condition;
        obj["conditionValue"] = r.conditionValue;
        obj["action"] = r.action;
        obj["enabled"] = r.enabled;
        arr.append(obj);
    }
    QSettings settings("PaperCrawler", "NotificationRules");
    settings.setValue("rules", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}
