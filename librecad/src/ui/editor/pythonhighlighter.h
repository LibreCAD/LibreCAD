// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef PYTHONHIGHLIGHTER_H
#define PYTHONHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegExp>

#ifdef DEVELOPER

QT_BEGIN_NAMESPACE
class QTextDocument;
QT_END_NAMESPACE

class PythonHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    PythonHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule
    {
        QRegExp expression;
        int nth = 0;
        QTextCharFormat format;
    };

    QList<int> tripleQuoteIndexes;
    QList<int> tripleQuoutesWithinStrings;

    QList<HighlightingRule> highlightingRules;

    QTextCharFormat atFormat;
    QTextCharFormat bytesFormat;
    QTextCharFormat classValuesFormat;
    QTextCharFormat commentFormat;
    QTextCharFormat connectFormat;
    QTextCharFormat expFormat;
    QTextCharFormat functionsFormat;
    QTextCharFormat moduleFormat;
    QTextCharFormat numberFormat;
    QTextCharFormat placeholderFormat;
    QTextCharFormat quoteFormat;
    QTextCharFormat statementFormat;
    QTextCharFormat symbolFormat;
    QTextCharFormat valuesFormat;

    HighlightingRule triSingle;
    HighlightingRule triDouble;

    bool matchMultiline(const QString& text, const QRegExp& delimiter, int inState, const QTextCharFormat& format);
};

#endif // DEVELOPER

#endif // HIGHLIGHTER_H
