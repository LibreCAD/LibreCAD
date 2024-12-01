// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "pythonhighlighter.h"

#ifdef DEVELOPER

//! [0]
PythonHighlighter::PythonHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;
    commentFormat.setForeground(QColor(137,136,135));

// rule multiline comment '''
    triSingle.expression = QRegExp(QStringLiteral("\'\'\'"));
    triSingle.nth = 1;
    triSingle.format = commentFormat;

// rule multiline comment """
    triDouble.expression = QRegExp(QStringLiteral("\"\"\""));
    triDouble.nth = 2;
    triDouble.format = commentFormat;

// rule [0]
    symbolFormat.setForeground(QColor(202,96,202));
    rule.expression = QRegExp(QStringLiteral("([-+<>=*/%&!|\\~^])"));
    rule.format = symbolFormat;
    highlightingRules.append(rule);

// rule [1]
    numberFormat.setForeground(QColor(176,128,0));
    rule.expression = QRegExp(QStringLiteral("\\b[+-]?[0-9]+[lL]?\\b|\\b[+-]?0[xX][0-9A-Fa-f]+[lL]?\\b|\\b[+-]?[0-9]+(?:\\.[0-9]+)?(?:[eE][+-]?[0-9]+)?\\b"));
    rule.format = numberFormat;
    highlightingRules.append(rule);

// rule [2]
    expFormat.setForeground(QColor(0,110,40));
    rule.expression = QRegExp(QStringLiteral("([A-Za-z]+(Error|Iteration))"));
    rule.format = expFormat;
    highlightingRules.append(rule);

// rule [3]
    const QString keywordPatternsModule[] = {
        QStringLiteral("\\bimport\\b"),
        QStringLiteral("\\bas\\b"),
        QStringLiteral("\\bfrom\\b")
    };

    moduleFormat.setForeground(QColor(255, 85, 0));
    for (const QString &pattern : keywordPatternsModule) {
        rule.expression = QRegExp(pattern);
        rule.format = moduleFormat;
        highlightingRules.append(rule);
    }

// rule [4]
    const QString keywordPatternsStatement[] = {
        QStringLiteral("\\band\\b"),
        QStringLiteral("\\bassert\\b"),
        QStringLiteral("\\bbreak\\b"),
        QStringLiteral("\\bcase\\b"),
        QStringLiteral("\\bclass\\b"),
        QStringLiteral("\\bcontinue\\b"),
        QStringLiteral("\\bdef\\b"),
        QStringLiteral("\\bdel\\b"),
        QStringLiteral("\\belse\\b"),
        QStringLiteral("\\belif\\b"),
        QStringLiteral("\\bfinally\\b"),
        QStringLiteral("\\bglobal\\b"),
        QStringLiteral("\\bexcept\\b"),
        QStringLiteral("\\bfor\\b"),
        QStringLiteral("\\bif\\b"),
        QStringLiteral("\\bis\\b"),
        QStringLiteral("\\bin\\b"),
        QStringLiteral("\\bnot\\b"),
        QStringLiteral("\\bor\\b"),
        QStringLiteral("\\bpass\\b"),
        QStringLiteral("\\braise\\b"),
        QStringLiteral("\\brepeat\\b"),
        QStringLiteral("\\breturn\\b"),
        QStringLiteral("\\bsuper\\b"),
        QStringLiteral("\\bwith\\b"),
        QStringLiteral("\\bwhile\\b"),
        QStringLiteral("\\blambda\\b"),
        QStringLiteral("\\bmatch\\b"),
        QStringLiteral("\\btry\\b"),
        QStringLiteral("\\byield\\b"),
    };

    statementFormat.setFontWeight(QFont::Bold);
    statementFormat.setForeground(QColor(31, 28, 27));
    for (const QString &pattern : keywordPatternsStatement) {
        rule.expression = QRegExp(pattern);
        rule.format = statementFormat;
        highlightingRules.append(rule);
    }

