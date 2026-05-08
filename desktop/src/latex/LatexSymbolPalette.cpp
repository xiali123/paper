#include "latex/LatexSymbolPalette.hpp"
#include <QVBoxLayout>
#include <QPushButton>
#include <QToolTip>
#include <QScrollArea>

LatexSymbolPalette::LatexSymbolPalette(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void LatexSymbolPalette::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    tabWidget_ = new QTabWidget();
    tabWidget_->setTabPosition(QTabWidget::South);

    // Greek Letters (lowercase)
    tabWidget_->addTab(
        createSymbolGrid({
            {"\\alpha", "α", "alpha"}, {"\\beta", "β", "beta"},
            {"\\gamma", "γ", "gamma"}, {"\\delta", "δ", "delta"},
            {"\\epsilon", "ε", "epsilon"}, {"\\varepsilon", "ϵ", "varepsilon"},
            {"\\zeta", "ζ", "zeta"}, {"\\eta", "η", "eta"},
            {"\\theta", "θ", "theta"}, {"\\vartheta", "ϑ", "vartheta"},
            {"\\iota", "ι", "iota"}, {"\\kappa", "κ", "kappa"},
            {"\\lambda", "λ", "lambda"}, {"\\mu", "μ", "mu"},
            {"\\nu", "ν", "nu"}, {"\\xi", "ξ", "xi"},
            {"\\pi", "π", "pi"}, {"\\varpi", "ϖ", "varpi"},
            {"\\rho", "ρ", "rho"}, {"\\varrho", "ϱ", "varrho"},
            {"\\sigma", "σ", "sigma"}, {"\\varsigma", "ς", "varsigma"},
            {"\\tau", "τ", "tau"}, {"\\upsilon", "υ", "upsilon"},
            {"\\phi", "φ", "phi"}, {"\\varphi", "ϕ", "varphi"},
            {"\\chi", "χ", "chi"}, {"\\psi", "ψ", "psi"},
            {"\\omega", "ω", "omega"},
        }),
        "Greek"
    );

    // Greek Uppercase
    tabWidget_->addTab(
        createSymbolGrid({
            {"\\Gamma", "Γ", "Gamma"}, {"\\Delta", "Δ", "Delta"},
            {"\\Theta", "Θ", "Theta"}, {"\\Lambda", "Λ", "Lambda"},
            {"\\Xi", "Ξ", "Xi"}, {"\\Pi", "Π", "Pi"},
            {"\\Sigma", "Σ", "Sigma"}, {"\\Upsilon", "Υ", "Upsilon"},
            {"\\Phi", "Φ", "Phi"}, {"\\Psi", "Ψ", "Psi"},
            {"\\Omega", "Ω", "Omega"},
        }),
        "Greek UC"
    );

    // Operators
    tabWidget_->addTab(
        createSymbolGrid({
            {"\\pm", "±", "plus-minus"}, {"\\mp", "∓", "minus-plus"},
            {"\\times", "×", "times"}, {"\\div", "÷", "divide"},
            {"\\cdot", "⋅", "center dot"}, {"\\star", "⋆", "star"},
            {"\\circ", "∘", "circle"}, {"\\bullet", "∙", "bullet"},
            {"\\oplus", "⊕", "oplus"}, {"\\ominus", "⊖", "ominus"},
            {"\\otimes", "⊗", "otimes"}, {"\\oslash", "⊘", "oslash"},
            {"\\odot", "⊙", "odot"}, {"\\dagger", "†", "dagger"},
            {"\\ddagger", "‡", "ddagger"}, {"\\cap", "∩", "intersection"},
            {"\\cup", "∪", "union"}, {"\\setminus", "∖", "set minus"},
            {"\\land", "∧", "logical and"}, {"\\lor", "∨", "logical or"},
            {"\\neg", "¬", "negation"}, {"\\lnot", "¬", "not"},
            {"\\wedge", "∧", "wedge"}, {"\\vee", "∨", "vee"},
            {"\\angle", "∠", "angle"}, {"\\nabla", "∇", "nabla"},
            {"\\partial", "∂", "partial"}, {"\\infty", "∞", "infinity"},
            {"\\forall", "∀", "for all"}, {"\\exists", "∃", "exists"},
        }),
        "Operators"
    );

    // Relations
    tabWidget_->addTab(
        createSymbolGrid({
            {"\\leq", "≤", "leq"}, {"\\geq", "≥", "geq"},
            {"\\neq", "≠", "neq"}, {"\\approx", "≈", "approx"},
            {"\\equiv", "≡", "equiv"}, {"\\sim", "∼", "sim"},
            {"\\simeq", "≃", "simeq"}, {"\\cong", "≅", "cong"},
            {"\\propto", "∝", "propto"}, {"\\ll", "≪", "much less"},
            {"\\gg", "≫", "much greater"}, {"\\subset", "⊂", "subset"},
            {"\\supset", "⊃", "superset"}, {"\\subseteq", "⊆", "subseteq"},
            {"\\supseteq", "⊇", "supseteq"}, {"\\in", "∈", "in"},
            {"\\notin", "∉", "not in"}, {"\\ni", "∋", "ni"},
            {"\\perp", "⊥", "perpendicular"}, {"\\parallel", "∥", "parallel"},
            {"\\mid", "|", "mid"}, {"\\vdash", "⊢", "vdash"},
            {"\\models", "⊨", "models"},
        }),
        "Relations"
    );

    // Arrows
    tabWidget_->addTab(
        createSymbolGrid({
            {"\\leftarrow", "←", "left arrow"}, {"\\rightarrow", "→", "right arrow"},
            {"\\leftrightarrow", "↔", "left-right arrow"},
            {"\\Leftarrow", "⇐", "Left arrow"}, {"\\Rightarrow", "⇒", "Right arrow"},
            {"\\Leftrightarrow", "⇔", "Left-Right arrow"},
            {"\\uparrow", "↑", "up arrow"}, {"\\downarrow", "↓", "down arrow"},
            {"\\Uparrow", "⇑", "Up arrow"}, {"\\Downarrow", "⇓", "Down arrow"},
            {"\\mapsto", "↦", "maps to"}, {"\\hookrightarrow", "↪", "hook right"},
            {"\\nearrow", "↗", "NE arrow"}, {"\\searrow", "↘", "SE arrow"},
            {"\\nwarrow", "↖", "NW arrow"}, {"\\swarrow", "↙", "SW arrow"},
            {"\\longrightarrow", "⟶", "long right arrow"},
            {"\\Longrightarrow", "⟹", "long Right arrow"},
        }),
        "Arrows"
    );

    // Accents and Misc
    tabWidget_->addTab(
        createSymbolGrid({
            {"\\hat{a}", "â", "hat"}, {"\\bar{a}", "ā", "bar"},
            {"\\tilde{a}", "ã", "tilde"}, {"\\vec{a}", "a⃗", "vector"},
            {"\\dot{a}", "ȧ", "dot"}, {"\\ddot{a}", "ä", "ddot"},
            {"\\breve{a}", "ă", "breve"}, {"\\check{a}", "ǎ", "check"},
            {"\\acute{a}", "á", "acute"}, {"\\grave{a}", "à", "grave"},
            {"\\overline{ab}", "ab̅", "overline"}, {"\\underline{ab}", "ab̲", "underline"},
            {"\\overrightarrow{ab}", "ab⃗", "over right arrow"},
            {"\\ldots", "...", "low dots"}, {"\\cdots", "⋯", "center dots"},
            {"\\vdots", "⋮", "vertical dots"}, {"\\ddots", "⋱", "diagonal dots"},
            {"\\sum", "∑", "summation"}, {"\\prod", "∏", "product"},
            {"\\int", "∫", "integral"}, {"\\oint", "∮", "contour integral"},
            {"\\sqrt{x}", "√x", "square root"},
        }),
        "Accents"
    );

    layout->addWidget(tabWidget_);
    setMaximumHeight(200);
}

