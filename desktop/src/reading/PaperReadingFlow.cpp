#include "reading/PaperReadingFlow.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReadingFlow::PaperReadingFlow(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingFlow")
{
    setupUI();
    loadSettings();
}

void PaperReadingFlow::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    createBtn_ = new QPushButton("Create");
    createBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(createBtn_, &QPushButton::clicked, this, &PaperReadingFlow::onCreate);
    toolbar->addWidget(createBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Skim", "Deep", "Review", "Note"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingFlow::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter reading session...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Create reading flow");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperReadingFlow::addEntry(const FlowEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit flowUpdated(entry.id, entry.focus);
    update();
}

QList<FlowEntry> PaperReadingFlow::entries() const { return entries_; }

int PaperReadingFlow::activeCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.active) c++;
    return c;
}

qreal PaperReadingFlow::avgFocus() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.focus;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingFlow::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperReadingFlow::onCreate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"skim", "deep", "review", "note"};
    QStringList techniques = {"pomodoro", "sq3r", "spaced", "active recall"};
    QStringList steps = {"preview", "question", "read", "recite", "review"};
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        FlowEntry e;
        e.id = entries_.size() + 1;
        e.step = steps[i % steps.size()];
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.duration = 5 + QRandomGenerator::global()->bounded(60);
        e.focus = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
        e.retention = 0.2 + QRandomGenerator::global()->bounded(80) / 100.0;
        e.technique = techniques[QRandomGenerator::global()->bounded(techniques.size())];
        e.order = i;
        e.active = e.focus >= 0.6;
        e.color = e.active ? QColor(16,185,129) : (e.focus >= 0.4 ? QColor(59,130,246) : QColor(245,158,11));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperReadingFlow::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Create reading flow");
    update();
}

void PaperReadingFlow::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Create reading flow");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Flow");
    int w = width(), h = height();
    drawFlowView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReadingFlow::drawFlowView(QPainter& p, const QRect& rect) {
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
        // Connector arrow
        if (i > 0) {
            p.setPen(QPen(QColor(203, 213, 225), 1));
            p.drawLine(rect.x() + 10, y - 3, rect.x() + 10, y);
        }
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 14, y + 4, rect.width() / 2 - 14, 16, Qt::AlignVCenter,
                   e.step.left(10) + " [" + e.category.left(4) + "]" + (e.active ? " *" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 14, y + 20, rect.width() / 2 - 14, 14, Qt::AlignVCenter,
                   QString::number(e.duration) + "min | " + e.technique);
        int barW = static_cast<int>(e.focus * (rect.width() / 2 - 20));
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x() + rect.width() / 2, y + 10, barW, 14, 3, 3);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(rect.x() + rect.width() / 2 + barW + 4, y + 22,
                   QString::number(e.focus * 100, 'f', 0) + "% focus");
    }
}

void PaperReadingFlow::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"skim", "deep", "review", "note"};
    QString labels[] = {"Skim", "Deep", "Review", "Note"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 80));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x(), y + barH - 2, 30, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 35, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 38 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperReadingFlow::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Steps", QString::number(entries_.size()), QColor(59,130,246)},
        {"Active", QString::number(activeCount()), QColor(16,185,129)},
        {"Avg Focus", QString::number(avgFocus() * 100, 'f', 0) + "%", QColor(245,158,11)},
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

void PaperReadingFlow::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Create reading flow"); return; }
    infoLabel_->setText(QString("%1 steps | %2 active | %3% focus")
        .arg(entries_.size()).arg(activeCount()).arg(avgFocus() * 100, 0, 'f', 0));
}

void PaperReadingFlow::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        FlowEntry e;
        e.id = settings_.value("id").toInt();
        e.step = settings_.value("step").toString();
        e.category = settings_.value("category").toString();
        e.duration = settings_.value("duration").toInt();
        e.focus = settings_.value("focus").toDouble();
        e.retention = settings_.value("retention").toDouble();
        e.technique = settings_.value("technique").toString();
        e.order = settings_.value("order").toInt();
        e.active = settings_.value("active").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingFlow::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("step", entries_[i].step);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("duration", entries_[i].duration);
        settings_.setValue("focus", entries_[i].focus);
        settings_.setValue("retention", entries_[i].retention);
        settings_.setValue("technique", entries_[i].technique);
        settings_.setValue("order", entries_[i].order);
        settings_.setValue("active", entries_[i].active);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
