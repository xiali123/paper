#include "workspace/PaperBudgetForecast.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperBudgetForecast::PaperBudgetForecast(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "BudgetForecast")
{
    setupUI();
    loadSettings();
}

void PaperBudgetForecast::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    forecastBtn_ = new QPushButton("Forecast");
    forecastBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(forecastBtn_, &QPushButton::clicked, this, &PaperBudgetForecast::onForecast);
    toolbar->addWidget(forecastBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Publishing", "Equipment", "Travel", "Software"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperBudgetForecast::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter project name for budget forecast...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Forecast research budget");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperBudgetForecast::addEntry(const BudgetEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit forecastUpdated(entry.id, entry.variance);
    update();
}

QList<BudgetEntry> PaperBudgetForecast::entries() const { return entries_; }

qreal PaperBudgetForecast::totalAllocated() const {
    qreal t = 0;
    for (const auto& e : entries_) t += e.allocated;
    return t;
}

int PaperBudgetForecast::onBudgetCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.onBudget) c++;
    return c;
}

QMap<QString, int> PaperBudgetForecast::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperBudgetForecast::onForecast() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"publishing", "equipment", "travel", "software", "services"};
    QStringList periods = {"Q1", "Q2", "Q3", "Q4"};
    QStringList projects = {"Paper A", "Paper B", "Paper C"};

    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        BudgetEntry e;
        e.id = entries_.size() + 1;
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.allocated = 500 + QRandomGenerator::global()->bounded(10000);
        e.spent = QRandomGenerator::global()->bounded(static_cast<int>(e.allocated + 2000));
        e.forecast = e.spent * (0.8 + QRandomGenerator::global()->bounded(40) / 100.0);
        e.period = periods[QRandomGenerator::global()->bounded(periods.size())];
        e.variance = (e.forecast - e.allocated) / e.allocated;
        e.project = text.left(10) + " - " + projects[i % projects.size()];
        e.papersAffected = 1 + QRandomGenerator::global()->bounded(5);
        e.onBudget = e.variance <= 0.1;
        e.color = e.onBudget ? QColor(16,185,129) : (e.variance <= 0.3 ? QColor(245,158,11) : QColor(239,68,68));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperBudgetForecast::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Forecast research budget");
    update();
}

void PaperBudgetForecast::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Forecast research budget");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Budget Forecast");

    int w = width(), h = height();
    drawBudgetList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperBudgetForecast::drawBudgetList(QPainter& p, const QRect& rect) {
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
                   e.category.left(10) + " [" + e.period + "]" + (e.onBudget ? " OK" : " !!"));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.project.left(18));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "$" + QString::number(static_cast<int>(e.forecast)));
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "var " + QString::number(e.variance * 100, 'f', 0) + "% | " + QString::number(e.papersAffected) + " papers");
    }
}

void PaperBudgetForecast::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"publishing", "equipment", "travel", "software", "services"};
    QString labels[] = {"Publish", "Equip", "Travel", "Software", "Service"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68), QColor(139,92,246)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 2);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 60, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 65, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 68 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperBudgetForecast::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Items", QString::number(entries_.size()), QColor(59,130,246)},
        {"On Budget", QString::number(onBudgetCount()), QColor(16,185,129)},
        {"Total Alloc", "$" + QString::number(static_cast<int>(totalAllocated())), QColor(245,158,11)},
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

void PaperBudgetForecast::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Forecast research budget"); return; }
    infoLabel_->setText(QString("%1 items | %2 on-budget | $%3 total")
        .arg(entries_.size()).arg(onBudgetCount()).arg(static_cast<int>(totalAllocated())));
}

void PaperBudgetForecast::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        BudgetEntry e;
        e.id = settings_.value("id").toInt();
        e.category = settings_.value("category").toString();
        e.allocated = settings_.value("allocated").toDouble();
        e.spent = settings_.value("spent").toDouble();
        e.forecast = settings_.value("forecast").toDouble();
        e.period = settings_.value("period").toString();
        e.variance = settings_.value("variance").toDouble();
        e.project = settings_.value("project").toString();
        e.papersAffected = settings_.value("papersAffected").toInt();
        e.onBudget = settings_.value("onBudget").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperBudgetForecast::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("allocated", entries_[i].allocated);
        settings_.setValue("spent", entries_[i].spent);
        settings_.setValue("forecast", entries_[i].forecast);
        settings_.setValue("period", entries_[i].period);
        settings_.setValue("variance", entries_[i].variance);
        settings_.setValue("project", entries_[i].project);
        settings_.setValue("papersAffected", entries_[i].papersAffected);
        settings_.setValue("onBudget", entries_[i].onBudget);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
