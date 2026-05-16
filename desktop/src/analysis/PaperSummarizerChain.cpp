#include "analysis/PaperSummarizerChain.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

PaperSummarizerChain::PaperSummarizerChain(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadSettings();
}

void PaperSummarizerChain::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* splitter = new QSplitter(Qt::Horizontal);

    // Left: chains
    auto* leftPanel = new QWidget();
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->addWidget(new QLabel("Chains:"));
    chainList_ = new QListWidget();
    chainList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 3px; }"
        "QListWidget::item:selected { background: #3b82f6; color: white; }"
    );
    connect(chainList_, &QListWidget::itemClicked, this, &PaperSummarizerChain::onChainSelected);
    leftLayout->addWidget(chainList_, 1);

    auto* chainBtnRow = new QHBoxLayout();
    createBtn_ = new QPushButton("New");
    createBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 3px 10px; border-radius: 4px; }");
    connect(createBtn_, &QPushButton::clicked, this, &PaperSummarizerChain::onCreateChain);
    chainBtnRow->addWidget(createBtn_);

    deleteBtn_ = new QPushButton("Delete");
    deleteBtn_->setStyleSheet("color: #dc2626;");
    connect(deleteBtn_, &QPushButton::clicked, this, &PaperSummarizerChain::onDeleteChain);
    chainBtnRow->addWidget(deleteBtn_);

    runBtn_ = new QPushButton("Run");
    runBtn_->setStyleSheet("QPushButton { background: #059669; color: white; padding: 3px 10px; border-radius: 4px; }");
    connect(runBtn_, &QPushButton::clicked, this, &PaperSummarizerChain::onRun);
    chainBtnRow->addWidget(runBtn_);
    leftLayout->addLayout(chainBtnRow);
    splitter->addWidget(leftPanel);

    // Center: steps + config
    auto* centerPanel = new QWidget();
    auto* centerLayout = new QVBoxLayout(centerPanel);
    centerLayout->setContentsMargins(0, 0, 0, 0);

    auto* nameRow = new QHBoxLayout();
    nameRow->addWidget(new QLabel("Name:"));
    nameEdit_ = new QLineEdit();
    nameEdit_->setPlaceholderText("Chain name...");
    nameRow->addWidget(nameEdit_, 1);
    centerLayout->addLayout(nameRow);

    centerLayout->addWidget(new QLabel("Steps:"));
    stepList_ = new QListWidget();
    stepList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 3px; }"
    );
    connect(stepList_, &QListWidget::itemClicked, this, &PaperSummarizerChain::onStepSelected);
    centerLayout->addWidget(stepList_, 1);

    auto* stepFormRow = new QHBoxLayout();
    stepFormRow->addWidget(new QLabel("Step:"));
    stepNameEdit_ = new QLineEdit();
    stepNameEdit_->setPlaceholderText("Step name...");
    stepFormRow->addWidget(stepNameEdit_, 1);
    centerLayout->addLayout(stepFormRow);

    promptEdit_ = new QTextEdit();
    promptEdit_->setMaximumHeight(50);
    promptEdit_->setPlaceholderText("Prompt template (e.g. 'Summarize in 3 bullet points')...");
    centerLayout->addWidget(promptEdit_);

    auto* stepBtnRow = new QHBoxLayout();
    addStepBtn_ = new QPushButton("Add Step");
    connect(addStepBtn_, &QPushButton::clicked, this, &PaperSummarizerChain::onAddStep);
    stepBtnRow->addWidget(addStepBtn_);

    removeStepBtn_ = new QPushButton("Remove");
    removeStepBtn_->setStyleSheet("color: #dc2626;");
    connect(removeStepBtn_, &QPushButton::clicked, this, &PaperSummarizerChain::onRemoveStep);
    stepBtnRow->addWidget(removeStepBtn_);
    centerLayout->addLayout(stepBtnRow);
    splitter->addWidget(centerPanel);

    // Right: input/output
    auto* rightPanel = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->addWidget(new QLabel("Input:"));
    inputEdit_ = new QTextEdit();
    inputEdit_->setPlaceholderText("Paste paper abstract or text...");
    rightLayout->addWidget(inputEdit_, 1);

    rightLayout->addWidget(new QLabel("Output:"));
    outputEdit_ = new QTextEdit();
    outputEdit_->setReadOnly(true);
    rightLayout->addWidget(outputEdit_, 1);
    splitter->addWidget(rightPanel);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 1);
    layout->addWidget(splitter, 1);

    statsLabel_ = new QLabel("0 chains");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);
}

