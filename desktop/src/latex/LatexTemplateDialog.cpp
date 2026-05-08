#include "latex/LatexTemplateDialog.hpp"
#include "core/ApiManager.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QHeaderView>
#include <QMessageBox>
#include <QJsonArray>

LatexTemplateDialog::LatexTemplateDialog(ApiManager* apiManager, QWidget* parent)
    : QDialog(parent)
    , apiManager_(apiManager)
{
    setWindowTitle("LaTeX Templates");
    resize(900, 600);
    setupUI();
    populateBuiltInTemplates();
}

void LatexTemplateDialog::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);

    // Top bar: category filter + search + refresh
    auto* topBar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItem("All Categories", "");
    categoryCombo_->addItem("Articles", "article");
    categoryCombo_->addItem("Reports", "report");
    categoryCombo_->addItem("Presentations", "presentation");
    categoryCombo_->addItem("Letters", "letter");
    categoryCombo_->addItem("Books", "book");
    categoryCombo_->addItem("Math", "math");
    categoryCombo_->addItem("Thesis", "thesis");
    categoryCombo_->setMinimumWidth(150);
    connect(categoryCombo_, &QComboBox::currentTextChanged, this, &LatexTemplateDialog::onCategoryFilter);
    topBar->addWidget(categoryCombo_);

    searchEdit_ = new QLineEdit();
    searchEdit_->setPlaceholderText("Search templates...");
    searchEdit_->setClearButtonEnabled(true);
    connect(searchEdit_, &QLineEdit::textChanged, this, &LatexTemplateDialog::onCategoryFilter);
    topBar->addWidget(searchEdit_, 1);

    refreshBtn_ = new QPushButton("Load from Server");
    connect(refreshBtn_, &QPushButton::clicked, this, &LatexTemplateDialog::onLoadFromServer);
    topBar->addWidget(refreshBtn_);

    mainLayout->addLayout(topBar);

    // Splitter: tree | preview
    auto* splitter = new QSplitter(Qt::Horizontal);

    templateTree_ = new QTreeWidget();
    templateTree_->setHeaderLabels({"Name", "Category"});
    templateTree_->header()->setStretchLastSection(true);
    templateTree_->setColumnWidth(0, 200);
    templateTree_->setRootIsDecorated(false);
    connect(templateTree_, &QTreeWidget::currentItemChanged,
            this, [this](QTreeWidgetItem* item) { onTemplateSelected(item, 0); });
    connect(templateTree_, &QTreeWidget::itemDoubleClicked, this, &LatexTemplateDialog::onTemplateSelected);
    splitter->addWidget(templateTree_);

    auto* rightPanel = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    descLabel_ = new QLabel("Select a template to preview");
    descLabel_->setWordWrap(true);
    descLabel_->setStyleSheet("font-weight: bold; font-size: 13px; color: palette(text); padding: 4px;");
    rightLayout->addWidget(descLabel_);

    previewEdit_ = new QTextEdit();
    previewEdit_->setReadOnly(true);
    previewEdit_->setFont(QFont("Consolas", 11));
    rightLayout->addWidget(previewEdit_, 1);

    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);
    mainLayout->addWidget(splitter, 1);

    // Bottom buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    applyBtn_ = new QPushButton("Apply Template");
    applyBtn_->setEnabled(false);
    applyBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 8px 24px; "
        "border-radius: 6px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }"
        "QPushButton:disabled { background: palette(mid); color: palette(mid); }"
    );
    connect(applyBtn_, &QPushButton::clicked, this, &LatexTemplateDialog::onApply);
    btnLayout->addWidget(applyBtn_);

    auto* closeBtn = new QPushButton("Close");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(closeBtn);

    mainLayout->addLayout(btnLayout);
}

