#include "latex/LatexSnippetManager.hpp"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QRegularExpression>

LatexSnippetManager::LatexSnippetManager(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadSnippets();
}

void LatexSnippetManager::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    searchEdit_ = new QLineEdit();
    searchEdit_->setPlaceholderText("Search snippets...");
    searchEdit_->setClearButtonEnabled(true);
    connect(searchEdit_, &QLineEdit::textChanged, this, &LatexSnippetManager::onSearch);
    layout->addWidget(searchEdit_);

    snippetTree_ = new QTreeWidget();
    snippetTree_->setHeaderLabels({"Snippet", "Description"});
    snippetTree_->header()->setStretchLastSection(true);
    snippetTree_->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    connect(snippetTree_, &QTreeWidget::itemActivated, this, &LatexSnippetManager::onItemActivated);
    connect(snippetTree_, &QTreeWidget::itemClicked, this, &LatexSnippetManager::onItemActivated);
    layout->addWidget(snippetTree_, 1);

    setMaximumWidth(280);
}

void LatexSnippetManager::loadSnippets() {
    // Document Structure
    addCategory("Document Structure", "doc", {
        {"Section", "doc", "\\section{", "}", "\\section{Title}"},
        {"Subsection", "doc", "\\subsection{", "}", "\\subsection{Title}"},
        {"Subsubsection", "doc", "\\subsubsection{", "}", "\\subsubsection{Title}"},
        {"Paragraph", "doc", "\\paragraph{", "}", "\\paragraph{Title}"},
        {"Appendix", "doc", "\\appendix\n", "", "Start appendix section"},
        {"Table of Contents", "doc", "\\tableofcontents\n", "", "Insert TOC"},
    });

    // Text Formatting
    addCategory("Text Formatting", "fmt", {
        {"Bold", "fmt", "\\textbf{", "}", "Bold text"},
        {"Italic", "fmt", "\\textit{", "}", "Italic text"},
        {"Underline", "fmt", "\\underline{", "}", "Underlined text"},
        {"Emphasis", "fmt", "\\emph{", "}", "Emphasized text"},
        {"Typewriter", "fmt", "\\texttt{", "}", "Monospace text"},
        {"Small Caps", "fmt", "\\textsc{", "}", "Small capitals"},
        {"Text Color", "fmt", "\\textcolor{red}{", "}", "Colored text"},
        {"Font Size Small", "fmt", "{\\small ", "}", "Small text"},
        {"Font Size Large", "fmt", "{\\large ", "}", "Large text"},
        {"Font Size Huge", "fmt", "{\\huge ", "}", "Huge text"},
    });

    // Math
    addCategory("Math", "math", {
        {"Inline Math", "math", "$", "$", "Inline math $x^2$"},
        {"Display Math", "math", "\n\\[\n", "\n\\]\n", "Display equation"},
        {"Equation", "math", "\\begin{equation}\n", "\n\\end{equation}\n", "Numbered equation"},
        {"Align", "math", "\\begin{align}\n", "\n\\end{align}\n", "Aligned equations"},
        {"Fraction", "math", "\\frac{", "}{}", "\\frac{num}{den}"},
        {"Square Root", "math", "\\sqrt{", "}", "Square root"},
        {"Nth Root", "math", "\\sqrt[", "]{}", "\\sqrt[n]{x}"},
        {"Sum", "math", "\\sum_{i=1}^{n} ", "", "Summation"},
        {"Product", "math", "\\prod_{i=1}^{n} ", "", "Product"},
        {"Integral", "math", "\\int_{a}^{b} ", " dx", "Integral"},
        {"Limit", "math", "\\lim_{x \\to \\infty} ", "", "Limit"},
        {"Matrix", "math", "\\begin{pmatrix}\n", "\n\\end{pmatrix}", "Matrix (parentheses)"},
        {"Bmatrix", "math", "\\begin{bmatrix}\n", "\n\\end{bmatrix}", "Matrix (brackets)"},
        {"Cases", "math", "\\begin{cases}\n", "\n\\end{cases}", "Cases environment"},
        {"Superscript", "math", "^{", "}", "Superscript"},
        {"Subscript", "math", "_{", "}", "Subscript"},
        {"Overline", "math", "\\overline{", "}", "Overline"},
        {"Hat", "math", "\\hat{", "}", "Hat accent"},
        {"Bar", "math", "\\bar{", "}", "Bar accent"},
        {"Vec", "math", "\\vec{", "}", "Vector"},
    });

    // Lists
    addCategory("Lists", "list", {
        {"Itemize", "list", "\\begin{itemize}\n  \\item ", "\n\\end{itemize}", "Bullet list"},
        {"Enumerate", "list", "\\begin{enumerate}\n  \\item ", "\n\\end{enumerate}", "Numbered list"},
        {"Description", "list", "\\begin{description}\n  \\item[Label] ", "\n\\end{description}", "Description list"},
    });

    // Tables
    addCategory("Tables", "table", {
        {"Basic Table", "table",
         "\\begin{table}[h]\n\\centering\n\\begin{tabular}{|l|c|r|}\n\\hline\n"
         "Header1 & Header2 & Header3 \\\\\n\\hline\n",
         " \\\\\n\\hline\n\\end{tabular}\n\\caption{Caption}\n\\label{tab:label}\n\\end{table}\n",
         "Basic table with borders"},
        {"Booktabs Table", "table",
         "\\begin{table}[h]\n\\centering\n\\begin{tabular}{lcc}\n\\toprule\n"
         "Header1 & Header2 & Header3 \\\\\n\\midrule\n",
         " \\\\\n\\bottomrule\n\\end{tabular}\n\\caption{Caption}\n\\label{tab:label}\n\\end{table}\n",
         "Professional table (requires booktabs)"},
        {"Tabular", "table",
         "\\begin{tabular}{|l|c|r|}\n\\hline\n",
         " \\\\\n\\hline\n\\end{tabular}\n",
         "Inline tabular"},
    });

    // Figures
    addCategory("Figures", "figure", {
        {"Figure", "figure",
         "\\begin{figure}[h]\n\\centering\n\\includegraphics[width=0.8\\textwidth]{",
         "}\n\\caption{Caption}\n\\label{fig:label}\n\\end{figure}\n",
         "Insert figure"},
        {"Figure (draft)", "figure",
         "\\begin{figure}[h]\n\\centering\n\\fbox{\\parbox{0.8\\textwidth}{\\centering\\vspace{2cm}Figure placeholder\\vspace{2cm}}}\n\\caption{",
         "}\n\\label{fig:label}\n\\end{figure}\n",
         "Figure placeholder box"},
    });

    // Code
    addCategory("Code Listings", "code", {
        {"Verbatim", "code", "\\begin{verbatim}\n", "\n\\end{verbatim}\n", "Verbatim text"},
        {"Code Block", "code",
         "\\begin{lstlisting}[language=Python]\n",
         "\n\\end{lstlisting}\n",
         "Code listing (requires listings)"},
        {"Inline Code", "code", "\\lstinline|", "|", "Inline code"},
    });

    // References
    addCategory("References", "ref", {
        {"Citation", "ref", "\\cite{", "}", "Citation"},
        {"Label", "ref", "\\label{", "}", "Label for cross-reference"},
        {"Reference", "ref", "\\ref{", "}", "Cross-reference"},
        {"Page Reference", "ref", "\\pageref{", "}", "Page reference"},
        {"Eq. Reference", "ref", "\\eqref{", "}", "Equation reference (amsmath)"},
        {"URL", "ref", "\\url{", "}", "URL link"},
        {"Hyperlink", "ref", "\\href{", "}{}", "Hyperlink with text"},
        {"Footnote", "ref", "\\footnote{", "}", "Footnote"},
        {"Bibliography", "ref", "\\bibliographystyle{plain}\n\\bibliography{", "}\n", "Bibliography file"},
    });
}