void PaperSummarizerChain::addChain(const SummaryChain& chain) {
    SummaryChain c = chain;
    if (c.id < 0) c.id = nextChainId_++;
    if (c.createdAt == 0) c.createdAt = QDateTime::currentSecsSinceEpoch();
    chains_.append(c);
    nextChainId_ = qMax(nextChainId_, c.id + 1);
    refreshChainList();
    saveSettings();
    updateStats();
    emit chainCreated(c.id);
}

void PaperSummarizerChain::removeChain(int chainId) {
    chains_.removeIf([chainId](const SummaryChain& c) { return c.id == chainId; });
    if (selectedChainId_ == chainId) selectedChainId_ = -1;
    refreshChainList();
    refreshStepList();
    saveSettings();
    updateStats();
}

QList<SummaryChain> PaperSummarizerChain::chains() const { return chains_; }

void PaperSummarizerChain::runChain(int chainId, const QString& input) {
    for (auto& c : chains_) {
        if (c.id != chainId) continue;
        c.input = input;
        QString current = input;
        for (auto& step : c.steps) {
            if (!step.enabled) continue;
            step.output = processStep(current, step.prompt);
            current = step.output;
            emit stepCompleted(chainId, step.id, step.output);
        }
        c.finalOutput = current;
        outputEdit_->setPlainText(c.finalOutput);
        emit chainCompleted(chainId, c.finalOutput);
        break;
    }
}

void PaperSummarizerChain::onCreateChain() {
    SummaryChain c;
    c.name = nameEdit_->text().trimmed().isEmpty() ? "New Chain" : nameEdit_->text().trimmed();
    addChain(c);
    nameEdit_->clear();
}

void PaperSummarizerChain::onDeleteChain() {
    if (selectedChainId_ < 0) return;
    removeChain(selectedChainId_);
}

void PaperSummarizerChain::onAddStep() {
    if (selectedChainId_ < 0 || stepNameEdit_->text().trimmed().isEmpty()) return;
    for (auto& c : chains_) {
        if (c.id == selectedChainId_) {
            SummaryStep s;
            s.id = nextStepId_++;
            s.name = stepNameEdit_->text().trimmed();
            s.prompt = promptEdit_->toPlainText();
            c.steps.append(s);
            refreshStepList();
            saveSettings();
            break;
        }
    }
    stepNameEdit_->clear();
    promptEdit_->clear();
}

void PaperSummarizerChain::onRemoveStep() {
    auto* item = stepList_->currentItem();
    if (!item || selectedChainId_ < 0) return;
    int stepId = item->data(Qt::UserRole).toInt();
    for (auto& c : chains_) {
        if (c.id == selectedChainId_) {
            c.steps.removeIf([stepId](const SummaryStep& s) { return s.id == stepId; });
            refreshStepList();
            saveSettings();
            break;
        }
    }
}

void PaperSummarizerChain::onRun() {
    if (selectedChainId_ < 0) return;
    runChain(selectedChainId_, inputEdit_->toPlainText());
}

void PaperSummarizerChain::onChainSelected() {
    auto* item = chainList_->currentItem();
    if (!item) return;
    selectedChainId_ = item->data(Qt::UserRole).toInt();
    for (const auto& c : chains_) {
        if (c.id == selectedChainId_) {
            nameEdit_->setText(c.name);
            break;
        }
    }
    refreshStepList();
}

