// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause


#ifdef DEVELOPER

#include "dclhighlighter.h"

//! [0]
DclHighlighter::DclHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

//! [0]
    symbolFormat.setForeground(QColor(202,96,202));
    rule.pattern = QRegularExpression(QStringLiteral("([-+<>=*/%&!|\\~^;])"));
    rule.format = symbolFormat;
    highlightingRules.append(rule);

//! [1]
    numberFormat.setForeground(QColor(176,128,0));
    rule.pattern = QRegularExpression(QStringLiteral("\\b[+-]?[0-9]+[lL]?\\b|\\b[+-]?0[xX][0-9A-Fa-f]+[lL]?\\b|\\b[+-]?[0-9]+(?:\\.[0-9]+)?(?:[eE][+-]?[0-9]+)?\\b"));
    rule.format = numberFormat;
    highlightingRules.append(rule);

    keywordFormat.setForeground(QColor(31, 28, 27));
    keywordFormat.setFontWeight(QFont::Bold);
    const QString keywordPatterns[] = {
        QStringLiteral("[:]"),
        QStringLiteral("\\bboxed_column\\b"),
        QStringLiteral("\\bboxed_radio_column\\b"),
        QStringLiteral("\\bboxed_radio_row\\b"),
        QStringLiteral("\\bboxed_row\\b"),
        QStringLiteral("\\bbutton\\b"),
        QStringLiteral("\\bcolumn\\b"),
        QStringLiteral("\\bconcatenation\\b"),
        QStringLiteral("\\bdial\\b"),
        QStringLiteral("\\bdialog\\b"),
        QStringLiteral("\\bedit_box\\b"),
        QStringLiteral("\\berrtile\\b"),
        QStringLiteral("\\bimage\\b"),
        QStringLiteral("\\bimage_button\\b"),
        QStringLiteral("\\blist_box\\b"),
        QStringLiteral("\\bok_cancel\\b"),
        QStringLiteral("\\bok_cancel_help\\b"),
        QStringLiteral("\\bok_cancel_help_errtile\\b"),
        QStringLiteral("\\bok_cancel_help_info\\b"),
        QStringLiteral("\\bok_only\\b"),
        QStringLiteral("\\bparagraph\\b"),
        QStringLiteral("\\bpopup_list\\b"),
        QStringLiteral("\\bregister\\b"),
        QStringLiteral("\\bradio_button\\b"),
        QStringLiteral("\\bradio_column\\b"),
        QStringLiteral("\\bradio_row\\b"),
        QStringLiteral("\\brow\\b"),
        QStringLiteral("\\bscroll\\b"),
        QStringLiteral("\\bslider\\b"),
        QStringLiteral("\\bspacer\\b"),
        QStringLiteral("\\bspacer_0\\b"),
        QStringLiteral("\\bspacer_1\\b"),
        QStringLiteral("\\btab\\b"),
        QStringLiteral("\\btext\\b"),
        QStringLiteral("\\btext_part\\b"),
        QStringLiteral("\\btoggle\\b"),
    };

