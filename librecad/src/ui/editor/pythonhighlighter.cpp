// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifdef DEVELOPER

#include "pythonhighlighter.h"

//! [0]
PythonHighlighter::PythonHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;
    commentFormat.setForeground(QColor(137,136,135));

    // rule multiline comment '''
    triSingle.pattern = QRegularExpression(QStringLiteral("\'\'\'"));
    triSingle.nth = 1;
    triSingle.format = commentFormat;

    // rule multiline comment """
    triDouble.pattern = QRegularExpression(QStringLiteral("\"\"\""));
    triDouble.nth = 2;
    triDouble.format = commentFormat;

    // rule [0]
    symbolFormat.setForeground(QColor(202,96,202));
    rule.pattern = QRegularExpression(QStringLiteral("([-+<>=*/%&!|\\~^])"));
    rule.format = symbolFormat;
    highlightingRules.append(rule);

    // rule [1]
    numberFormat.setForeground(QColor(176,128,0));
    rule.pattern = QRegularExpression(QStringLiteral("\\b[+-]?[0-9]+[lL]?\\b|\\b[+-]?0[xX][0-9A-Fa-f]+[lL]?\\b|\\b[+-]?[0-9]+(?:\\.[0-9]+)?(?:[eE][+-]?[0-9]+)?\\b"));
    rule.format = numberFormat;
    highlightingRules.append(rule);

    // rule [2]
    expFormat.setForeground(QColor(0,110,40));
    rule.pattern = QRegularExpression(QStringLiteral("([A-Za-z]+(Error|Iteration))"));
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
        rule.pattern = QRegularExpression(pattern);
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
        QStringLiteral("\\braise\\b"),
        QStringLiteral("\\btry\\b"),
        QStringLiteral("\\byield\\b"),
    };

    statementFormat.setFontWeight(QFont::Bold);
    statementFormat.setForeground(QColor(31, 28, 27));
    for (const QString &pattern : keywordPatternsStatement) {
        rule.pattern = QRegularExpression(pattern);
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
        rule.pattern = QRegularExpression(pattern);
        rule.format = functionsFormat;
        highlightingRules.append(rule);
    }
// rule [6]
    const QString dcLkeywordPatternsFunctions[] = {
        QStringLiteral("\\b.actionTile\\b"),
        QStringLiteral("\\b.addList\\b"),
        QStringLiteral("\\b.command\\b"),
        QStringLiteral("\\b.dimxTile\\b"),
        QStringLiteral("\\b.dimyTile\\b"),
        QStringLiteral("\\b.doneDialog\\b"),
        QStringLiteral("\\b.endList\\b"),
        QStringLiteral("\\b.endImage\\b"),
        QStringLiteral("\\b.EvalSimpleString\\b"),
        QStringLiteral("\\b.EvalSimpleFile\\b"),
        QStringLiteral("\\b.fillImage\\b"),
        QStringLiteral("\\b.getContainer\\b"),
        QStringLiteral("\\b.getCorner\\b"),
        QStringLiteral("\\b.getDist\\b"),
        QStringLiteral("\\b.getDocument\\b"),
        QStringLiteral("\\b.getGraphic\\b"),
        QStringLiteral("\\b.getInt\\b"),
        QStringLiteral("\\b.getKword\\b"),
        QStringLiteral("\\b.getOrient\\b"),
        QStringLiteral("\\b.getPoint\\b"),
        QStringLiteral("\\b.getReal\\b"),
        QStringLiteral("\\b.getString\\b"),
        QStringLiteral("\\b.getTile\\b"),
        QStringLiteral("\\b.GetIntDialog\\b"),
        QStringLiteral("\\b.GetDoubleDialog\\b"),
        QStringLiteral("\\b.GetStringDialog\\b"),
        QStringLiteral("\\b.unloadDialog\\b"),
        QStringLiteral("\\b.initGet\\b"),
        QStringLiteral("\\b.loadDialog\\b"),
        QStringLiteral("\\b.modeTile\\b"),
        QStringLiteral("\\b.MessageBox\\b"),
        QStringLiteral("\\b.newDialog\\b"),
        QStringLiteral("\\b.OpenFileDialog\\b"),
        QStringLiteral("\\b.pixImage\\b"),
        QStringLiteral("\\b.prompt\\b"),
        QStringLiteral("\\b.ReadCharDialog\\b"),
        QStringLiteral("\\b.RunSimpleFile\\b"),
        QStringLiteral("\\b.RunSimpleString\\b"),
        QStringLiteral("\\b.setTile\\b"),
        QStringLiteral("\\b.startDialog\\b"),
        QStringLiteral("\\b.startImage\\b"),
        QStringLiteral("\\b.startList\\b"),
        QStringLiteral("\\b.textImage\\b"),
        QStringLiteral("\\b.vectorImage\\b"),
    };

    dclFormat.setForeground(QColor(0, 87, 174));
    dclFormat.setFontWeight(QFont::Bold);
    dclFormat.setFontItalic(true);

    for (const QString &pattern : dcLkeywordPatternsFunctions) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = dclFormat;
        highlightingRules.append(rule);
    }
