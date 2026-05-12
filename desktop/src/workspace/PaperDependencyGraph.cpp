#include "workspace/PaperDependencyGraph.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPaintEvent>

PaperDependencyGraph::PaperDependencyGraph(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "DependencyGraph")
{
    setupUI();
    loadSettings();
}

void PaperDependencyGraph::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperDependencyGraph::onAdd);
    toolbar->addWidget(addBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Core", "Plugin", "UI", "Network", "Data"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperDependencyGraph::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter module->depends (e.g. Parser->Tokenizer)...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Add dependencies");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperDependencyGraph::addEntry(const DependencyEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit dependencyAdded(entry.id, entry.criticality);
    update();
}

QList<DependencyEntry> PaperDependencyGraph::entries() const { return entries_; }

int PaperDependencyGraph::cyclicCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.cyclic) c++;
    return c;
}

qreal PaperDependencyGraph::avgCriticality() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.criticality;
    return sum / entries_.size();
}

QMap<QString, int> PaperDependencyGraph::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperDependencyGraph::onAdd() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList parts = text.split("->");
    QString module = parts[0].trimmed();
    QString depends = parts.size() > 1 ? parts[1].trimmed() : "";

    if (module.isEmpty()) return;

    QStringList categories = {"Core", "Plugin", "UI", "Network", "Data"};
    QColor palette[] = {
        QColor(0x3b, 0x82, 0xf6),
        QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06),
        QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };

    int cIdx = categoryCombo_->currentIndex();
    int count = 2 + QRandomGenerator::global()->bounded(3);
    for (int i = 0; i < count; ++i) {
        DependencyEntry e;
        e.id = entries_.size() + 1;
        e.module = module + (count > 1 ? QString("_%1").arg(i) : "");
        e.category = cIdx == 0
            ? categories[QRandomGenerator::global()->bounded(categories.size())]
            : categories[cIdx - 1];
        e.depends = depends.isEmpty()
            ? categories[QRandomGenerator::global()->bounded(categories.size())].toLower()
            : depends;
        e.depth = 1 + QRandomGenerator::global()->bounded(6);
        e.criticality = 0.1 + QRandomGenerator::global()->bounded(90) / 100.0;
        e.version = QString("%1.%2.%3")
            .arg(QRandomGenerator::global()->bounded(4))
            .arg(QRandomGenerator::global()->bounded(10))
            .arg(QRandomGenerator::global()->bounded(20));

        // Detect cyclic: check if depends loops back to any existing module
        e.cyclic = false;
        if (!depends.isEmpty()) {
            for (const auto& existing : entries_) {
                if (existing.module == depends && existing.depends == module) {
                    e.cyclic = true;
                    break;
                }
            }
            // Also randomly mark some as cyclic for variety
            if (!e.cyclic && QRandomGenerator::global()->bounded(10) == 0) {
                e.cyclic = true;
            }
        }

        int colorIdx = categories.indexOf(e.category);
        if (colorIdx < 0) colorIdx = 0;
        e.color = palette[colorIdx];

        addEntry(e);
    }
    inputField_->clear();
}

void PaperDependencyGraph::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Add dependencies");
    update();
}

void PaperDependencyGraph::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Add dependencies");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Dependency Graph");

    int w = width(), h = height();
    drawDependencyList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperDependencyGraph::drawDependencyList(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(34, (rect.height() - 10) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 3);

        // Background pill
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        // Left color accent bar
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        // Module name and cyclic indicator
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        QString modLabel = e.module.left(14) + (e.cyclic ? " [CYC]" : "");
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter, modLabel);

        // Dependency and version line
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter, e.depends + " | v" + e.version + " | d" + QString::number(e.depth));

        // Criticality percentage
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.criticality * 100, 'f', 0) + "%");

        // Category label
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight, e.category);
    }
}

void PaperDependencyGraph::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList categories = {"Core", "Plugin", "UI", "Network", "Data"};
    QString labels[] = {"Core", "Plugin", "UI", "Network", "Data"};
    QColor colors[] = {
        QColor(0x3b, 0x82, 0xf6),
        QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06),
        QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
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

void PaperDependencyGraph::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Dependencies", QString::number(entries_.size()), QColor(0x3b, 0x82, 0xf6)},
        {"Cyclic",       QString::number(cyclicCount()),  QColor(0xdc, 0x26, 0x26)},
        {"Avg Crit",     QString::number(avgCriticality() * 100, 'f', 0) + "%",
                                                           QColor(0xd9, 0x77, 0x06)},
        {"Categories",   QString::number(categoryCounts().size()),
                                                           QColor(0x7c, 0x3a, 0xed)}
    };

    int boxH = qMin(42, (rect.height() - 10) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 22,
                   Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 26, rect.width() - 20, 14,
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperDependencyGraph::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Add dependencies");
        return;
    }
    infoLabel_->setText(QString("%1 deps | %2 cyclic | %3% crit")
        .arg(entries_.size())
        .arg(cyclicCount())
        .arg(avgCriticality() * 100, 0, 'f', 0));
}

void PaperDependencyGraph::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        DependencyEntry e;
        e.id          = settings_.value("id").toInt();
        e.module      = settings_.value("module").toString();
        e.category    = settings_.value("category").toString();
        e.depends     = settings_.value("depends").toString();
        e.depth       = settings_.value("depth").toInt();
        e.criticality = settings_.value("criticality").toDouble();
        e.version     = settings_.value("version").toString();
        e.cyclic      = settings_.value("cyclic").toBool();
        e.color       = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperDependencyGraph::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",          entries_[i].id);
        settings_.setValue("module",      entries_[i].module);
        settings_.setValue("category",    entries_[i].category);
        settings_.setValue("depends",     entries_[i].depends);
        settings_.setValue("depth",       entries_[i].depth);
        settings_.setValue("criticality", entries_[i].criticality);
        settings_.setValue("version",     entries_[i].version);
        settings_.setValue("cyclic",      entries_[i].cyclic);
        settings_.setValue("color",       entries_[i].color.name());
    }
    settings_.endArray();
}