void LatexSnippetManager::addCategory(const QString& name, const QString& icon,
                                       const QList<LatexSnippet>& snippets) {
    allSnippets_[name] = snippets;

    auto* catItem = new QTreeWidgetItem({name, ""});
    catItem->setFlags(catItem->flags() & ~Qt::ItemIsSelectable);
    QFont catFont = catItem->font(0);
    catFont.setBold(true);
    catItem->setFont(0, catFont);
    snippetTree_->addTopLevelItem(catItem);

    for (const auto& snippet : snippets) {
        auto* item = new QTreeWidgetItem({snippet.name, snippet.description});
        item->setData(0, Qt::UserRole, snippet.insertBefore);
        item->setData(0, Qt::UserRole + 1, snippet.insertAfter);
        catItem->addChild(item);
    }
    catItem->setExpanded(true);
}

void LatexSnippetManager::onSearch(const QString& text) {
    QString lower = text.trimmed().toLower();

    for (int i = 0; i < snippetTree_->topLevelItemCount(); ++i) {
        auto* catItem = snippetTree_->topLevelItem(i);
        if (lower.isEmpty()) {
            catItem->setHidden(false);
            for (int j = 0; j < catItem->childCount(); ++j) {
                catItem->child(j)->setHidden(false);
            }
            continue;
        }

        bool anyVisible = false;
        for (int j = 0; j < catItem->childCount(); ++j) {
            auto* child = catItem->child(j);
            bool match = child->text(0).toLower().contains(lower) ||
                         child->text(1).toLower().contains(lower);
            child->setHidden(!match);
            if (match) anyVisible = true;
        }
        catItem->setHidden(!anyVisible);
    }
}

void LatexSnippetManager::onItemActivated(QTreeWidgetItem* item, int) {
    if (!item || !item->parent()) return;

    QString before = item->data(0, Qt::UserRole).toString();
    QString after = item->data(0, Qt::UserRole + 1).toString();
    emit snippetInsert(before, after);
}
