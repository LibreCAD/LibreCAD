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
#include <QTextDocument>
#include <QTextEdit>
#include <QBasicTimer>
#include <QtMath>
#include <map>
#include <memory>
#include <optional>

// Logical plain-text document with a display-only traditional Chinese RTL layout.
class LC_TextEdit : public QTextEdit {
    Q_OBJECT
public:
    explicit LC_TextEdit(QWidget *parent = nullptr);
    ~LC_TextEdit() override;
    void setTextDirection(Qt::LayoutDirection direction);
    QRect cursorRect(const QTextCursor &cursor) const;
    QRect cursorRect() const { return cursorRect(textCursor()); }
    QTextCursor cursorForPosition(const QPoint &position) const;
    QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;
    Q_INVOKABLE QVariant inputMethodQuery(Qt::InputMethodQuery query, QVariant argument) const;

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    bool viewportEvent(QEvent *event) override;
    void inputMethodEvent(QInputMethodEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;
    void timerEvent(QTimerEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    struct BlockLayout;
    const BlockLayout &layoutForBlock(const QTextBlock &block) const;
    QPointF blockOrigin(const QTextBlock &block) const;
    bool hasChineseRtl() const;
    void invalidateLayouts();
    int displayCursorPosition(const QTextCursor &cursor, const BlockLayout &view) const;
    void setDisplayCursor(QTextCursor cursor, int position, QTextCursor::MoveMode mode);
    void ensureDisplayCursorVisible();
    mutable std::map<int, std::unique_ptr<BlockLayout>> m_layouts;
    mutable bool m_directionDirty = true;
    mutable bool m_chineseRtl = false;
    int m_cursorBlock = -1;
    int m_displayCursor = -1;
    qreal m_verticalX = -1;
    QBasicTimer m_cursorTimer;
    QBasicTimer m_selectionScrollTimer;
    QPointF m_mousePosition;
    std::optional<QPointF> m_dropPosition;
    bool m_cursorVisible = true;
    int m_preeditCursor = 0;
    bool m_preeditCursorVisible = true;
};

// QLineEdit has no public API for setting its text's paragraph base direction.
class LC_SingleLineTextEdit : public LC_TextEdit {
    Q_OBJECT
public:
    explicit LC_SingleLineTextEdit(QWidget *parent = nullptr) : LC_TextEdit(parent) {
        setTabChangesFocus(true);
        // Keep viewport width for RTL alignment without wrapping.
        setWordWrapMode(QTextOption::NoWrap);
        setInputMethodHints(inputMethodHints() & ~Qt::ImhMultiLine);
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
    void inputMethodEvent(QInputMethodEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override {
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            emit editingFinished();
            event->ignore();
            return;
        }
        LC_TextEdit::keyPressEvent(event);
    }

    void focusOutEvent(QFocusEvent *event) override {
        LC_TextEdit::focusOutEvent(event);
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
