#include "LatexWordCounter.hpp"
#include <QVBoxLayout>
#include <QRegularExpression>
#include <QFrame>

LatexWordCounter::LatexWordCounter(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void LatexWordCounter::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    auto titleStyle = "font-weight: bold; font-size: 13px; color: palette(text);";
    auto labelStyle = "font-size: 12px; color: palette(text);";
    auto valueStyle = "font-size: 12px; font-weight: bold; color: palette(text);";

    auto* titleLabel = new QLabel("Document Statistics");
    titleLabel->setStyleSheet(titleStyle);
    layout->addWidget(titleLabel);

    // Separator
    auto* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("color: palette(mid);");
    layout->addWidget(line);

    auto addStat = [&](const QString& name, QLabel*& label) {
        auto* row = new QHBoxLayout();
        auto* nameLabel = new QLabel(name);
        nameLabel->setStyleSheet(labelStyle);
        row->addWidget(nameLabel);
        row->addStretch();
        label = new QLabel("0");
        label->setStyleSheet(valueStyle);
        row->addWidget(label);
        layout->addLayout(row);
    };

    addStat("Words (plain text):", wordsLabel_);
    addStat("Characters:", charsLabel_);
    addStat("Chars (no commands):", charsNoCmdLabel_);
    addStat("Lines:", linesLabel_);
    addStat("Sections:", sectionsLabel_);
    addStat("Equations:", equationsLabel_);
    addStat("Environments:", environmentsLabel_);
    addStat("Bibliography entries:", bibliographyLabel_);

    layout->addStretch();
    setMinimumWidth(200);
    setMaximumWidth(250);
}

void LatexWordCounter::updateCount(const QString& latexContent) {
    // Total characters
    int totalChars = latexContent.size();
    charsLabel_->setText(QString::number(totalChars));

    // Lines
    int lines = latexContent.count('\n') + 1;
    linesLabel_->setText(QString::number(lines));

    // Strip LaTeX commands to get plain text
    QString plain = latexContent;

    // Remove comments
    plain.replace(QRegularExpression("%[^\n]*"), "");

    // Remove environments: \begin{...} and \end{...}
    plain.replace(QRegularExpression("\\\\begin\\{[^}]*\\}"), " ");
    plain.replace(QRegularExpression("\\\\end\\{[^}]*\\}"), " ");

    // Remove \command[opt]{arg} patterns — replace with arg content
    plain.replace(QRegularExpression("\\\\[a-zA-Z]+\\*?(?:\\[[^\\]]*\\])?"), " ");

    // Remove remaining backslash commands
    plain.replace(QRegularExpression("\\\\[^a-zA-Z]"), " ");

    // Remove braces
    plain.replace("{", " ");
    plain.replace("}", " ");

    // Remove math delimiters
    plain.replace("$$", " ");
    plain.replace("$", " ");

    // Remove special chars
    plain.replace(QRegularExpression("[~&_^]"), " ");

    // Words
    QStringList wordList = plain.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    wordsLabel_->setText(QString::number(wordList.size()));

    // Characters without commands
    QString cleaned = plain.simplified();
    charsNoCmdLabel_->setText(QString::number(cleaned.size()));

    // Sections count
    QRegularExpression sectionRe("\\\\(?:part|chapter|section|subsection|subsubsection|paragraph)\\*?\\{");
    auto sectionMatch = sectionRe.globalMatch(latexContent);
    int sectionCount = 0;
    while (sectionMatch.hasNext()) { sectionMatch.next(); sectionCount++; }
    sectionsLabel_->setText(QString::number(sectionCount));

    // Equations
    QRegularExpression eqRe("\\\\begin\\{equation|align|gather|multline|eqnarray");
    auto eqMatch = eqRe.globalMatch(latexContent);
    int eqCount = 0;
    while (eqMatch.hasNext()) { eqMatch.next(); eqCount++; }
    // Also count $$...$$ blocks
    QRegularExpression displayMathRe("\\$\\$");
    auto dmMatch = displayMathRe.globalMatch(latexContent);
    int dmCount = 0;
    while (dmMatch.hasNext()) { dmMatch.next(); dmCount++; }
    eqCount += dmCount / 2;
    equationsLabel_->setText(QString::number(eqCount));

    // Environments
    QRegularExpression envRe("\\\\begin\\{");
    auto envMatch = envRe.globalMatch(latexContent);
    int envCount = 0;
    while (envMatch.hasNext()) { envMatch.next(); envCount++; }
    environmentsLabel_->setText(QString::number(envCount));

    // Bibliography entries
    QRegularExpression bibRe("@\\w+\\{");
    auto bibMatch = bibRe.globalMatch(latexContent);
    int bibCount = 0;
    while (bibMatch.hasNext()) { bibMatch.next(); bibCount++; }
    bibliographyLabel_->setText(QString::number(bibCount));
}

void LatexWordCounter::setDarkMode(bool dark) {
    darkMode_ = dark;
}
