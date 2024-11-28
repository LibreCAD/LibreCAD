// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef LISPHIGHLIGHTER_H
#define LISPHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>

#ifdef DEVELOPER

QT_BEGIN_NAMESPACE
class QTextDocument;
QT_END_NAMESPACE

class LispHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    LispHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QList<HighlightingRule> highlightingRules;

    QTextCharFormat symbolFormat;
    QTextCharFormat numberFormat;
    QTextCharFormat keywordFormat;
    QTextCharFormat bracketFormat;
    QTextCharFormat singleLineCommentFormat;
    QTextCharFormat quotationFormat;
};

#endif // DEVELOPER

#endif // LISPHIGHLIGHTER_H