// rule [7]
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

// rule [8]
    valuesFormat.setForeground(QColor(0, 87, 174));
    for (const QString &pattern : keywordPatternsBlue) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = valuesFormat;
        highlightingRules.append(rule);
    }

// rule [9]
    classValuesFormat.setForeground(QColor(146,76,157));
    rule.pattern = QRegularExpression(QStringLiteral("([_]{2}[a-z]+[_]{2})"));
    rule.format = classValuesFormat;
    highlightingRules.append(rule);

// rule [10]
    connectFormat.setFontWeight(QFont::Bold);
    connectFormat.setForeground(QColor(61,174,237)); //100,49,255
    rule.pattern = QRegularExpression(QStringLiteral("(connect)"));
    rule.format = connectFormat;
    highlightingRules.append(rule);

// rule [11]
    quoteFormat.setForeground(QColor(191,3,3));
    rule.pattern = QRegularExpression(QStringLiteral(R"**((?<!\\)([\"'])(.*?)(?<!\\)\1)**"));
    rule.format = quoteFormat;
    highlightingRules.append(rule);

// rule [12]
    rule.pattern = QRegularExpression(QStringLiteral("(\\\\\")|(\\\\\')|(\\\\n)"));
    rule.format = classValuesFormat;
    highlightingRules.append(rule);

// rule [13]
    placeholderFormat.setForeground(QColor(61,174,237));
    rule.pattern = QRegularExpression(QStringLiteral("(%)(\\d+d|d|\\d+i|i|o|u|\\d+[.]{1}\\d+x|\\d+x|x|\\d+[.]{1}\\d+X|\\d+X|X|\\d+[.]{1}\\d+e|e|\\d+[.]{1}\\d+E|E|\\d+[.]{1}\\d+f|F|\\d+[.]{1}\\d+g|g|\\d+[.]{1}\\d+G|G|c|s|r)"));
    rule.format = placeholderFormat;
    highlightingRules.append(rule);

// rule [14]
    bytesFormat.setForeground(QColor(146,76,157));
    rule.pattern = QRegularExpression(QStringLiteral("\\\\x\\d+"));
    rule.format = bytesFormat;
    highlightingRules.append(rule);

// rule [15]
    atFormat.setForeground(QColor(0, 87, 174));
    rule.pattern = QRegularExpression(QStringLiteral("@[^\'\"].*$"));
    rule.format = atFormat;
    highlightingRules.append(rule);

// rule [16]
    rule.pattern = QRegularExpression(QStringLiteral("[$](x|y|value|key|reason)"));
    rule.format = placeholderFormat;
    highlightingRules.append(rule);

// rule [17] - last befor multiline comment
    rule.pattern = QRegularExpression(QStringLiteral("^\\s*#.*$"));
    rule.format = commentFormat;
    highlightingRules.append(rule);
}

void PythonHighlighter::highlightBlock(const QString &text)
{
    for (const HighlightingRule &rule : std::as_const(highlightingRules))
    {
        QRegularExpression      expression  = rule.pattern;
        QRegularExpressionMatch match       = expression.match(text);
        int                     index       = match.capturedStart();
        QTextCharFormat         format      = rule.format;

        while (index >= 0)
        {
            int length = match.capturedLength();
            setFormat(index, length, format);
            match = expression.match(text, index + length);
            index = match.capturedStart();
        }
    }

    setCurrentBlockState(0);

    bool inMultiline = matchMultiline(text, triSingle.pattern, triSingle.nth, triSingle.format);
    if (!inMultiline)
    {
        // qDebug() << "[PythonHighlighter::highlightBlock] in Multiline";
        inMultiline = matchMultiline(text, triDouble.pattern, triDouble.nth , triDouble.format);
    }
}

bool PythonHighlighter::matchMultiline(const QString &text, const QRegularExpression& delimiter, int inState, const QTextCharFormat& format)
{
    int start = -1;
    int add = -1;
    QRegularExpressionMatch match;

    /* If inside triple-single quotes, start at 0 */
    if (previousBlockState() == inState)
    {
        start = 0;
        add = 0;
    }
    /* Otherwise, look for the delimiter on this line */
    else
    {
        match = delimiter.match(text);
        start = match.capturedStart();
        add = match.capturedLength();
    }

    // As long as there's a delimiter match on this line...
    while (start >= 0)
    {
        int length = -1;
        int end = -1;
        // Look for the ending delimiter
        match = delimiter.match(text, start + add);
        end = match.capturedStart();

        // Ending delimiter on this line?
        if (end >= add)
        {
            length = end - start + add + match.capturedLength();
            setCurrentBlockState(0);
        }
        // No; multi-line string
        else
        {
            setCurrentBlockState(inState);
            length = text.size() - start + add;
        }
        // Apply formatting
        setFormat(start, length, format);
        // Look for the next match
        match = delimiter.match(text, start + length);
        start = match.capturedStart();
    }

    // Return True if still inside a multi-line string, False otherwise
    if (currentBlockState() == inState)
    {
        return true;
    }
    else
    {
        return false;
    }
}

#endif // DEVELOPER
