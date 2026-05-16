#include "workspace/PaperDatasetManager.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

PaperDatasetManager::PaperDatasetManager(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "DatasetManager")
{
    setupUI();
    loadSettings();
}

void PaperDatasetManager::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Dataset");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperDatasetManager::onAdd);
    toolbar->addWidget(addBtn_);

    syncBtn_ = new QPushButton("Sync");
    connect(syncBtn_, &QPushButton::clicked, this, &PaperDatasetManager::onSync);
    toolbar->addWidget(syncBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperDatasetManager::onClear);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Manage research datasets");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperDatasetManager::addDataset(const DatasetEntry& dataset) {
    datasets_.append(dataset);
    saveSettings();
    updateInfo();
    emit datasetAdded(dataset.id);
    update();
}

QList<DatasetEntry> PaperDatasetManager::datasets() const { return datasets_; }

QMap<QString, int> PaperDatasetManager::formatCounts() const {
    QMap<QString, int> counts;
    for (const auto& d : datasets_) counts[d.format]++;
    return counts;
}

QMap<QString, int> PaperDatasetManager::statusCounts() const {
    QMap<QString, int> counts;
    for (const auto& d : datasets_) counts[d.status]++;
    return counts;
}

qreal PaperDatasetManager::totalSize() const {
    qreal t = 0;
    for (const auto& d : datasets_) t += d.sizeMB;
    return t;
}

void PaperDatasetManager::onAdd() {
    bool ok;
    QString name = QInputDialog::getText(this, "Add Dataset", "Name:", QLineEdit::Normal, "", &ok);
    if (!ok || name.isEmpty()) return;
    QStringList formats = {"csv", "json", "xml", "sql"};
    QString format = QInputDialog::getItem(this, "Add Dataset", "Format:", formats, 0, false, &ok);
    if (!ok) return;
    QStringList statuses = {"local", "remote", "synced", "outdated"};
    QString status = QInputDialog::getItem(this, "Add Dataset", "Status:", statuses, 0, false, &ok);
    if (!ok) return;

    DatasetEntry d;
    d.id = datasets_.size() + 1;
    d.name = name;
    d.format = format;
    d.source = "Local";
    d.status = status;
    d.records = 100 + QRandomGenerator::global()->bounded(9900);
    d.sizeMB = 1.0 + QRandomGenerator::global()->bounded(5000) / 10.0;
    d.lastUpdated = QDate::currentDate().addDays(-QRandomGenerator::global()->bounded(30));
    d.description = name;

    QColor statusColors[] = {QColor(16,185,129), QColor(59,130,246), QColor(139,92,246), QColor(245,158,11)};
    int sIdx = statuses.indexOf(status);
    d.color = statusColors[qBound(0, sIdx, 3)];
    addDataset(d);
}

void PaperDatasetManager::onSync() {
    for (auto& d : datasets_) {
        if (d.status != "synced") {
            d.status = "synced";
            d.lastUpdated = QDate::currentDate();
            d.color = QColor(139, 92, 246);
        }
    }
    saveSettings();
    updateInfo();
    emit syncCompleted(datasets_.size());
    update();
}

void PaperDatasetManager::onClear() {
    datasets_.clear();
    saveSettings();
    infoLabel_->setText("Manage research datasets");
    update();
}

void PaperDatasetManager::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (datasets_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Manage research datasets");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Dataset Manager");

    int w = width(), h = height();
    drawDatasetList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawFormatChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperDatasetManager::drawDatasetList(QPainter& p, const QRect& rect) {
    int show = qMin(8, datasets_.size());
    int itemH = qMin(46, (rect.height() - 10) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& d = datasets_[i];
        int y = rect.y() + i * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(d.color.lighter(180));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(d.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   d.name.left(18));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   d.format.toUpper() + " | " + QString::number(d.records) + " records");
        p.drawText(rect.x() + 10, y + 34, rect.width() / 2 - 10, 12, Qt::AlignVCenter,
                   d.lastUpdated.toString("MM/dd"));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight, d.status);

        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(d.sizeMB, 'f', 1) + " MB");
    }
}

void PaperDatasetManager::drawFormatChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "By Format");

    auto counts = formatCounts();
    QList<QString> fmts = counts.keys();
    if (fmts.isEmpty()) return;

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(24, (rect.height() - 30) / fmts.size());
    for (int i = 0; i < fmts.size(); ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int barW = static_cast<int>((static_cast<qreal>(counts[fmts[i]]) / maxVal) * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 65, barH, Qt::AlignRight | Qt::AlignVCenter, fmts[i].toUpper());

        QColor c(59 + (i * 47) % 180, 130 + (i * 31) % 120, 246 - (i * 19) % 100);
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawRoundedRect(rect.x() + 70, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 73 + barW, y + barH - 2, QString::number(counts[fmts[i]]));
    }
}

void PaperDatasetManager::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Datasets", QString::number(datasets_.size()), QColor(59,130,246)},
        {"Total Size", QString::number(totalSize(), 'f', 1) + " MB", QColor(16,185,129)},
        {"Synced", QString::number(statusCounts().value("synced", 0)), QColor(139,92,246)},
        {"Formats", QString::number(formatCounts().size()), QColor(245,158,11)}
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

void PaperDatasetManager::updateInfo() {
    if (datasets_.isEmpty()) { infoLabel_->setText("Manage research datasets"); return; }
    infoLabel_->setText(QString("%1 datasets | %2 MB | %3 synced")
        .arg(datasets_.size()).arg(totalSize(), 0, 'f', 1).arg(statusCounts().value("synced", 0)));
}

void PaperDatasetManager::loadSettings() {
    int size = settings_.beginReadArray("datasets");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        DatasetEntry d;
        d.id = settings_.value("id").toInt();
        d.name = settings_.value("name").toString();
        d.format = settings_.value("format").toString();
        d.source = settings_.value("source").toString();
        d.status = settings_.value("status").toString();
        d.records = settings_.value("records").toInt();
        d.sizeMB = settings_.value("sizeMB").toDouble();
        d.lastUpdated = QDate::fromString(settings_.value("lastUpdated").toString(), Qt::ISODate);
        d.description = settings_.value("description").toString();
        d.color = QColor(settings_.value("color").toString());
        datasets_.append(d);
    }
    settings_.endArray();
    updateInfo();
}

void PaperDatasetManager::saveSettings() {
    settings_.beginWriteArray("datasets");
    for (int i = 0; i < datasets_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", datasets_[i].id);
        settings_.setValue("name", datasets_[i].name);
        settings_.setValue("format", datasets_[i].format);
        settings_.setValue("source", datasets_[i].source);
        settings_.setValue("status", datasets_[i].status);
        settings_.setValue("records", datasets_[i].records);
        settings_.setValue("sizeMB", datasets_[i].sizeMB);
        settings_.setValue("lastUpdated", datasets_[i].lastUpdated.toString(Qt::ISODate));
        settings_.setValue("description", datasets_[i].description);
        settings_.setValue("color", datasets_[i].color.name());
    }
    settings_.endArray();
}