void LatexTemplateDialog::populateBuiltInTemplates() {
    builtInTemplates_ = {
        {0, "Article", "Standard article with sections", "article",
         "\\documentclass{article}\n"
         "\\usepackage[utf8]{inputenc}\n"
         "\\usepackage{amsmath,amssymb}\n"
         "\\usepackage{graphicx}\n"
         "\\usepackage{hyperref}\n\n"
         "\\title{Title}\n\\author{Author}\n\\date{\\today}\n\n"
         "\\begin{document}\n\\maketitle\n\n"
         "\\begin{abstract}\nAbstract text.\n\\end{abstract}\n\n"
         "\\section{Introduction}\n\n\\section{Methods}\n\n"
         "\\section{Results}\n\n\\section{Conclusion}\n\n"
         "\\bibliographystyle{plain}\n\\bibliography{refs}\n\n"
         "\\end{document}\n", "article", true},

        {0, "IEEE Paper", "IEEE conference/journal paper format", "article",
         "\\documentclass[conference]{IEEEtran}\n"
         "\\usepackage{cite}\n\\usepackage{amsmath}\n\\usepackage{graphicx}\n\n"
         "\\begin{document}\n\n"
         "\\title{Paper Title}\n\n"
         "\\author{\\IEEEauthorblockN{Author Name}\n"
         "\\IEEEauthorblockA{Affiliation}}\n\n"
         "\\maketitle\n\n"
         "\\begin{abstract}\nAbstract.\n\\end{abstract}\n\n"
         "\\begin{IEEEkeywords}\nkeyword1, keyword2\n\\end{IEEEkeywords}\n\n"
         "\\section{Introduction}\n\\section{Related Work}\n"
         "\\section{Methodology}\n\\section{Experiments}\n"
         "\\section{Conclusion}\n\n"
         "\\bibliographystyle{IEEEtran}\n\\bibliography{refs}\n\n"
         "\\end{document}\n", "article", true},

        {0, "Report", "Technical report with chapters", "report",
         "\\documentclass{report}\n"
         "\\usepackage[utf8]{inputenc}\n\\usepackage{graphicx}\n\\usepackage{hyperref}\n\n"
         "\\title{Report Title}\n\\author{Author}\n\\date{\\today}\n\n"
         "\\begin{document}\n\\maketitle\n\\tableofcontents\n\n"
         "\\chapter{Introduction}\n\\chapter{Background}\n"
         "\\chapter{Analysis}\n\\chapter{Results}\n"
         "\\chapter{Conclusion}\n\n"
         "\\bibliographystyle{plain}\n\\bibliography{refs}\n\n"
         "\\end{document}\n", "report", true},

        {0, "Beamer Presentation", "Slideshow with Beamer", "presentation",
         "\\documentclass{beamer}\n"
         "\\usetheme{Madrid}\n\\usecolortheme{default}\n\n"
         "\\title{Presentation Title}\n\\author{Author}\n\\date{\\today}\n\n"
         "\\begin{document}\n\n"
         "\\begin{frame}\n\\titlepage\n\\end{frame}\n\n"
         "\\begin{frame}{Outline}\n\\tableofcontents\n\\end{frame}\n\n"
         "\\section{Introduction}\n"
         "\\begin{frame}{Introduction}\nContent here.\n\\end{frame}\n\n"
         "\\section{Methods}\n"
         "\\begin{frame}{Methods}\nContent here.\n\\end{frame}\n\n"
         "\\section{Results}\n"
         "\\begin{frame}{Results}\nContent here.\n\\end{frame}\n\n"
         "\\begin{frame}\n\\centering\n\\Huge Thank You!\n\\end{frame}\n\n"
         "\\end{document}\n", "presentation", true},

        {0, "Letter", "Formal letter", "letter",
         "\\documentclass{letter}\n"
         "\\usepackage[utf8]{inputenc}\n\n"
         "\\address{Sender Name \\\\ Address \\\\ City}\n"
         "\\signature{Sender Name}\n\n"
         "\\begin{document}\n\n"
         "\\begin{letter}{Recipient Name \\\\ Address \\\\ City}\n\n"
         "\\opening{Dear Sir/Madam,}\n\n"
         "Letter body here.\n\n"
         "\\closing{Sincerely,}\n\n"
         "\\end{letter}\n\\end{document}\n", "letter", true},

        {0, "Book", "Book with chapters and parts", "book",
         "\\documentclass{book}\n"
         "\\usepackage[utf8]{inputenc}\n\\usepackage{graphicx}\n\\usepackage{hyperref}\n\n"
         "\\title{Book Title}\n\\author{Author}\n\\date{\\today}\n\n"
         "\\begin{document}\n\n\\maketitle\n\\tableofcontents\n\n"
         "\\part{Part One}\n\\chapter{Chapter One}\n\\section{Section}\n\n"
         "\\part{Part Two}\n\\chapter{Chapter Two}\n\n"
         "\\end{document}\n", "book", true},

        {0, "Math Homework", "Math-focused document with AMS packages", "math",
         "\\documentclass{article}\n"
         "\\usepackage{amsmath,amssymb,amsthm}\n\\usepackage{mathtools}\n"
         "\\usepackage{enumitem}\n\n"
         "\\newtheorem{theorem}{Theorem}\n\\newtheorem{lemma}{Lemma}\n\n"
         "\\title{Math Homework}\n\\author{Student}\n\\date{\\today}\n\n"
         "\\begin{document}\n\\maketitle\n\n"
         "\\section{Problem 1}\n\n"
         "\\begin{theorem}\nStatement.\n\\end{theorem}\n\n"
         "\\begin{proof}\nProof here.\n\\end{proof}\n\n"
         "\\section{Problem 2}\n\n"
         "Inline math: $E = mc^2$\n\n"
         "Display math:\n\\begin{equation}\n  a^2 + b^2 = c^2\n\\end{equation}\n\n"
         "\\end{document}\n", "math", true},

        {0, "Thesis", "Masters/PhD thesis template", "thesis",
         "\\documentclass[12pt]{report}\n"
         "\\usepackage[utf8]{inputenc}\n\\usepackage{graphicx}\n"
         "\\usepackage{amsmath}\n\\usepackage{hyperref}\n"
         "\\usepackage{setspace}\n\\onehalfspacing\n\n"
         "\\title{Thesis Title}\n\\author{Author Name}\n\n"
         "\\begin{document}\n\n"
         "\\maketitle\n\n"
         "\\begin{abstract}\nAbstract.\n\\end{abstract}\n\n"
         "\\tableofcontents\n\\listoffigures\n\\listoftables\n\n"
         "\\chapter{Introduction}\n\\chapter{Literature Review}\n"
         "\\chapter{Methodology}\n\\chapter{Results}\n"
         "\\chapter{Discussion}\n\\chapter{Conclusion}\n\n"
         "\\bibliographystyle{apalike}\n\\bibliography{refs}\n\n"
         "\\end{document}\n", "thesis", true},
    };

    for (const auto& tmpl : builtInTemplates_) {
        addTemplateItem(tmpl);
    }
}

