#include "workspace/PaperSprintRetroBoard.hpp"
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperSprintRetroBoard::PaperSprintRetroBoard(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "SprintRetroBoard")
{
    setupUI();
    loadSettings();
}

void PaperSprintRetroBoard::setupUI() {
    auto* layout = new QHBoxLayout(this);

    auto* leftPanel = new QWidget();
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    auto* toolbar = new QHBoxLayout();
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Went Well", "Improve", "Actions"});
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Retro item...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    addBtn_ = new QPushButton("Add");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperSprintRetroBoard::onAdd);
    toolbar->addWidget(addBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperSprintRetroBoard::onClear);
    toolbar->addWidget(clearBtn_);

    leftLayout->addLayout(toolbar);

    infoLabel_ = new QLabel("Items: 0 | Addressed: 0 | Avg Votes: 0.0");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    leftLayout->addWidget(infoLabel_);

    layout->addWidget(leftPanel, 0);
    layout->addStretch(1);

    setMinimumSize(640, 480);
}

void PaperSprintRetroBoard::addEntry(const SprintRetroEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<SprintRetroEntry> PaperSprintRetroBoard::entries() const {
    return entries_;
}

int PaperSprintRetroBoard::addressedCount() const {
    int count = 0;
    for (const auto& e : entries_)
        if (e.addressed) ++count;
    return count;
}

qreal PaperSprintRetroBoard::avgVotes() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.votes;
    return sum / entries_.size();
}

QMap<QString, int> PaperSprintRetroBoard::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperSprintRetroBoard::onAdd() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QColor palette[] = {
        QColor(59, 130, 246),   // #3b82f6
        QColor(22, 163, 74),    // #16a34a
        QColor(217, 119, 6),    // #d97706
        QColor(220, 38, 38),    // #dc2626
        QColor(124, 58, 237)    // #7c3aed
    };

    QStringList types = {"wentWell", "improve", "action"};
    QStringList categories = {"Went Well", "Improve", "Actions"};

    int cIdx = categoryCombo_->currentIndex();
    QString category = cIdx == 0
        ? categories[QRandomGenerator::global()->bounded(categories.size())]
        : categories[cIdx - 1];
    QString type = types[categories.indexOf(category)];

    SprintRetroEntry entry;
    entry.id = entries_.size() + 1;
    entry.title = text;
    entry.category = category;
    entry.type = type;
    entry.votes = QRandomGenerator::global()->bounded(21);         // 0-20
    entry.actionItems = QRandomGenerator::global()->bounded(9);    // 0-8
    entry.addressed = QRandomGenerator::global()->bounded(2) == 1; // random bool
    entry.color = palette[QRandomGenerator::global()->bounded(5)];

    addEntry(entry);
    inputField_->clear();
    emit retroSaved(entry.id, entry.votes);
}

void PaperSprintRetroBoard::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    repaint();
}

void PaperSprintRetroBoard::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Add retro items to the sprint board");
        return;
    }

    int w = width(), h = height();
    int colW = w / 3;

    drawRetroBoard(p, QRect(10, 10, colW * 2 - 20, h - 20));
    drawCategoryChart(p, QRect(colW * 2, 10, colW - 10, h / 2 - 10));
    drawStats(p, QRect(colW * 2, h / 2 + 5, colW - 10, h / 2 - 15));
}