//! [2]
    for (const QString &pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    arttibuteFormat.setForeground(QColor(146,76,157));
    const QString arttibutePatterns[] = {
        QStringLiteral("\\baction\\b"),
        QStringLiteral("\\balignment\\b"),
        QStringLiteral("\\ballow_accept\\b"),
        QStringLiteral("\\baspect_ratio\\b"),
        QStringLiteral("\\bbig_increment\\b"),
        QStringLiteral("\\bchildren_alignment\\b"),
        QStringLiteral("\\bchildren_fixed_height\\b"),
        QStringLiteral("\\bchildren_fixed_width\\b"),
        QStringLiteral("\\bcolor\\b"),
        QStringLiteral("\\bedit_limit\\b"),
        QStringLiteral("\\bedit_width\\b"),
        QStringLiteral("\\bfixed_height\\b"),
        QStringLiteral("\\bfixed_width\\b"),
        QStringLiteral("\\bfixed_width_font\\b"),
        QStringLiteral("\\bheight\\b"),
        QStringLiteral("\\binitial_focus\\b"),
        QStringLiteral("\\bis_bold\\b"),
        QStringLiteral("\\bis_cancel\\b"),
        QStringLiteral("\\bis_default\\b"),
        QStringLiteral("\\bis_enabled\\b"),
        QStringLiteral("\\bis_tab_stop\\b"),
        QStringLiteral("\\bkey\\b"),
        QStringLiteral("\\blabel\\b"),
        QStringLiteral("\\blayout\\b"),
        QStringLiteral("\\blist\\b"),
        QStringLiteral("\\bmax_value\\b"),
        QStringLiteral("\\bmin_value\\b"),
        QStringLiteral("\\bmnemonic\\b"),
        QStringLiteral("\\bmultiple_select\\b"),
        QStringLiteral("\\bpassword_char\\b"),
        QStringLiteral("\\bsmall_increment\\b"),
        QStringLiteral("\\btabs\\b"),
        QStringLiteral("\\btab_truncate\\b"),
        QStringLiteral("\\bvalue\\b"),
        QStringLiteral("\\bwidth\\b"),
    };

//! [3]
    for (const QString &pattern : arttibutePatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = arttibuteFormat;
        highlightingRules.append(rule);
    }

    valueFormat.setForeground(QColor(0, 87, 174));
    const QString valuePatterns[] = {
        QStringLiteral("\\bleft\\b"),
        QStringLiteral("\\bright\\b"),
        QStringLiteral("\\btop\\b"),
        QStringLiteral("\\bbottom\\b"),
        QStringLiteral("\\bcentered\\b"),
        QStringLiteral("\\bhorizontal\\b"),
        QStringLiteral("\\bvertical\\b"),
        QStringLiteral("\\bdialog_line\\b"),
        QStringLiteral("\\bdialog_foreground\\b"),
        QStringLiteral("\\bdialog_background\\b"),
        QStringLiteral("\\bfalse\\b"),
        QStringLiteral("\\bgraphics_background\\b"),
        QStringLiteral("\\bblack\\b"),
        QStringLiteral("\\bred\\b"),
        QStringLiteral("\\byellow\\b"),
        QStringLiteral("\\bgreen\\b"),
        QStringLiteral("\\bcyan\\b"),
        QStringLiteral("\\bblue\\b"),
        QStringLiteral("\\bmagenta\\b"),
        QStringLiteral("\\btrue\\b"),
        QStringLiteral("\\bwhite\\b"),
        QStringLiteral("\\bgraphics_foreground\\b")
    };

//! [4]
    for (const QString &pattern : valuePatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = valueFormat;
        highlightingRules.append(rule);
    }

//! [5]
    commentFormat.setForeground(QColor(137,136,135));
    rule.pattern = QRegularExpression(QStringLiteral("//[^\n]*"));
    rule.format = commentFormat;
    highlightingRules.append(rule);

//! [6]
    quotationFormat.setForeground(QColor(191,3,3));
    rule.pattern = QRegularExpression(QStringLiteral(R"**((?<!\\)([\"'])(.+?)(?<!\\)\1)**"));
    rule.format = quotationFormat;
    highlightingRules.append(rule);

//! [7]
    bracketFormat.setForeground(QColor(0, 87, 174));
    bracketFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression(QStringLiteral("[{}]"));
    rule.format = bracketFormat;
    highlightingRules.append(rule);

//! [8]
    commentStartExpression = QRegularExpression(QStringLiteral("/\\*"));
    commentEndExpression = QRegularExpression(QStringLiteral("\\*/"));
}

void DclHighlighter::highlightBlock(const QString &text)
{
    for (const HighlightingRule &rule : std::as_const(highlightingRules)) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 1)
        startIndex = text.indexOf(commentStartExpression);

    while (startIndex >= 0) {
        QRegularExpressionMatch match = commentEndExpression.match(text, startIndex);
        int endIndex = match.capturedStart();
        int commentLength = 0;
        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex
                            + match.capturedLength();
        }
        setFormat(startIndex, commentLength, commentFormat);
        startIndex = text.indexOf(commentStartExpression, startIndex + commentLength);
    }
}

#endif // DEVELOPER
