#include "tools/PaperScriptRunner.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperScriptRunner::PaperScriptRunner(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ScriptRunner")
{
    setupUI();
    loadSettings();
}

void PaperScriptRunner::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    runBtn_ = new QPushButton("Run");
    runBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(runBtn_, &QPushButton::clicked, this, &PaperScriptRunner::onRun);
    toolbar->addWidget(runBtn_);
    toolbar->addWidget(new QLabel("Language:"));
    langCombo_ = new QComboBox();
    langCombo_->addItems({"All", "Python", "R", "Julia", "Bash"});
    toolbar->addWidget(langCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperScriptRunner::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter script name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Run analysis scripts");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperScriptRunner::addEntry(const ScriptEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit scriptExecuted(entry.id, entry.runtime);
    update();
}

QList<ScriptEntry> PaperScriptRunner::entries() const { return entries_; }

int PaperScriptRunner::successCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.success) c++;
    return c;
}

qreal PaperScriptRunner::avgRuntime() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.runtime;
    return sum / entries_.size();
}

QMap<QString, int> PaperScriptRunner::languageCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.language]++;
    return counts;
}

void PaperScriptRunner::onRun() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList languages = {"python", "r", "julia", "bash"};
    QStringList statuses = {"completed", "completed", "completed", "error", "timeout"};
    QStringList outputs = {"42 rows processed", "model fitted", "plot saved", "export done", "stats computed"};
    int lIdx = langCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        ScriptEntry e;
        e.id = entries_.size() + 1;
        e.scriptName = text.left(10) + " script" + QString::number(i);
        e.language = lIdx == 0 ? languages[QRandomGenerator::global()->bounded(languages.size())] : languages[lIdx - 1];
        e.status = statuses[QRandomGenerator::global()->bounded(statuses.size())];
        e.runtime = 100 + QRandomGenerator::global()->bounded(30000);
        e.memory = 10 + QRandomGenerator::global()->bounded(500);
        e.output = outputs[QRandomGenerator::global()->bounded(outputs.size())];
        e.success = e.status == "completed";
        e.color = e.success ? QColor(16,185,129) : (e.status == "timeout" ? QColor(245,158,11) : QColor(239,68,68));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperScriptRunner::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Run analysis scripts");
    update();
}

void PaperScriptRunner::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Run analysis scripts");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Script Runner");
    int w = width(), h = height();
    drawScriptList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawLanguageChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperScriptRunner::drawScriptList(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(34, (rect.height() - 10) / qMax(show, 1));
    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 3);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Courier", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.scriptName.left(16) + (e.success ? " [OK]" : " [ERR]"));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.language + " | " + e.status + " | " + e.output.left(14));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.runtime) + "ms");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.memory, 'f', 0) + "MB");
    }
}

void PaperScriptRunner::drawLanguageChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Languages");
    auto counts = languageCounts();
    QStringList languages = {"python", "r", "julia", "bash"};
    QString labels[] = {"Python", "R", "Julia", "Bash"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(139,92,246), QColor(245,158,11)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(languages[i]) ? counts[languages[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 80));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 30, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 35, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 38 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperScriptRunner::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Scripts", QString::number(entries_.size()), QColor(59,130,246)},
        {"Success", QString::number(successCount()), QColor(16,185,129)},
        {"Avg Time", QString::number(avgRuntime(), 'f', 0) + "ms", QColor(245,158,11)},
        {"Languages", QString::number(languageCounts().size()), QColor(139,92,246)}
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

void PaperScriptRunner::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Run analysis scripts"); return; }
    infoLabel_->setText(QString("%1 scripts | %2 ok | %3ms avg")
        .arg(entries_.size()).arg(successCount()).arg(avgRuntime(), 0, 'f', 0));
}

void PaperScriptRunner::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ScriptEntry e;
        e.id = settings_.value("id").toInt();
        e.scriptName = settings_.value("scriptName").toString();
        e.language = settings_.value("language").toString();
        e.status = settings_.value("status").toString();
        e.runtime = settings_.value("runtime").toInt();
        e.memory = settings_.value("memory").toDouble();
        e.output = settings_.value("output").toString();
        e.success = settings_.value("success").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperScriptRunner::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("scriptName", entries_[i].scriptName);
        settings_.setValue("language", entries_[i].language);
        settings_.setValue("status", entries_[i].status);
        settings_.setValue("runtime", entries_[i].runtime);
        settings_.setValue("memory", entries_[i].memory);
        settings_.setValue("output", entries_[i].output);
        settings_.setValue("success", entries_[i].success);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
