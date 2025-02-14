// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#ifdef DEVELOPER

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>

QT_BEGIN_NAMESPACE
class QTextDocument;
QT_END_NAMESPACE

//! [0]
class DclHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    DclHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QList<HighlightingRule> highlightingRules;

    QRegularExpression commentStartExpression;
    QRegularExpression commentEndExpression;

    QTextCharFormat symbolFormat;
    QTextCharFormat numberFormat;
    QTextCharFormat keywordFormat;
    QTextCharFormat arttibuteFormat;
    QTextCharFormat valueFormat;
    QTextCharFormat commentFormat;
    QTextCharFormat quotationFormat;
    QTextCharFormat bracketFormat;
};
//! [0]

#endif // DEVELOPER

#endif // HIGHLIGHTER_H
