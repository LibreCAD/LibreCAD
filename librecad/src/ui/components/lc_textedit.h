/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 Dongxu Li (github.com/dxli)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 * ********************************************************************************
 */

#ifndef LC_TEXTEDIT_H
#define LC_TEXTEDIT_H

#include <QFocusEvent>
#include <QKeyEvent>
#include <QMimeData>
#include <QSignalBlocker>
#include <QTextBlockFormat>
#include <QTextDocument>
#include <QTextEdit>
#include <QtMath>

namespace lc::textedit {

template <typename Editor>
void setDirection(Editor *editor, Qt::LayoutDirection direction) {
    const QSignalBlocker blocker(editor);
    editor->setLayoutDirection(direction);
    auto *document = editor->document();
    Qt::Alignment alignment = direction == Qt::RightToLeft ? Qt::AlignRight : Qt::AlignLeft;
    if (direction != Qt::LayoutDirectionAuto) {
        alignment |= Qt::AlignAbsolute;
    }
    auto option = document->defaultTextOption();
    option.setTextDirection(direction);
    option.setAlignment(alignment);
    if (document->defaultTextOption().textDirection() != direction ||
        document->defaultTextOption().alignment() != alignment) {
        document->setDefaultTextOption(option);
    }

    // Widget direction alone does not set the text's paragraph direction.
    QTextCursor cursor(document);
    cursor.beginEditBlock();
    do {
        auto format = cursor.blockFormat();
        if (format.layoutDirection() != direction || format.alignment() != alignment) {
            format.setLayoutDirection(direction);
            format.setAlignment(alignment);
            cursor.setBlockFormat(format);
        }
    } while (cursor.movePosition(QTextCursor::NextBlock));
    cursor.endEditBlock();
}

} // namespace lc::textedit

// QLineEdit has no public API for setting its text's paragraph base direction.
class LC_SingleLineTextEdit : public QTextEdit {
    Q_OBJECT
public:
    explicit LC_SingleLineTextEdit(QWidget *parent = nullptr) : QTextEdit(parent) {
        setAcceptRichText(false);
        setTabChangesFocus(true);
        setWordWrapMode(QTextOption::NoWrap);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    QSize sizeHint() const override {
        auto size = QTextEdit::sizeHint();
        size.setHeight(fontMetrics().height() + 2 * frameWidth() +
                       qCeil(2 * document()->documentMargin()));
        return size;
    }

    QSize minimumSizeHint() const override {
        return {QTextEdit::minimumSizeHint().width(), sizeHint().height()};
    }

    QString text() const { return toPlainText(); }
    void setText(const QString &value) { setPlainText(singleLine(value)); }
    void insert(const QString &value) { insertPlainText(singleLine(value)); }

signals:
    void editingFinished();

protected:
    void keyPressEvent(QKeyEvent *event) override {
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            emit editingFinished();
            event->ignore();
            return;
        }
        QTextEdit::keyPressEvent(event);
    }

    void focusOutEvent(QFocusEvent *event) override {
        QTextEdit::focusOutEvent(event);
        emit editingFinished();
    }

    void insertFromMimeData(const QMimeData *source) override {
        insert(source->text());
    }

private:
    static QString singleLine(QString value) {
        value.replace(QStringLiteral("\r\n"), QStringLiteral(" "));
        for (auto &ch : value) {
            if (ch == QLatin1Char('\r') || ch == QLatin1Char('\n') ||
                ch == QChar::LineSeparator || ch == QChar::ParagraphSeparator) {
                ch = QLatin1Char(' ');
            }
        }
        return value;
    }
};

#endif
