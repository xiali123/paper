#include "tools/PaperFormulaEditor.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperFormulaEditor::PaperFormulaEditor(QWidget* parent)
    : QWidget(parent) {
    setupUI();
    loadSettings();
}

void PaperFormulaEditor::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Statistics", "Physics", "ML", "Economics", "Biology"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Formula name...");
    saveBtn_ = new QPushButton("Save", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Formulas: 0 | Verified: 0 | Avg Accuracy: 0.00", this);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(saveBtn_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);
    connect(saveBtn_, &QPushButton::clicked, this, &PaperFormulaEditor::onSave);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperFormulaEditor::onClear);
}

void PaperFormulaEditor::addEntry(const FormulaEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<FormulaEntry> PaperFormulaEditor::entries() const { return entries_; }

int PaperFormulaEditor::verifiedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.verified) c++;
    return c;
}

qreal PaperFormulaEditor::avgAccuracy() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.accuracy;
    return sum / entries_.size();
}

QMap<QString, int> PaperFormulaEditor::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperFormulaEditor::onSave() {
    FormulaEntry e;
    e.id = entries_.size() + 1;
    e.name = inputField_->text().trimmed();
    if (e.name.isEmpty()) e.name = QString("Formula_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    QStringList expressions = {"f(x) = ax^2 + bx + c", "P(A|B) = P(B|A)*P(A)/P(B)", "E = mc^2", "y = mx + b", "sigma = sqrt(var)"};
    e.expression = expressions[QRandomGenerator::global()->bounded(expressions.size())];
    e.result = QRandomGenerator::global()->bounded(-100.0, 100.0);
    e.accuracy = QRandomGenerator::global()->bounded(0.0, 1.0);
    e.variables = QRandomGenerator::global()->bounded(1, 10);
    e.verified = e.accuracy > 0.9;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    updateInfo();
    saveSettings();
    emit formulaSaved(e.id, e.accuracy);
    update();
}

void PaperFormulaEditor::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperFormulaEditor::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawFormulaList(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryChart(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperFormulaEditor::drawFormulaList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Formula Editor:");
    int y = rect.top() + 20;
    for (int i = 0; i < qMin(entries_.size(), 8); ++i) {
        const auto& e = entries_[i];
        p.setPen(e.color);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.left(), y, 8, 8, 2, 2);
        p.setPen(QColor(0x334155));
        QString text = QString("%1 | %2 | %3 vars | Acc: %4 | %5")
            .arg(e.name, e.category)
            .arg(e.variables)
            .arg(QString::number(e.accuracy, 'f', 2))
            .arg(e.verified ? "Verified" : "Draft");
        p.drawText(rect.left() + 14, y + 9, text);
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperFormulaEditor::drawCategoryChart(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts();
    int y = rect.top() + 5;
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Field:");
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

void PaperFormulaEditor::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Verified: %1").arg(verifiedCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Avg Accuracy: %1").arg(QString::number(avgAccuracy(), 'f', 3)));
}

void PaperFormulaEditor::updateInfo() {
    infoLabel_->setText(QString("Formulas: %1 | Verified: %2 | Avg Accuracy: %3")
        .arg(entries_.size()).arg(verifiedCount())
        .arg(QString::number(avgAccuracy(), 'f', 2)));
}

void PaperFormulaEditor::loadSettings() {
    settings_.beginGroup("FormulaEditor");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        FormulaEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.name = settings_.value(QString("name_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.expression = settings_.value(QString("expression_%1").arg(i)).toString();
        e.result = settings_.value(QString("result_%1").arg(i)).toDouble();
        e.accuracy = settings_.value(QString("accuracy_%1").arg(i)).toDouble();
        e.variables = settings_.value(QString("variables_%1").arg(i)).toInt();
        e.verified = settings_.value(QString("verified_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperFormulaEditor::saveSettings() {
    settings_.beginGroup("FormulaEditor");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("name_%1").arg(i), e.name);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("expression_%1").arg(i), e.expression);
        settings_.setValue(QString("result_%1").arg(i), e.result);
        settings_.setValue(QString("accuracy_%1").arg(i), e.accuracy);
        settings_.setValue(QString("variables_%1").arg(i), e.variables);
        settings_.setValue(QString("verified_%1").arg(i), e.verified);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
