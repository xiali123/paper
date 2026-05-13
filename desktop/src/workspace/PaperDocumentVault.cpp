#include "workspace/PaperDocumentVault.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperDocumentVault::PaperDocumentVault(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "DocumentVault")
{
    setupUI();
    loadSettings();
}

void PaperDocumentVault::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    storeBtn_ = new QPushButton("Store");
    storeBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(storeBtn_, &QPushButton::clicked, this, &PaperDocumentVault::onStore);
    toolbar->addWidget(storeBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Paper", "Dataset", "Code", "Figure", "Supplement"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperDocumentVault::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter document name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Store documents in vault");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperDocumentVault::addEntry(const DocumentVaultEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit documentStored(entry.id, entry.size);
    update();
}

QList<DocumentVaultEntry> PaperDocumentVault::entries() const { return entries_; }

int PaperDocumentVault::lockedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.locked) c++;
    return c;
}

qreal PaperDocumentVault::totalSize() const {
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.size;
    return sum;
}

QMap<QString, int> PaperDocumentVault::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperDocumentVault::onStore() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"Paper", "Dataset", "Code", "Figure", "Supplement"};
    QStringList storages = {"Local", "Cloud", "Hybrid", "Archive"};
    QList<QColor> palette = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(6);
    for (int i = 0; i < count; ++i) {
        DocumentVaultEntry e;
        e.id = entries_.size() + 1;
        e.document = text.left(10) + " doc" + QString::number(i);
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())]
                                : categories[cIdx - 1];
        e.storage = storages[QRandomGenerator::global()->bounded(storages.size())];
        e.size = 0.1 + QRandomGenerator::global()->bounded(500) / 10.0;
        e.versions = 1 + QRandomGenerator::global()->bounded(10);
        e.locked = QRandomGenerator::global()->bounded(4) == 0;
        int catIdx = categories.indexOf(e.category);
        e.color = palette[qMax(0, catIdx)];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperDocumentVault::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Store documents in vault");
    update();
}

void PaperDocumentVault::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Store documents in vault");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Document Vault");
    int w = width(), h = height();
    drawVaultView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperDocumentVault::drawVaultView(QPainter& p, const QRect& rect) {
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
                   e.document.left(14) + (e.locked ? " [L]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + e.storage + " | v" + QString::number(e.versions));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.size, 'f', 1) + " MB");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.locked ? "Locked" : "Open");
    }
}

void PaperDocumentVault::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"Paper", "Dataset", "Code", "Figure", "Supplement"};
    QString labels[] = {"Paper", "Dataset", "Code", "Figure", "Suppl."};
    QList<QColor> palette = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(22, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 120));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 65, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(palette[i]);
        p.drawRoundedRect(rect.x() + 70, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 73 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperDocumentVault::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Documents", QString::number(entries_.size()), QColor("#3b82f6")},
        {"Locked", QString::number(lockedCount()), QColor("#dc2626")},
        {"Total Size", QString::number(totalSize(), 'f', 1) + " MB", QColor("#16a34a")},
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

void PaperDocumentVault::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Store documents in vault"); return; }
    infoLabel_->setText(QString("%1 docs | %2 locked | %3 MB")
        .arg(entries_.size()).arg(lockedCount()).arg(totalSize(), 0, 'f', 1));
}

void PaperDocumentVault::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        DocumentVaultEntry e;
        e.id = settings_.value("id").toInt();
        e.document = settings_.value("document").toString();
        e.category = settings_.value("category").toString();
        e.storage = settings_.value("storage").toString();
        e.size = settings_.value("size").toDouble();
        e.versions = settings_.value("versions").toInt();
        e.locked = settings_.value("locked").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    if (entries_.isEmpty()) {
        QStringList categories = {"Paper", "Dataset", "Code", "Figure", "Supplement"};
        QStringList storages = {"Local", "Cloud", "Hybrid", "Archive"};
        QStringList names = {"thesis_draft", "survey_data", "analysis_script",
                             "result_plot", "appendix", "experiment_log",
                             "training_set", "model_weights"};
        QList<QColor> palette = {
            QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
            QColor("#dc2626"), QColor("#7c3aed")
        };
        for (int i = 0; i < 8; ++i) {
            DocumentVaultEntry e;
            e.id = i + 1;
            e.document = names[i];
            e.category = categories[i % 5];
            e.storage = storages[i % 4];
            e.size = 0.5 + i * 3.7;
            e.versions = 1 + i;
            e.locked = (i == 0 || i == 4);
            e.color = palette[i % 5];
            entries_.append(e);
        }
        saveSettings();
    }
    updateInfo();
}

void PaperDocumentVault::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("document", entries_[i].document);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("storage", entries_[i].storage);
        settings_.setValue("size", entries_[i].size);
        settings_.setValue("versions", entries_[i].versions);
        settings_.setValue("locked", entries_[i].locked);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
