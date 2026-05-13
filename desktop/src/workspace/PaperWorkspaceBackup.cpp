#include "workspace/PaperWorkspaceBackup.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperWorkspaceBackup::PaperWorkspaceBackup(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "WorkspaceBackup")
{
    setupUI();
    loadSettings();

    if (entries_.isEmpty()) {
        QStringList categories = {"Full", "Incremental", "Differential", "Snapshot", "Archive"};
        QStringList storages = {"Local", "Cloud", "Network", "USB", "NAS"};
        QColor colors[] = {
            QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
            QColor("#dc2626"), QColor("#7c3aed")
        };
        for (int i = 0; i < 8; ++i) {
            WorkspaceBackupEntry e;
            e.id = i + 1;
            e.backup = QStringLiteral("backup_%1_%2")
                .arg(categories[i % 5].toLower())
                .arg(QString::number(i + 1).rightJustified(3, '0'));
            e.category = categories[i % 5];
            e.storage = storages[i % 5];
            e.size = 0.5 + QRandomGenerator::global()->bounded(5000) / 100.0;
            e.files = 50 + QRandomGenerator::global()->bounded(2000);
            e.encrypted = QRandomGenerator::global()->bounded(2) == 0;
            e.color = colors[i % 5];
            entries_.append(e);
        }
        saveSettings();
        updateInfo();
        update();
    }
}

void PaperWorkspaceBackup::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    backupBtn_ = new QPushButton("Backup");
    backupBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(backupBtn_, &QPushButton::clicked, this, &PaperWorkspaceBackup::onBackup);
    toolbar->addWidget(backupBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Full", "Incremental", "Differential", "Snapshot", "Archive"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperWorkspaceBackup::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter backup name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Manage workspace backups");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperWorkspaceBackup::addEntry(const WorkspaceBackupEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit backupCreated(entry.id, entry.size);
    update();
}

QList<WorkspaceBackupEntry> PaperWorkspaceBackup::entries() const {
    return entries_;
}

int PaperWorkspaceBackup::encryptedCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.encrypted) c++;
    return c;
}

qreal PaperWorkspaceBackup::totalSize() const {
    qreal sum = 0;
    for (const auto& e : entries_)
        sum += e.size;
    return sum;
}

QMap<QString, int> PaperWorkspaceBackup::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperWorkspaceBackup::onBackup() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"Full", "Incremental", "Differential", "Snapshot", "Archive"};
    QStringList storages = {"Local", "Cloud", "Network", "USB", "NAS"};
    QColor colors[] = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };
    int cIdx = categoryCombo_->currentIndex();
    int count = 2 + QRandomGenerator::global()->bounded(3);
    for (int i = 0; i < count; ++i) {
        WorkspaceBackupEntry e;
        e.id = entries_.size() + 1;
        e.backup = text.left(12) + " bk" + QString::number(i);
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())]
                                : categories[cIdx - 1];
        e.storage = storages[QRandomGenerator::global()->bounded(storages.size())];
        e.size = 0.5 + QRandomGenerator::global()->bounded(5000) / 100.0;
        e.files = 50 + QRandomGenerator::global()->bounded(2000);
        e.encrypted = QRandomGenerator::global()->bounded(2) == 0;
        int catIdx = categories.indexOf(e.category);
        e.color = catIdx >= 0 ? colors[catIdx] : colors[0];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperWorkspaceBackup::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Manage workspace backups");
    update();
}

void PaperWorkspaceBackup::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Manage workspace backups");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Workspace Backups");
    int w = width(), h = height();
    drawBackupView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperWorkspaceBackup::drawBackupView(QPainter& p, const QRect& rect) {
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
                   e.backup.left(14) + (e.encrypted ? " [E]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + e.storage + " | " + QString::number(e.files) + " files");
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.size, 'f', 1) + " MB");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.encrypted ? "Encrypted" : "Plain");
    }
}

void PaperWorkspaceBackup::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"Full", "Incremental", "Differential", "Snapshot", "Archive"};
    QString labels[] = {"Full", "Incr", "Diff", "Snap", "Archive"};
    QColor colors[] = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
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

void PaperWorkspaceBackup::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Backups", QString::number(entries_.size()), QColor("#3b82f6")},
        {"Encrypted", QString::number(encryptedCount()), QColor("#16a34a")},
        {"Total Size", QString::number(totalSize(), 'f', 1) + " MB", QColor("#d97706")},
        {"Categories", QString::number(categoryCounts().size()), QColor("#7c3aed")}
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

void PaperWorkspaceBackup::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Manage workspace backups");
        return;
    }
    infoLabel_->setText(QString("%1 backups | %2 encrypted | %3 MB total")
        .arg(entries_.size())
        .arg(encryptedCount())
        .arg(totalSize(), 0, 'f', 1));
}

void PaperWorkspaceBackup::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        WorkspaceBackupEntry e;
        e.id = settings_.value("id").toInt();
        e.backup = settings_.value("backup").toString();
        e.category = settings_.value("category").toString();
        e.storage = settings_.value("storage").toString();
        e.size = settings_.value("size").toDouble();
        e.files = settings_.value("files").toInt();
        e.encrypted = settings_.value("encrypted").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperWorkspaceBackup::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("backup", entries_[i].backup);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("storage", entries_[i].storage);
        settings_.setValue("size", entries_[i].size);
        settings_.setValue("files", entries_[i].files);
        settings_.setValue("encrypted", entries_[i].encrypted);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
