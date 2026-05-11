#include "workspace/PaperLabNotebook.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperLabNotebook::PaperLabNotebook(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "LabNotebook")
{
    setupUI();
    loadSettings();
}

void PaperLabNotebook::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    logBtn_ = new QPushButton("Log");
    logBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(logBtn_, &QPushButton::clicked, this, &PaperLabNotebook::onLog);
    toolbar->addWidget(logBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Experiment", "Analysis", "Simulation", "Measurement"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperLabNotebook::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter experiment name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Log experiments");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperLabNotebook::addEntry(const LabEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit experimentLogged(entry.id, entry.confidence);
    update();
}

QList<LabEntry> PaperLabNotebook::entries() const { return entries_; }

int PaperLabNotebook::successCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.success) c++;
    return c;
}

qreal PaperLabNotebook::avgConfidence() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.confidence;
    return sum / entries_.size();
}

QMap<QString, int> PaperLabNotebook::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperLabNotebook::onLog() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"experiment", "analysis", "simulation", "measurement"};
    QStringList results = {"positive", "negative", "inconclusive", "pending"};
    QStringList statuses = {"complete", "running", "failed", "queued"};
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        LabEntry e;
        e.id = entries_.size() + 1;
        e.experiment = text.left(8) + " exp" + QString::number(i);
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.result = results[QRandomGenerator::global()->bounded(results.size())];
        e.status = statuses[QRandomGenerator::global()->bounded(statuses.size())];
        e.confidence = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
        e.date = "2026-05-" + QString::number(1 + QRandomGenerator::global()->bounded(28));
        e.success = e.result == "positive" && e.confidence >= 0.7;
        e.color = e.success ? QColor(16,185,129) : (e.status == "failed" ? QColor(239,68,68) : QColor(59,130,246));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperLabNotebook::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Log experiments");
    update();
}

void PaperLabNotebook::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Log experiments");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Lab Notebook");
    int w = width(), h = height();
    drawLabList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperLabNotebook::drawLabList(QPainter& p, const QRect& rect) {
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
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.experiment.left(14) + (e.success ? " [OK]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.result + " | " + e.status + " | " + e.date);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.confidence * 100, 'f', 0) + "%");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.category);
    }
}

void PaperLabNotebook::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"experiment", "analysis", "simulation", "measurement"};
    QString labels[] = {"Exper", "Analysis", "Sim", "Measure"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 65, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 70, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 73 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperLabNotebook::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Experiments", QString::number(entries_.size()), QColor(59,130,246)},
        {"Success", QString::number(successCount()), QColor(16,185,129)},
        {"Avg Conf", QString::number(avgConfidence() * 100, 'f', 0) + "%", QColor(245,158,11)},
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

void PaperLabNotebook::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Log experiments"); return; }
    infoLabel_->setText(QString("%1 exps | %2 ok | %3% conf")
        .arg(entries_.size()).arg(successCount()).arg(avgConfidence() * 100, 0, 'f', 0));
}

void PaperLabNotebook::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        LabEntry e;
        e.id = settings_.value("id").toInt();
        e.experiment = settings_.value("experiment").toString();
        e.category = settings_.value("category").toString();
        e.result = settings_.value("result").toString();
        e.status = settings_.value("status").toString();
        e.confidence = settings_.value("confidence").toDouble();
        e.date = settings_.value("date").toString();
        e.success = settings_.value("success").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperLabNotebook::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("experiment", entries_[i].experiment);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("result", entries_[i].result);
        settings_.setValue("status", entries_[i].status);
        settings_.setValue("confidence", entries_[i].confidence);
        settings_.setValue("date", entries_[i].date);
        settings_.setValue("success", entries_[i].success);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