// rule [5]
    const QString keywordPatternsFunctions[] = {
        QStringLiteral("\\b__import__\\b"),
        QStringLiteral("\\babs\\b"),
        QStringLiteral("\\bbytes\\b"),
        QStringLiteral("\\bbytearray\\b"),
        QStringLiteral("\\bbasestring\\b"),
        QStringLiteral("\\bbool\\b"),
        QStringLiteral("\\bcompile\\b"),
        QStringLiteral("\\bdivmod\\b"),
        QStringLiteral("\\benumerate\\b"),
        QStringLiteral("\\bexec\\b"),
        QStringLiteral("\\bfile\\b"),
        QStringLiteral("\\bfrozenset\\b"),
        QStringLiteral("\\bformat\\b"),
        QStringLiteral("\\beval\\b"),
        QStringLiteral("\\bglobals\\b"),
        QStringLiteral("\\bhasattr\\b"),
        QStringLiteral("\\bhash\\b"),
        QStringLiteral("\\bhex\\b"),
        QStringLiteral("\\binput\\b"),
        QStringLiteral("\\bint\\b"),
        QStringLiteral("\\biter\\b"),
        QStringLiteral("\\bfloat\\b"),
        QStringLiteral("\\bcomplex\\b"),
        QStringLiteral("\\blocals\\b"),
        QStringLiteral("\\bopen\\b"),
        QStringLiteral("\\bord\\b"),
        QStringLiteral("\\bmap\\b"),
        QStringLiteral("\\bmemoryview\\b"),
        QStringLiteral("\\bmin\\b"),
        QStringLiteral("\\bmax\\b"),
        QStringLiteral("\\bpow\\b"),
        QStringLiteral("\\bprint\\b"),
        QStringLiteral("\\bround\\b"),
        QStringLiteral("\\bset\\b"),
        QStringLiteral("\\bsorted\\b"),
        QStringLiteral("\\bsetattr\\b"),
        QStringLiteral("\\bstr\\b"),
        QStringLiteral("\\btype\\b"),
        QStringLiteral("\\btuple\\b"),
        QStringLiteral("\\bunicode\\b"),
        QStringLiteral("\\bdict\\b"),
        QStringLiteral("\\bisinstance\\b"),
        QStringLiteral("\\bid\\b"),
        QStringLiteral("\\blist\\b"),
        QStringLiteral("\\blen\\b"),
        QStringLiteral("\\bobject\\b"),
        QStringLiteral("\\brepr\\b"),
        QStringLiteral("\\breversed\\b"),
        QStringLiteral("\\bnext\\b"),
        QStringLiteral("\\bfile\\b"),
        QStringLiteral("\\brange\\b"),
        QStringLiteral("\\bslice\\b"),
        QStringLiteral("\\bbuffer\\b"),
        QStringLiteral("\\bproperty\\b"),
    };

    functionsFormat.setFontWeight(QFont::Bold);
    functionsFormat.setForeground(QColor(100,74,155));

    for (const QString &pattern : keywordPatternsFunctions) {
        rule.expression = QRegExp(pattern);
        rule.format = functionsFormat;
        highlightingRules.append(rule);
    }

// rule [6]
    const QString keywordPatternsBlue[] = {
        QStringLiteral("\\bNone\\b"),
        QStringLiteral("\\bFalse\\b"),
        QStringLiteral("\\bTrue\\b"),
        QStringLiteral("\\bself\\b"),
        QStringLiteral("\\b__debug__\\b"),
        QStringLiteral("\\b__name__\\b"),
        QStringLiteral("\\b__type_params__\\b"),
        QStringLiteral("\\b__value__\\b"),
    };

// rule [7]
    valuesFormat.setForeground(QColor(0, 87, 174));
    for (const QString &pattern : keywordPatternsBlue) {
        rule.expression = QRegExp(pattern);
        rule.format = valuesFormat;
        highlightingRules.append(rule);
    }

// rule [8]
    classValuesFormat.setForeground(QColor(146,76,157));
    rule.expression = QRegExp(QStringLiteral("([_]{2}[a-z]+[_]{2})"));
    rule.format = classValuesFormat;
    highlightingRules.append(rule);

// rule [9]
    connectFormat.setFontWeight(QFont::Bold);
    connectFormat.setForeground(QColor(61,174,237)); //100,49,255
    rule.expression = QRegExp(QStringLiteral("(connect)"));
    rule.format = connectFormat;
    highlightingRules.append(rule);

// rule [10]
    quoteFormat.setForeground(QColor(191,3,3));
    //rule.expression = QRegExp(QStringLiteral("\"([^\"]*)\"|\'([^\']*)\'"));
    rule.expression = QRegExp(QStringLiteral("\"((?:\\\"|[^\"])*)\"|\'((?:\\\'|[^\'])*)\'"));
    rule.format = quoteFormat;
    highlightingRules.append(rule);

// rule [11]
    rule.expression = QRegExp(QStringLiteral("(\\\\\")|(\\\\\')|(\\\\n)"));
    rule.format = classValuesFormat;
    highlightingRules.append(rule);

