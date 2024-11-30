// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "lisphighlighter.h"

#ifdef DEVELOPER

//! [0]
LispHighlighter::LispHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

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
    keywordFormat.setForeground(QColor(31, 28, 27));
    keywordFormat.setFontWeight(QFont::Bold);
    const QString keywordPatterns[] = {
        QStringLiteral("\\baction_tile\\b"),
        QStringLiteral("\\badd_list\\b"),
        QStringLiteral("\\babs\\b"),
        QStringLiteral("\\baction_tile\\b"),
        QStringLiteral("\\band\\b"),
        QStringLiteral("\\balert\\b"),
        QStringLiteral("\\bappend\\b"),
        QStringLiteral("\\bapply\\b"),
        QStringLiteral("\\bascii\\b"),
        QStringLiteral("\\bassoc\\b"),
        QStringLiteral("\\batan\\b"),
        QStringLiteral("\\batof\\b"),
        QStringLiteral("\\batoi\\b"),
        QStringLiteral("\\batom\\b"),
        QStringLiteral("\\batom\\b([?])"),
        QStringLiteral("\\bboolean\\b([?])"),
        QStringLiteral("\\bbound\\b([?])"),
        QStringLiteral("\\bboundp\\b"),
        QStringLiteral("\\bcaddr\\b"),
        QStringLiteral("\\bcadr\\b"),
        QStringLiteral("\\bcar\\b"),
        QStringLiteral("\\bcdr\\b"),
        QStringLiteral("\\bchr\\b"),
        QStringLiteral("\\bclose\\b"),
        QStringLiteral("\\bcommand\\b"),
        QStringLiteral("\\bconcat\\b"),
        QStringLiteral("\\bcond\\b"),
        QStringLiteral("\\bconj\\b"),
        QStringLiteral("\\bcons\\b"),
        QStringLiteral("\\bcontains\\b([?])"),
        QStringLiteral("\\bcos\\b"),
        QStringLiteral("\\bcount\\b"),
        QStringLiteral("\\bdebug-eval\\b"),
        QStringLiteral("\\bdef\\b([!])"),
        QStringLiteral("\\bdefmacro\\b([!])"),
        QStringLiteral("\\bdefun\\b"),
        QStringLiteral("\\bderef\\b"),
        QStringLiteral("\\bdissoc\\b"),
        QStringLiteral("\\bdo\\b"),
        QStringLiteral("\\bdone_dialog\\b"),
        QStringLiteral("\\bdouble\\b([?])"),
        QStringLiteral("\\bempty\\b([?])"),
        QStringLiteral("\\bend_list\\b"),
        QStringLiteral("\\beval\\b"),
        QStringLiteral("\\bexit\\b"),
        QStringLiteral("\\bexp\\b"),
        QStringLiteral("\\bexpt\\b"),
        QStringLiteral("\\bfalse\\b([?])"),
        QStringLiteral("\\bfile\\b([?])"),
        QStringLiteral("\\bfirst\\b"),
        QStringLiteral("\\bfix\\b"),
        QStringLiteral("\\bfloat\\b"),
        QStringLiteral("\\bfn\\b([?])"),
        QStringLiteral("\\bfn*lambda\\b"),
        QStringLiteral("\\bforeach\\b"),
        QStringLiteral("\\bget\\b"),
        QStringLiteral("\\bget_tile\\b"),
        QStringLiteral("\\bgetenv\\b"),
        QStringLiteral("\\bgetint\\b"),
        QStringLiteral("\\bgetkword\\b"),
        QStringLiteral("\\bgetreal\\b"),
        QStringLiteral("\\bgetstring\\b"),
        QStringLiteral("\\bgetvar\\b"),
        QStringLiteral("\\bhash-map\\b"),
        QStringLiteral("\\bif\\b"),
        QStringLiteral("\\binitget\\b"),
        QStringLiteral("\\binteger\\b([?])"),
        QStringLiteral("\\bkeys\\b"),
        QStringLiteral("\\bkeyword\\b"),
        QStringLiteral("\\bkeyword\\b([?])"),
        QStringLiteral("\\blast\\b"),
        QStringLiteral("\\blength\\b"),
        QStringLiteral("\\blet*\\b"),
        QStringLiteral("\\blist\\b"),
        QStringLiteral("\\blist\\b([?])"),
        QStringLiteral("\\blistp\\b"),
        QStringLiteral("\\bload_dialog\\b"),
        QStringLiteral("\\bload-file\\b"),
        QStringLiteral("\\blog\\b"),
        QStringLiteral("\\blog10\\b"),
        QStringLiteral("\\blogand\\b"),
        QStringLiteral("\\bmacro\\b([?])"),
        QStringLiteral("\\bmap\\b"),
        QStringLiteral("\\bmap\\b([?])"),
        QStringLiteral("\\bmapcar\\b"),
        QStringLiteral("\\bmax\\b"),
        QStringLiteral("\\bmember\\b([?])"),
        QStringLiteral("\\bmeta\\b"),
        QStringLiteral("\\bmin\\b"),
        QStringLiteral("\\bminus\\b([?])"),
        QStringLiteral("\\bminusp\\b"),
        QStringLiteral("\\bmode_tile\\b"),
        QStringLiteral("\\bnew_dialog\\b"),
        QStringLiteral("\\bnil\\b"),
        QStringLiteral("\\bnil\\b([?])"),
        QStringLiteral("\\bnot\\b"),
        QStringLiteral("\\bnth\\b"),
        QStringLiteral("\\bnull\\b"),
        QStringLiteral("\\bnumber\\b([?])"),
        QStringLiteral("\\bnumberp\\b"),
        QStringLiteral("\\bopen\\b"),
        QStringLiteral("\\bor\\b"),
        QStringLiteral("\\bpolar\\b"),
        QStringLiteral("\\bprin1\\b"),
        QStringLiteral("\\bprinc\\b"),
        QStringLiteral("\\bprint\\b"),
        QStringLiteral("\\bprintln\\b"),
        QStringLiteral("\\bprn\\b"),
        QStringLiteral("\\bprogn\\b"),
        QStringLiteral("\\bprompt\\b"),
        QStringLiteral("\\bpr-str\\b"),
        QStringLiteral("\\bquasiquote\\b"),
        QStringLiteral("\\bquote\\b"),
        QStringLiteral("\\bread-char\\b"),
        QStringLiteral("\\bread-line\\b"),
        QStringLiteral("\\breadline\\b"),
        QStringLiteral("\\bread-string\\b"),
        QStringLiteral("\\brem\\b"),
        QStringLiteral("\\brepeat\\b"),
        QStringLiteral("\\breset\\b([!])"),
        QStringLiteral("\\brest\\b"),
        QStringLiteral("\\breverse\\b"),
        QStringLiteral("\\bseq\\b"),
        QStringLiteral("\\bsequential\\b([?])"),
        QStringLiteral("\\bset\\b"),
        QStringLiteral("\\bset_tile\\b"),
        QStringLiteral("\\bsetq\\b"),
        QStringLiteral("\\bsetvar\\b"),
        QStringLiteral("\\bsin\\b"),
        QStringLiteral("\\bslurp\\b"),
        QStringLiteral("\\bsqrt\\b"),
        QStringLiteral("\\bstart_dialog\\b"),
        QStringLiteral("\\bstart_list\\b"),
        QStringLiteral("\\bstartapp\\b"),
        QStringLiteral("\\bstr\\b"),
        QStringLiteral("\\bstrcase\\b"),
        QStringLiteral("\\bstrcat\\b"),
        QStringLiteral("\\bstring\\b([?])"),
        QStringLiteral("\\bstrlen\\b"),
        QStringLiteral("\\bsubst\\b"),
        QStringLiteral("\\bsubstr\\b"),
        QStringLiteral("\\bswap\\b([!])"),
        QStringLiteral("\\bsymbol\\b"),
        QStringLiteral("\\bsymbol\\b"),
        QStringLiteral("\\bsymbol\\b([?])"),
        QStringLiteral("\\bT\\b"),
        QStringLiteral("\\btan\\b"),
        QStringLiteral("\\bterpri\\b"),
        QStringLiteral("\\bthrow\\b"),
        QStringLiteral("\\btime-ms\\b"),
        QStringLiteral("\\btrace\\b"),
        QStringLiteral("\\btrue\\b([?])"),
        QStringLiteral("\\btry*\\b"),
        QStringLiteral("\\btype\\b"),
        QStringLiteral("\\btype\\b([?])"),
        QStringLiteral("\\bunload_dialog\\b"),
        QStringLiteral("\\buntrace\\b"),
        QStringLiteral("\\bvals\\b"),
        QStringLiteral("\\bvec\\b"),
        QStringLiteral("\\bvector\\b"),
        QStringLiteral("\\bvector\\b([?])"),
        QStringLiteral("\\bver\\b"),
        QStringLiteral("\\bvl-consp\\b"),
        QStringLiteral("\\bvl-directory-files\\b"),
        QStringLiteral("\\bvl-file-copy\\b"),
        QStringLiteral("\\bvl-file-delete\\b"),
        QStringLiteral("\\bvl-file-directory-p\\b"),
        QStringLiteral("\\bvl-filename-base\\b"),
        QStringLiteral("\\bvl-filename-directory\\b"),
        QStringLiteral("\\bvl-filename-extension\\b"),
        QStringLiteral("\\bvl-filename-mktemp\\b"),
        QStringLiteral("\\bvl-file-rename\\b"),
        QStringLiteral("\\bvl-file-size\\b"),
        QStringLiteral("\\bvl-file-systime\\b"),
        QStringLiteral("\\bvl-mkdir\\b"),
        QStringLiteral("\\bvl-position\\b"),
        QStringLiteral("\\bwcmatch\\b"),
        QStringLiteral("\\bwhile\\b"),
        QStringLiteral("\\bwith-meta\\b"),
        QStringLiteral("\\bwrite-char\\b"),
        QStringLiteral("\\bwrite-line\\b"),
        QStringLiteral("\\bzero\\b([?])"),
        QStringLiteral("\\bzerop\\b"),
    };
    for (const QString &pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

//! [3]
    bracketFormat.setForeground(QColor(0, 0, 255));
    bracketFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression(QStringLiteral("[()]"));
    rule.format = bracketFormat;
    highlightingRules.append(rule);

//! [4]
    quotationFormat.setForeground(QColor(191,3,3));
    rule.pattern = QRegularExpression(QStringLiteral("\"((?:\\\"|[^\"])*)\""));
    rule.format = quotationFormat;
    highlightingRules.append(rule);

//! [5]
    innerQuotationFormat.setForeground(QColor(146,76,157));
    rule.pattern = QRegularExpression(QStringLiteral("(\\\\\")|(\\\\\')"));
    rule.format = innerQuotationFormat;
    highlightingRules.append(rule);

//! [6]
    placeholderFormat.setForeground(QColor(61,174,237));
    rule.pattern = QRegularExpression(QStringLiteral("[$](x|y|value|key)"));
    rule.format = placeholderFormat;
    highlightingRules.append(rule);

//! [7]
    singleLineCommentFormat.setForeground(QColor(137,136,135));
    rule.pattern = QRegularExpression(QStringLiteral(";(?=[^\"]*(?:(?:\"[^\"]*){2})*$).*"));
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);
}

void LispHighlighter::highlightBlock(const QString &text)
{
    for (const HighlightingRule &rule : std::as_const(highlightingRules)) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    setCurrentBlockState(0);
}

#endif // DEVELOPER
