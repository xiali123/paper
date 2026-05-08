#include "workspace/ResearchNoteWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

ResearchNoteWidget::ResearchNoteWidget(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ResearchNotes")
{
    setupUI();
    loadSettings();
}

void ResearchNoteWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Ideas", "Findings", "Methods", "Questions", "References"});
    connect(categoryCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ResearchNoteWidget::onCategoryFilter);
    toolbar->addWidget(categoryCombo_, 1);

    addBtn_ = new QPushButton("Add Note");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &ResearchNoteWidget::onAdd);
    toolbar->addWidget(addBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &ResearchNoteWidget::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Add research notes");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(550, 450);
}

void ResearchNoteWidget::addNote(const ResearchNote& note) {
    notes_.append(note);
    saveSettings();
    updateInfo();
    emit notesChanged(notes_.size());
    update();
}

QList<ResearchNote> ResearchNoteWidget::notes() const { return notes_; }

QMap<QString, int> ResearchNoteWidget::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& n : notes_) counts[n.category]++;
    return counts;
}

QStringList ResearchNoteWidget::allTags() const {
    QStringList tags;
    for (const auto& n : notes_) tags.append(n.tags);
    tags.removeDuplicates();
    return tags;
}

void ResearchNoteWidget::onAdd() {
    bool ok;
    QString title = QInputDialog::getText(this, "Add Note", "Title:", QLineEdit::Normal, "", &ok);
    if (!ok || title.isEmpty()) return;
    QStringList cats = {"idea", "finding", "method", "question", "reference"};
    QString cat = QInputDialog::getItem(this, "Add Note", "Category:", cats, 0, false, &ok);
    if (!ok) return;
    QString content = QInputDialog::getMultiLineText(this, "Add Note", "Content:", "", &ok);
    if (!ok) return;

    ResearchNote n;
    n.id = notes_.size() + 1;
    n.title = title;
    n.content = content;
    n.category = cat;
    n.createdDate = QDate::currentDate();

    QColor catColors[] = {QColor(245,158,11), QColor(16,185,129), QColor(59,130,246), QColor(239,68,68), QColor(139,92,246)};
    int catIdx = cats.indexOf(cat);
    n.color = catColors[qBound(0, catIdx, 4)];
    addNote(n);
}

void ResearchNoteWidget::onCategoryFilter(int) { update(); }
void ResearchNoteWidget::onClear() {
    notes_.clear();
    selectedNote_ = -1;
    saveSettings();
    infoLabel_->setText("Add research notes");
    update();
}

void ResearchNoteWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (notes_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Add research notes");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Research Notes");

    int w = width(), h = height();
    drawNoteCards(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 40));
    drawTimeline(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 60));
}

void ResearchNoteWidget::drawNoteCards(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Notes");

    int filterIdx = categoryCombo_->currentIndex();
    QStringList catKeys = {"idea", "finding", "method", "question", "reference"};
    QString filterCat = filterIdx > 0 ? catKeys[filterIdx - 1] : "";

    int cardH = 56;
    int count = 0;
    for (int i = 0; i < notes_.size(); ++i) {
        if (!filterCat.isEmpty() && notes_[i].category != filterCat) continue;
        int y = rect.y() + 18 + count * (cardH + 4);
        if (y + cardH > rect.bottom()) break;

        bool sel = (notes_[i].id == selectedNote_);
        p.setPen(Qt::NoPen);
        p.setBrush(sel ? QColor(241, 245, 249) : Qt::white);
        p.drawRoundedRect(rect.x(), y, rect.width(), cardH, 6, 6);

        p.setPen(Qt::NoPen);
        p.setBrush(notes_[i].color);
        p.drawRoundedRect(rect.x(), y, 4, cardH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 10, y + 14, rect.width() - 20, 16, Qt::AlignVCenter,
                   notes_[i].title.left(22));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 30, rect.width() - 20, 14, Qt::AlignVCenter,
                   notes_[i].content.left(40));

        p.setPen(notes_[i].color);
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 44, rect.width() - 20, 12, Qt::AlignVCenter,
                   notes_[i].category + " | " + notes_[i].createdDate.toString("MM/dd"));

        count++;
    }
}

void ResearchNoteWidget::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "By Category");

    auto counts = categoryCounts();
    QStringList cats = counts.keys();
    if (cats.isEmpty()) return;

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(20, (rect.height() - 40) / cats.size());
    for (int i = 0; i < cats.size(); ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int barW = static_cast<int>((static_cast<qreal>(counts[cats[i]]) / maxVal) * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 60, barH, Qt::AlignRight | Qt::AlignVCenter, cats[i]);

        QColor c(59 + (i * 37) % 180, 130 + (i * 23) % 120, 246 - (i * 17) % 100);
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawRoundedRect(rect.x() + 65, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 68 + barW, y + barH - 2, QString::number(counts[cats[i]]));
    }
}

void ResearchNoteWidget::drawTimeline(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Recent Activity");

    int show = qMin(6, notes_.size());
    int itemH = qMin(22, (rect.height() - 30) / qMax(1, show));

    QList<int> indices;
    for (int i = notes_.size() - 1; i >= 0 && indices.size() < show; --i) indices.append(i);

    for (int j = 0; j < indices.size(); ++j) {
        const auto& n = notes_[indices[j]];
        int y = rect.y() + 20 + j * itemH;

        p.setPen(Qt::NoPen);
        p.setBrush(n.color);
        p.drawEllipse(rect.x() + 4, y + itemH / 2 - 3, 6, 6);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 16, y, rect.width() - 60, itemH, Qt::AlignVCenter, n.title.left(18));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + rect.width() - 55, y, 55, itemH, Qt::AlignVCenter | Qt::AlignRight,
                   n.createdDate.toString("MM/dd"));
    }
}

void ResearchNoteWidget::updateInfo() {
    if (notes_.isEmpty()) { infoLabel_->setText("Add research notes"); return; }
    infoLabel_->setText(QString("%1 notes | %2 categories | %3 tags")
        .arg(notes_.size()).arg(categoryCounts().size()).arg(allTags().size()));
}

void ResearchNoteWidget::loadSettings() {
    int size = settings_.beginReadArray("notes");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ResearchNote n;
        n.id = settings_.value("id").toInt();
        n.title = settings_.value("title").toString();
        n.content = settings_.value("content").toString();
        n.category = settings_.value("category").toString();
        n.color = QColor(settings_.value("color").toString());
        n.createdDate = QDate::fromString(settings_.value("date").toString(), Qt::ISODate);
        n.paperTitle = settings_.value("paperTitle").toString();
        n.paperId = settings_.value("paperId").toInt();
        notes_.append(n);
    }
    settings_.endArray();
    updateInfo();
}

void ResearchNoteWidget::saveSettings() {
    settings_.beginWriteArray("notes");
    for (int i = 0; i < notes_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", notes_[i].id);
        settings_.setValue("title", notes_[i].title);
        settings_.setValue("content", notes_[i].content);
        settings_.setValue("category", notes_[i].category);
        settings_.setValue("color", notes_[i].color.name());
        settings_.setValue("date", notes_[i].createdDate.toString(Qt::ISODate));
        settings_.setValue("paperTitle", notes_[i].paperTitle);
        settings_.setValue("paperId", notes_[i].paperId);
    }
    settings_.endArray();
}