// rule [12]
    rule.expression = QRegExp(QStringLiteral("b\"([^\"]*)\"|b\'([^\']*)\'|r\"([^\"]*)\"|r\'([^\']*)\'"));
    rule.format = quoteFormat;
    highlightingRules.append(rule);

// rule [13]
    placeholderFormat.setForeground(QColor(61,174,237));
    rule.expression = QRegExp(QStringLiteral("(%)(\\d+d|d|\\d+i|i|o|u|\\d+[.]{1}\\d+x|\\d+x|x|\\d+[.]{1}\\d+X|\\d+X|X|\\d+[.]{1}\\d+e|e|\\d+[.]{1}\\d+E|E|\\d+[.]{1}\\d+f|F|\\d+[.]{1}\\d+g|g|\\d+[.]{1}\\d+G|G|c|s|r)"));
    rule.format = placeholderFormat;
    highlightingRules.append(rule);

// rule [14]
    bytesFormat.setForeground(QColor(146,76,157));
    rule.expression = QRegExp(QStringLiteral("\\\\x\\d+"));
    rule.format = bytesFormat;
    highlightingRules.append(rule);

// rule [15]
    atFormat.setForeground(QColor(0, 87, 174));
    rule.expression = QRegExp(QStringLiteral("@[^\'\"].*$"));
    rule.format = atFormat;
    highlightingRules.append(rule);

// rule [16]
    rule.expression = QRegExp(QStringLiteral("[$](x|y|value|key)"));
    rule.format = placeholderFormat;
    highlightingRules.append(rule);

// rule [17] - last befor multiline comment
    rule.expression = QRegExp(QStringLiteral("^\\s*#.*$"));
    rule.format = commentFormat;
    highlightingRules.append(rule);
}

void PythonHighlighter::highlightBlock(const QString &text)
{
    if (text == "") {
        return;
    }
    tripleQuoutesWithinStrings.clear();

    int index = 0;
    for (const HighlightingRule &rule : std::as_const(highlightingRules)) {
        QRegExp            expression = rule.expression;
        QTextCharFormat    format     = rule.format;
        int                nth        = rule.nth;

        index = expression.indexIn(text, 0);

        if (index >= 0 ) {
            if (QString("\"[^\"\\]*(\\.[^\"\\]*)*\"").contains(expression.pattern()) ||
                QString("\'[^\'\\]*(\\.[^'\\]*)*\'").contains(expression.pattern()))
            {
                int innerIndex = triSingle.expression.indexIn(text, index + 1);

                if (innerIndex == -1) {
                    innerIndex = triDouble.expression.indexIn(text, index + 1);
                }

                if (innerIndex != -1) {
                    for (int i = innerIndex; i >= innerIndex + 3; i++) {
                        tripleQuoteIndexes.push_back(i);
                    }
                    tripleQuoutesWithinStrings += tripleQuoteIndexes;
                }
            }
        }

        while (index >= 0) {
            for (auto& it : tripleQuoutesWithinStrings) {
                if (index == it) {
                    index++;
                    expression.indexIn(text, index);
                    continue;
                }
            }

            index = expression.pos(nth);
            int length = expression.cap(nth).size();
            setFormat(index, length, format);
            index = expression.indexIn(text, index + length);
        }

        setCurrentBlockState(0);
    }

    bool inMultiline = matchMultiline(text, triSingle.expression, triSingle.nth, triSingle.format);
    if (!inMultiline) {
        inMultiline = matchMultiline(text, triDouble.expression, triDouble.nth , triDouble.format);
    }

}

bool PythonHighlighter::matchMultiline(const QString &text, const QRegExp& delimiter, int inState, const QTextCharFormat& format)
{
    int start = 0;
    int end = 0;
    int add = 0;
    int length;

    if (previousBlockState() == inState) {
        start = 0;
        add = 0;
    }
    else {
        start = delimiter.indexIn(text);

        for (auto &it : tripleQuoutesWithinStrings) {
            if (start == it)  {
                return false;
            }
        }

        add = delimiter.matchedLength();
        setCurrentBlockState(0);
    }

    while (start >= 0) {
        end = delimiter.indexIn(text, start + add);
        if (end >= add) {
            length = end - start + add + delimiter.matchedLength();
            setCurrentBlockState(0);
        }
        else {
            setCurrentBlockState(inState);
            length = text.size() - start + add;
        }
        setFormat(start, length, format);
        start = delimiter.indexIn(text, start + length);
    }

    if (currentBlockState() == inState) {
        return true;
    }
    else {
        return false;
    }
}

#endif // DEVELOPER
