// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef PYTHONHIGHLIGHTER_H
#define PYTHONHIGHLIGHTER_H

#ifdef DEVELOPER

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>

QT_BEGIN_NAMESPACE
class QTextDocument;
QT_END_NAMESPACE

//! [0]
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
        int nth = 0;
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QList<HighlightingRule> highlightingRules;

    QRegularExpression commentStartExpression;
    QRegularExpression commentEndExpression;

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
    QTextCharFormat dclFormat;

    HighlightingRule triSingle;
    HighlightingRule triDouble;

    bool matchMultiline(const QString& text, const QRegularExpression& delimiter, int inState, const QTextCharFormat& format);
};

#endif // DEVELOPER

#endif // HIGHLIGHTER_H