void PaperSprintRetroBoard::drawRetroBoard(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 20, "Sprint Retro");

    QStringList columns = {"Went Well", "Improve", "Actions"};
    QColor colColors[] = {
        QColor(22, 163, 74),   // #16a34a
        QColor(217, 119, 6),   // #d97706
        QColor(59, 130, 246)   // #3b82f6
    };

    int colW = (rect.width() - 20) / 3;
    int headerH = 30;
    int cardTop = rect.y() + headerH + 10;

    for (int c = 0; c < 3; ++c) {
        int colX = rect.x() + c * (colW + 5);

        // Column header
        p.setPen(Qt::NoPen);
        p.setBrush(colColors[c].lighter(185));
        p.drawRoundedRect(colX, rect.y() + headerH, colW, 24, 4, 4);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 10, QFont::Bold));
        p.drawText(colX + 6, rect.y() + headerH, colW - 12, 24,
                   Qt::AlignVCenter, columns[c]);

        // Cards for this column
        QList<int> colIndices;
        for (int i = 0; i < entries_.size(); ++i)
            if (entries_[i].category == columns[c]) colIndices.append(i);

        int maxCards = qMin(static_cast<int>(colIndices.size()),
                            (rect.height() - headerH - 40) / qMax(1, 52));
        int cardH = qMin(48, (rect.height() - headerH - 40) / qMax(maxCards, 1) - 4);

        for (int j = 0; j < maxCards; ++j) {
            const auto& e = entries_[colIndices[j]];
            int y = cardTop + j * (cardH + 4);

            // Card background
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(248, 250, 252));
            p.drawRoundedRect(colX, y, colW, cardH, 4, 4);

            // Color accent bar
            p.setBrush(e.color);
            p.drawRoundedRect(colX, y, 4, cardH, 2, 2);

            // Title text
            p.setPen(QColor(15, 23, 42));
            p.setFont(QFont("Arial", 8, QFont::Bold));
            p.drawText(colX + 10, y + 2, colW - 16, 16, Qt::AlignVCenter,
                       e.title.left(20));

            // Vote count
            p.setPen(QColor(100, 116, 139));
            p.setFont(QFont("Arial", 7));
            p.drawText(colX + 10, y + 18, colW / 2 - 10, 14, Qt::AlignVCenter,
                       "Votes: " + QString::number(static_cast<int>(e.votes)));

            // Action items count
            p.drawText(colX + colW / 2, y + 18, colW / 2 - 10, 14,
                       Qt::AlignVCenter | Qt::AlignRight,
                       "Actions: " + QString::number(e.actionItems));

            // Addressed badge
            if (e.addressed) {
                p.setPen(Qt::NoPen);
                p.setBrush(QColor(22, 163, 74));
                QRect badgeRect(colX + colW - 58, y + 34, 50, 12);
                p.drawRoundedRect(badgeRect, 3, 3);
                p.setPen(Qt::white);
                p.setFont(QFont("Arial", 7, QFont::Bold));
                p.drawText(badgeRect, Qt::AlignCenter, "Addressed");
            }
        }
    }
}

void PaperSprintRetroBoard::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList categories = {"Went Well", "Improve", "Actions"};
    QString labels[] = {"Went Well", "Improve", "Actions"};
    QColor colors[] = {
        QColor(22, 163, 74),   // #16a34a
        QColor(217, 119, 6),   // #d97706
        QColor(59, 130, 246)   // #3b82f6
    };

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 40) / 3);
    for (int i = 0; i < 3; ++i) {
        int y = rect.y() + 25 + i * (barH + 6);
        int count = counts.contains(labels[i]) ? counts[labels[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 80));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + 2, 65, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 70, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 74 + barW, y + barH - 4, QString::number(count));
    }
}

void PaperSprintRetroBoard::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Items", QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Addressed",   QString::number(addressedCount()), QColor(22, 163, 74)},
        {"Avg Votes",   QString::number(avgVotes(), 'f', 1), QColor(217, 119, 6)}
    };

    int boxH = qMin(42, (rect.height() - 10) / 3);
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

void PaperSprintRetroBoard::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Items: 0 | Addressed: 0 | Avg Votes: 0.0");
        return;
    }
    infoLabel_->setText(QString("Items: %1 | Addressed: %2 | Avg Votes: %3")
        .arg(entries_.size())
        .arg(addressedCount())
        .arg(avgVotes(), 0, 'f', 1));
}

void PaperSprintRetroBoard::loadSettings() {
    settings_.beginGroup("SprintRetroBoard");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        SprintRetroEntry e;
        e.id = settings_.value("id").toInt();
        e.title = settings_.value("title").toString();
        e.category = settings_.value("category").toString();
        e.type = settings_.value("type").toString();
        e.votes = settings_.value("votes").toDouble();
        e.actionItems = settings_.value("actionItems").toInt();
        e.addressed = settings_.value("addressed").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperSprintRetroBoard::saveSettings() {
    settings_.beginGroup("SprintRetroBoard");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("title", entries_[i].title);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("type", entries_[i].type);
        settings_.setValue("votes", entries_[i].votes);
        settings_.setValue("actionItems", entries_[i].actionItems);
        settings_.setValue("addressed", entries_[i].addressed);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
