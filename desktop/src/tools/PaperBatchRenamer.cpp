#include "tools/PaperBatchRenamer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

PaperBatchRenamer::PaperBatchRenamer(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "BatchRenamer")
{
    setupUI();
    loadSettings();
}

void PaperBatchRenamer::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    renameBtn_ = new QPushButton("Rename");
    renameBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renameBtn_, &QPushButton::clicked, this, &PaperBatchRenamer::onRename);
    toolbar->addWidget(renameBtn_);

    toolbar->addWidget(new QLabel("Strategy:"));
    strategyCombo_ = new QComboBox();
    strategyCombo_->addItems({"Author-Year", "Title-Hash", "DOI-Based", "Sequential"});
    toolbar->addWidget(strategyCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperBatchRenamer::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    patternField_ = new QLineEdit();
    patternField_->setPlaceholderText("Enter filename pattern (e.g., {author}_{year}_{title})...");
    patternField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(patternField_);

    infoLabel_ = new QLabel("Batch rename paper files");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperBatchRenamer::addEntry(const RenameEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit renameComplete(entry.id, entry.filesAffected);
    update();
}

QList<RenameEntry> PaperBatchRenamer::entries() const { return entries_; }

QMap<QString, int> PaperBatchRenamer::statusCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.status]++;
    return counts;
}

int PaperBatchRenamer::totalFiles() const {
    int t = 0;
    for (const auto& e : entries_) t += e.filesAffected;
    return t;
}

int PaperBatchRenamer::renamedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.status == "done") c += e.filesAffected;
    return c;
}

void PaperBatchRenamer::onRename() {
    QString pattern = patternField_->text().trimmed();
    if (pattern.isEmpty()) pattern = "{author}_{year}";

    QStringList strategies = {"author-year", "title-hash", "doi-based", "sequential"};
    QStringList prefixes = {"Smith_2024_", "Jones_2023_", "Lee_2025_", "Wang_2024_"};

    int strategyIdx = strategyCombo_->currentIndex();

    RenameEntry e;
    e.id = entries_.size() + 1;
    e.pattern = pattern;
    e.category = strategies[strategyIdx];
    int fileCount = 3 + QRandomGenerator::global()->bounded(15);
    e.filesAffected = fileCount;
    e.originalName = "paper_" + QString::number(e.id) + "_raw.pdf";
    e.newName = prefixes[strategyIdx] + "batch_" + QString::number(e.id) + ".pdf";
    e.status = "done";
    e.preview = e.originalName.left(12) + " -> " + e.newName.left(14);
    e.color = QColor(16,185,129);
    addEntry(e);
}

void PaperBatchRenamer::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Batch rename paper files");
    update();
}

void PaperBatchRenamer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Batch rename paper files");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Batch Renamer");

    int w = width(), h = height();
    drawRenameList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawStatusChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperBatchRenamer::drawRenameList(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(36, (rect.height() - 10) / qMax(show, 1));

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
                   e.pattern.left(18));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + e.status);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.filesAffected) + " files");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.originalName.left(16));
    }
}

void PaperBatchRenamer::drawStatusChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Operations");

    auto counts = statusCounts();
    QStringList statuses = {"done", "pending", "failed"};
    QString labels[] = {"Done", "Pending", "Failed"};
    QColor colors[] = {QColor(16,185,129), QColor(245,158,11), QColor(239,68,68)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(28, (rect.height() - 30) / 3);
    for (int i = 0; i < 3; ++i) {
        int y = rect.y() + 22 + i * (barH + 4);
        int count = counts.contains(statuses[i]) ? counts[statuses[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x(), y + barH - 3, 60, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 65, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 68 + barW, y + barH - 3, QString::number(count));
    }
}

void PaperBatchRenamer::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Batches", QString::number(entries_.size()), QColor(59,130,246)},
        {"Total Files", QString::number(totalFiles()), QColor(16,185,129)},
        {"Renamed", QString::number(renamedCount()), QColor(245,158,11)},
        {"Failed", QString::number(statusCounts().value("failed", 0)), QColor(239,68,68)}
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

void PaperBatchRenamer::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Batch rename paper files"); return; }
    infoLabel_->setText(QString("%1 batches | %2 files | %3 renamed")
        .arg(entries_.size()).arg(totalFiles()).arg(renamedCount()));
}

void PaperBatchRenamer::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        RenameEntry e;
        e.id = settings_.value("id").toInt();
        e.originalName = settings_.value("originalName").toString();
        e.newName = settings_.value("newName").toString();
        e.pattern = settings_.value("pattern").toString();
        e.category = settings_.value("category").toString();
        e.filesAffected = settings_.value("filesAffected").toInt();
        e.status = settings_.value("status").toString();
        e.preview = settings_.value("preview").toString();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperBatchRenamer::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("originalName", entries_[i].originalName);
        settings_.setValue("newName", entries_[i].newName);
        settings_.setValue("pattern", entries_[i].pattern);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("filesAffected", entries_[i].filesAffected);
        settings_.setValue("status", entries_[i].status);
        settings_.setValue("preview", entries_[i].preview);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
