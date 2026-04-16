#ifndef HIGHLIGHTING_H
#define HIGHLIGHTING_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QVector>
#include <QStringList>
#include <QColor>

// C++ / Arduino Syntax Highlighter
class CppHighlighter : public QSyntaxHighlighter {
public:
    explicit CppHighlighter(QTextDocument *parent = nullptr)
        : QSyntaxHighlighter(parent)
    {
        initFormats();
        initRules();
    }

protected:
    void highlightBlock(const QString &text) override {
        for (const HighlightRule &rule : rules) {
            QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
            while (it.hasNext()) {
                QRegularExpressionMatch match = it.next();
                setFormat(match.capturedStart(), match.capturedLength(), rule.format);
            }
        }
    }

private:
    struct HighlightRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    QVector<HighlightRule> rules;

    // Predefined text formats
    QTextCharFormat keywordFormat;
    QTextCharFormat typeFormat;
    QTextCharFormat stringFormat;
    QTextCharFormat commentFormat;
    QTextCharFormat numberFormat;
    QTextCharFormat preprocessorFormat;
    QTextCharFormat functionFormat;
    QTextCharFormat arduinoFormat;

    void initFormats() {
        keywordFormat.setForeground(QColor("#569CD6"));
        keywordFormat.setFontWeight(QFont::Bold);

        typeFormat.setForeground(QColor("#4EC9B0"));

        stringFormat.setForeground(QColor("#CE9178"));

        commentFormat.setForeground(QColor("#6A9955"));

        numberFormat.setForeground(QColor("#B5CEA8"));

        preprocessorFormat.setForeground(QColor("#C586C0"));

        functionFormat.setForeground(QColor("#DCDCAA"));

        arduinoFormat.setForeground(QColor("#C586C0"));
        arduinoFormat.setFontWeight(QFont::Bold);
    }

    void initRules() {
        // Keywords
        QStringList keywords = {
            "if","else","while","for","return",
            "class","struct","public","private","protected",
            "virtual","override","const","static","namespace",
            "using","switch","case","break","continue","default"
        };
        for (const QString &kw : keywords)
            addRule("\\b" + kw + "\\b", keywordFormat);

        // Types
        QStringList types = {
            "int","float","double","char","void","bool",
            "long","short","auto","size_t","std","string"
        };
        for (const QString &t : types)
            addRule("\\b" + t + "\\b", typeFormat);

        // Arduino functions
        QStringList arduinoFunctions = {
            "pinMode", "digitalWrite", "digitalRead",
            "analogRead", "analogWrite",
            "delay", "delayMicroseconds",
            "millis", "micros",
            "Serial", "begin", "print", "println",
            "setup", "loop"
        };
        for (const QString &fn : arduinoFunctions)
            addRule("\\b" + fn + "\\b", arduinoFormat);

        // Strings
        addRule("\".*?\"", stringFormat);
        addRule("'.*?'", stringFormat); // single-quoted characters

        // Single-line comments
        addRule("//[^\n]*", commentFormat);

        // Numbers
        addRule("\\b[0-9]+\\b", numberFormat);

        // Preprocessor
        addRule("^#.*", preprocessorFormat);

        // Functions (generic)
        addRule("\\b[A-Za-z_][A-Za-z0-9_]*(?=\\()", functionFormat);
    }

    void addRule(const QString &pattern, const QTextCharFormat &format) {
        HighlightRule rule;
        rule.pattern = QRegularExpression(pattern);
        rule.format = format;
        rules.append(rule);
    }
};

#endif // HIGHLIGHTING_H