void LatexTemplateDialog::addTemplateItem(const LatexTemplate& tmpl) {
    auto* item = new QTreeWidgetItem({tmpl.name, tmpl.category});
    item->setData(0, Qt::UserRole, tmpl.id);
    item->setData(0, Qt::UserRole + 1, tmpl.content);
    item->setData(0, Qt::UserRole + 2, tmpl.description);
    item->setData(0, Qt::UserRole + 3, tmpl.category);
    item->setToolTip(0, tmpl.description);
    templateTree_->addTopLevelItem(item);
}

void LatexTemplateDialog::onCategoryFilter(const QString&) {
    QString category = categoryCombo_->currentData().toString();
    QString search = searchEdit_->text().trimmed().toLower();

    for (int i = 0; i < templateTree_->topLevelItemCount(); ++i) {
        auto* item = templateTree_->topLevelItem(i);
        QString itemCat = item->data(0, Qt::UserRole + 3).toString();
        QString name = item->text(0).toLower();
        bool catMatch = category.isEmpty() || itemCat == category;
        bool searchMatch = search.isEmpty() || name.contains(search);
        item->setHidden(!(catMatch && searchMatch));
    }
}

void LatexTemplateDialog::onTemplateSelected(QTreeWidgetItem* item, int) {
    if (!item || item->isHidden()) {
        previewEdit_->clear();
        descLabel_->setText("Select a template to preview");
        applyBtn_->setEnabled(false);
        return;
    }

    selectedTemplate_.id = item->data(0, Qt::UserRole).toInt();
    selectedTemplate_.content = item->data(0, Qt::UserRole + 1).toString();
    selectedTemplate_.description = item->data(0, Qt::UserRole + 2).toString();
    selectedTemplate_.name = item->text(0);
    selectedTemplate_.category = item->data(0, Qt::UserRole + 3).toString();

    descLabel_->setText(QString("<b>%1</b> (%2)<br><span style='color: palette(mid);'>%3</span>")
                            .arg(selectedTemplate_.name, selectedTemplate_.category, selectedTemplate_.description));
    previewEdit_->setPlainText(selectedTemplate_.content);
    applyBtn_->setEnabled(true);
}

void LatexTemplateDialog::onApply() {
    if (selectedTemplate_.content.isEmpty()) return;

    auto result = QMessageBox::question(this, "Apply Template",
        "This will replace the current document content. Continue?",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (result == QMessageBox::Yes) {
        emit templateApplied(selectedTemplate_.content);
        accept();
    }
}

void LatexTemplateDialog::onLoadFromServer() {
    apiManager_->listLatexTemplates();
    refreshBtn_->setEnabled(false);
    refreshBtn_->setText("Loading...");
}