QWidget* LatexSymbolPalette::createSymbolGrid(const QList<SymbolItem>& symbols) {
    auto* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto* container = new QWidget();
    int cols = 8;
    auto* grid = new QGridLayout(container);
    grid->setSpacing(2);
    grid->setContentsMargins(4, 4, 4, 4);

    for (int i = 0; i < symbols.size(); ++i) {
        auto* btn = createSymbolButton(symbols[i]);
        grid->addWidget(btn, i / cols, i % cols);
    }

    // Fill remaining cells in last row
    int lastRow = (symbols.size() - 1) / cols;
    for (int c = symbols.size() % cols; c < cols && c > 0; ++c) {
        grid->addItem(new QSpacerItem(30, 20), lastRow, c);
    }

    scroll->setWidget(container);
    return scroll;
}

QPushButton* LatexSymbolPalette::createSymbolButton(const SymbolItem& sym) {
    auto* btn = new QPushButton(sym.display);
    btn->setFixedSize(30, 30);
    btn->setToolTip(QString("%1 — %2").arg(sym.latex, sym.tooltip));
    btn->setStyleSheet(
        "QPushButton { border: 1px solid palette(mid); border-radius: 4px; "
        "font-size: 16px; background: palette(base); }"
        "QPushButton:hover { background: palette(highlight); color: palette(highlighted-text); }"
    );
    connect(btn, &QPushButton::clicked, this, [this, sym]() {
        emit symbolInsert(sym.latex);
    });
    return btn;
}
