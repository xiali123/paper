#include "workspace/PaperSurveyManager.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperSurveyManager::PaperSurveyManager(QWidget* parent)
    : QWidget(parent) {
    setupUI();
    loadSettings();
}

void PaperSurveyManager::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Literature", "Methodology", "Tools", "Trends", "Feedback"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Survey name...");
    createBtn_ = new QPushButton("Create", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Surveys: 0 | Active: 0 | Avg Completion: 0%", this);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(createBtn_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);
    connect(createBtn_, &QPushButton::clicked, this, &PaperSurveyManager::onCreate);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperSurveyManager::onClear);
}

void PaperSurveyManager::addEntry(const SurveyEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<SurveyEntry> PaperSurveyManager::entries() const { return entries_; }

int PaperSurveyManager::activeCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.active) c++;
    return c;
}

qreal PaperSurveyManager::avgCompletion() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.completion;
    return sum / entries_.size();
}

QMap<QString, int> PaperSurveyManager::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperSurveyManager::onCreate() {
    SurveyEntry e;
    e.id = entries_.size() + 1;
    e.survey = inputField_->text().trimmed();
    if (e.survey.isEmpty()) e.survey = QString("Survey_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    QStringList statuses = {"Draft", "Active", "Paused", "Completed", "Archived"};
    e.status = statuses[QRandomGenerator::global()->bounded(statuses.size())];
    e.responses = QRandomGenerator::global()->bounded(0, 500);
    e.completion = QRandomGenerator::global()->bounded(0.0, 1.0);
    e.deadline = QString("2026-%1-%2")
        .arg(QRandomGenerator::global()->bounded(1, 13), 2, 10, QChar('0'))
        .arg(QRandomGenerator::global()->bounded(1, 29), 2, 10, QChar('0'));
    e.active = e.status == "Active";
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    updateInfo();
    saveSettings();
    emit surveyCreated(e.id, e.completion);
    update();
}

void PaperSurveyManager::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperSurveyManager::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawSurveyList(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryChart(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperSurveyManager::drawSurveyList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Survey Manager:");
    int y = rect.top() + 20;
    for (int i = 0; i < qMin(entries_.size(), 8); ++i) {
        const auto& e = entries_[i];
        p.setPen(e.color);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.left(), y, 8, 8, 2, 2);
        p.setPen(QColor(0x334155));
        QString text = QString("%1 | %2 | %3 | %4 resp | %5% | Due: %6")
            .arg(e.survey.left(15), e.category, e.status)
            .arg(e.responses)
            .arg(static_cast<int>(e.completion * 100))
            .arg(e.deadline);
        p.drawText(rect.left() + 14, y + 9, text);
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperSurveyManager::drawCategoryChart(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts();
    int y = rect.top() + 5;
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Topic:");
    y += 18;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    int ci = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        p.setBrush(colors[ci++ % colors.size()]);
        p.drawRoundedRect(rect.left(), y, qMin(it.value() * 20, rect.width()), 14, 3, 3);
        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + 4, y + 12, QString("%1: %2").arg(it.key()).arg(it.value()));
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperSurveyManager::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Active: %1").arg(activeCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Avg Completion: %1%").arg(static_cast<int>(avgCompletion() * 100)));
}

void PaperSurveyManager::updateInfo() {
    infoLabel_->setText(QString("Surveys: %1 | Active: %2 | Avg Completion: %3%")
        .arg(entries_.size()).arg(activeCount())
        .arg(static_cast<int>(avgCompletion() * 100)));
}

void PaperSurveyManager::loadSettings() {
    settings_.beginGroup("SurveyManager");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        SurveyEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.survey = settings_.value(QString("survey_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.status = settings_.value(QString("status_%1").arg(i)).toString();
        e.responses = settings_.value(QString("responses_%1").arg(i)).toInt();
        e.completion = settings_.value(QString("completion_%1").arg(i)).toDouble();
        e.deadline = settings_.value(QString("deadline_%1").arg(i)).toString();
        e.active = settings_.value(QString("active_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperSurveyManager::saveSettings() {
    settings_.beginGroup("SurveyManager");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("survey_%1").arg(i), e.survey);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("status_%1").arg(i), e.status);
        settings_.setValue(QString("responses_%1").arg(i), e.responses);
        settings_.setValue(QString("completion_%1").arg(i), e.completion);
        settings_.setValue(QString("deadline_%1").arg(i), e.deadline);
        settings_.setValue(QString("active_%1").arg(i), e.active);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