void PaperSummarizerChain::onStepSelected() {
    auto* item = stepList_->currentItem();
    if (!item || selectedChainId_ < 0) return;
    int stepId = item->data(Qt::UserRole).toInt();
    for (const auto& c : chains_) {
        if (c.id != selectedChainId_) continue;
        for (const auto& s : c.steps) {
            if (s.id == stepId) {
                stepNameEdit_->setText(s.name);
                promptEdit_->setPlainText(s.prompt);
                break;
            }
        }
        break;
    }
}

QString PaperSummarizerChain::processStep(const QString& input, const QString& prompt) {
    Q_UNUSED(prompt);
    QStringList sentences = input.split(QRegularExpression("[.!?]"), Qt::SkipEmptyParts);
    if (sentences.size() <= 2) return input;

    QString result;
    if (input.length() > 500) {
        int half = sentences.size() / 2;
        for (int i = 0; i < half; ++i) result += sentences[i].trimmed() + ". ";
    } else {
        for (int i = 0; i < qMin(3, sentences.size()); ++i) {
            result += sentences[i].trimmed() + ". ";
        }
    }
    return result.trimmed();
}

void PaperSummarizerChain::refreshChainList() {
    chainList_->clear();
    for (const auto& c : chains_) {
        QString display = QString("%1 (%2 steps)").arg(c.name).arg(c.steps.size());
        auto* item = new QListWidgetItem(display);
        item->setData(Qt::UserRole, c.id);
        chainList_->addItem(item);
    }
}

void PaperSummarizerChain::refreshStepList() {
    stepList_->clear();
    if (selectedChainId_ < 0) return;
    for (const auto& c : chains_) {
        if (c.id != selectedChainId_) continue;
        for (int i = 0; i < c.steps.size(); ++i) {
            const auto& s = c.steps[i];
            QString display = QString("%1. %2 [%3]").arg(i + 1).arg(s.name, s.prompt.left(30));
            auto* item = new QListWidgetItem(display);
            item->setData(Qt::UserRole, s.id);
            stepList_->addItem(item);
        }
        break;
    }
}

void PaperSummarizerChain::updateStats() {
    statsLabel_->setText(QString("%1 chains").arg(chains_.size()));
}

void PaperSummarizerChain::loadSettings() {
    QSettings settings("PaperCrawler", "SummarizerChain");
    QByteArray data = settings.value("chains").toByteArray();
    if (data.isEmpty()) return;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        SummaryChain c;
        c.id = obj["id"].toInt();
        c.name = obj["name"].toString();
        c.finalOutput = obj["finalOutput"].toString();
        c.createdAt = obj["createdAt"].toInteger();
        QJsonArray steps = obj["steps"].toArray();
        for (const auto& si : steps) {
            QJsonObject so = si.toObject();
            SummaryStep s;
            s.id = so["id"].toInt();
            s.name = so["name"].toString();
            s.prompt = so["prompt"].toString();
            s.enabled = so["enabled"].toBool(true);
            c.steps.append(s);
            nextStepId_ = qMax(nextStepId_, s.id + 1);
        }
        chains_.append(c);
        nextChainId_ = qMax(nextChainId_, c.id + 1);
    }
    refreshChainList();
    updateStats();
}

void PaperSummarizerChain::saveSettings() {
    QJsonArray arr;
    for (const auto& c : chains_) {
        QJsonObject obj;
        obj["id"] = c.id;
        obj["name"] = c.name;
        obj["finalOutput"] = c.finalOutput;
        obj["createdAt"] = static_cast<qint64>(c.createdAt);
        QJsonArray steps;
        for (const auto& s : c.steps) {
            QJsonObject so;
            so["id"] = s.id;
            so["name"] = s.name;
            so["prompt"] = s.prompt;
            so["enabled"] = s.enabled;
            steps.append(so);
        }
        obj["steps"] = steps;
        arr.append(obj);
    }
    QSettings settings("PaperCrawler", "SummarizerChain");
    settings.setValue("chains", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}
