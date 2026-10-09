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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 * ********************************************************************************
 */

#include "lc_textedit.h"
#include "lc_textbidi.h"

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QDropEvent>
#include <QInputMethod>
#include <QInputMethodEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QStyleOption>
#include <QTextBlock>
#include <QTextLayout>
#include <QTimer>
#include <algorithm>
#include <limits>

struct LC_TextEdit::BlockLayout {
    lc::textbidi::DirectionalText display;
    QTextLayout layout;
    int preeditPosition = -1;
    int preeditLength = 0;
    qreal lineWidth = 0;
    int lineCount = 0;

    int displayPosition(int position) const {
        if (preeditPosition >= 0 && position > preeditPosition) position += preeditLength;
        return display.displayPositions.at(std::clamp(position, 0,
                                            int(display.displayPositions.size()) - 1));
    }

    int documentPosition(int position) const {
        position = display.sourcePosition(position);
        if (preeditPosition >= 0 && position > preeditPosition)
            position = std::max(preeditPosition, position - preeditLength);
        return position;
    }

    int hitTest(const QTextLine &line, qreal x) const {
        int nearest = line.textStart();
        qreal distance = std::numeric_limits<qreal>::max();
        for (int position = line.textStart(); position <= line.textStart() + line.textLength(); ++position) {
            if (!layout.isValidCursorPosition(position)) continue;
            const qreal delta = qAbs(line.cursorToX(position) - x);
            if (delta < distance ||
                (qFuzzyCompare(delta + 1, distance + 1) &&
                 position == displayPosition(documentPosition(position)))) {
                distance = delta;
                nearest = position;
            }
        }
        return nearest;
    }

    QTextLine lineAtY(qreal y) const {
        int index = 0;
        while (index + 1 < layout.lineCount() &&
               y >= layout.lineAt(index).y() + layout.lineAt(index).height()) ++index;
        return layout.lineCount() ? layout.lineAt(index) : QTextLine();
    }
};

LC_TextEdit::LC_TextEdit(QWidget *parent) : QTextEdit(parent) {
    setAcceptRichText(false);
    connect(document(), &QTextDocument::contentsChanged, this, &LC_TextEdit::invalidateLayouts);
    connect(this, &QTextEdit::selectionChanged, viewport(), qOverload<>(&QWidget::update));
    connect(this, &QTextEdit::cursorPositionChanged, this, [this] {
        m_cursorBlock = m_displayCursor = -1;
        m_verticalX = -1;
        m_cursorVisible = true;
        viewport()->update();
        QTimer::singleShot(0, this, &LC_TextEdit::ensureDisplayCursorVisible);
    });
    if (QApplication::cursorFlashTime() > 0)
        m_cursorTimer.start(QApplication::cursorFlashTime() / 2, this);
}

LC_TextEdit::~LC_TextEdit() = default;

void LC_TextEdit::invalidateLayouts() {
    m_layouts.clear();
    m_directionDirty = true;
    m_cursorBlock = m_displayCursor = -1;
    viewport()->update();
    QTimer::singleShot(0, this, &LC_TextEdit::ensureDisplayCursorVisible);
}

void LC_TextEdit::setTextDirection(Qt::LayoutDirection direction) {
    const QSignalBlocker blocker(this);
    setLayoutDirection(direction);
    auto option = document()->defaultTextOption();
    option.setTextDirection(direction);
    option.setAlignment(direction == Qt::RightToLeft ? Qt::AlignRight | Qt::AlignAbsolute
                        : direction == Qt::LeftToRight ? Qt::AlignLeft | Qt::AlignAbsolute
                                                     : Qt::AlignLeft);
    if (option.textDirection() != document()->defaultTextOption().textDirection() ||
        option.alignment() != document()->defaultTextOption().alignment()) {
        document()->setDefaultTextOption(option);
        invalidateLayouts();
    }
}

bool LC_TextEdit::hasChineseRtl() const {
    if (!m_directionDirty) return m_chineseRtl;
    m_directionDirty = false;
    m_chineseRtl = false;
    for (auto block = document()->begin(); block.isValid(); block = block.next()) {
        if (block.textDirection() != Qt::RightToLeft) continue;
        const QString text = block.text() + block.layout()->preeditAreaText();
        for (char32_t scalar : text.toUcs4()) {
            if (QChar::script(scalar) == QChar::Script_Han) return m_chineseRtl = true;
        }
    }
    return false;
}

QPointF LC_TextEdit::blockOrigin(const QTextBlock &block) const {
    document()->documentLayout()->blockBoundingRect(block);
    const auto *horizontal = horizontalScrollBar();
    const int x = isRightToLeft() ? horizontal->maximum() - horizontal->value() : horizontal->value();
    return block.layout()->position() - QPointF(x, verticalScrollBar()->value());
}

const LC_TextEdit::BlockLayout &LC_TextEdit::layoutForBlock(const QTextBlock &block) const {
    document()->documentLayout()->blockBoundingRect(block);
    const auto *native = block.layout();
    const qreal width = native->lineCount() > 0 ? native->lineAt(0).width() : 0;
    auto &entry = m_layouts[block.blockNumber()];
    if (entry && entry->layout.font() == document()->defaultFont() &&
        entry->lineWidth == width && entry->lineCount == native->lineCount()) return *entry;
    entry = std::make_unique<BlockLayout>();
    auto &result = *entry;
    result.lineWidth = width;
    result.lineCount = native->lineCount();
    QString source = block.text();
    result.preeditPosition = native->preeditAreaPosition();
    result.preeditLength = int(native->preeditAreaText().size());
    if (result.preeditPosition >= 0)
        source.insert(result.preeditPosition, native->preeditAreaText());
    result.display = lc::textbidi::directionalText(source, block.textDirection());
    result.layout.setText(result.display.text);
    result.layout.setFont(document()->defaultFont());
    result.layout.setTextOption(native->textOption());
    result.layout.setCursorMoveStyle(Qt::VisualMoveStyle);
    result.layout.setCacheEnabled(true);
    auto formats = block.textFormats();
    for (auto &format : formats) {
        const int end = result.displayPosition(format.start + format.length);
        format.start = result.displayPosition(format.start);
        format.length = end - format.start;
    }
    for (auto format : native->formats()) {
        const int end = result.display.displayPositions.at(format.start + format.length);
        format.start = result.display.displayPositions.at(format.start);
        format.length = end - format.start;
        formats.append(format);
    }
    result.layout.setFormats(formats);
    result.layout.beginLayout();
    for (int index = 0; index < native->lineCount(); ++index) {
        auto line = result.layout.createLine();
        if (!line.isValid()) break;
        const auto nativeLine = native->lineAt(index);
        const int sourceEnd = nativeLine.textStart() + nativeLine.textLength();
        const int displayEnd = index + 1 == native->lineCount() ? int(result.display.text.size())
                               : result.display.displayPositions.at(sourceEnd);
        line.setLeadingIncluded(true);
        // Keep native wrapping/scroll extents; synthetic controls have no width.
        line.setNumColumns(displayEnd - line.textStart(), nativeLine.width());
        line.setPosition(QPointF(nativeLine.x(), nativeLine.y()));
    }
    result.layout.endLayout();
    return result;
}

QRect LC_TextEdit::cursorRect(const QTextCursor &cursor) const {
    if (!hasChineseRtl()) return QTextEdit::cursorRect(cursor);
    const auto block = cursor.block();
    const auto &view = layoutForBlock(block);
    int position = displayCursorPosition(cursor, view);
    if (view.preeditLength && cursor == textCursor()) {
        position = view.display.displayPositions.at(view.preeditPosition +
                     std::clamp(m_preeditCursor, 0, view.preeditLength));
    }
    const auto line = view.layout.lineForTextPosition(position);
    if (!line.isValid()) return QTextEdit::cursorRect(cursor);
    const auto origin = blockOrigin(block);
    return QRect(qRound(origin.x() + line.cursorToX(position)),
                 qRound(origin.y() + line.y()), cursorWidth(), qCeil(line.height()));
}

QTextCursor LC_TextEdit::cursorForPosition(const QPoint &position) const {
    if (!hasChineseRtl()) return QTextEdit::cursorForPosition(position);
    auto cursor = QTextEdit::cursorForPosition(position);
    const auto block = cursor.block();
    const auto &view = layoutForBlock(block);
    const auto local = position - blockOrigin(block);
    const auto line = view.lineAtY(local.y());
    if (!line.isValid()) return cursor;
    cursor.setPosition(block.position() + view.documentPosition(view.hitTest(line, local.x())));
    return cursor;
}

void LC_TextEdit::paintEvent(QPaintEvent *event) {
    if (!hasChineseRtl()) {
        QTextEdit::paintEvent(event);
        return;
    }
    QPainter painter(viewport());
    painter.fillRect(event->rect(), palette().brush(QPalette::Base));
    painter.setPen(palette().color(QPalette::Text));
    const auto cursor = textCursor();
    for (auto block = QTextEdit::cursorForPosition(event->rect().topLeft()).block();
         block.isValid(); block = block.next()) {
        const auto origin = blockOrigin(block);
        if (origin.y() > event->rect().bottom()) break;
        const auto bounds = document()->documentLayout()->blockBoundingRect(block).translated(
                                origin - block.layout()->position());
        if (!bounds.intersects(event->rect())) continue;
        const auto &view = layoutForBlock(block);
        if (!view.layout.boundingRect().translated(origin).intersects(event->rect())) continue;
        QList<QTextLayout::FormatRange> selections;
        const int start = cursor.selectionStart() - block.position();
        const int end = cursor.selectionEnd() - block.position();
        if (cursor.hasSelection() && start < block.length() && end > 0) {
            QTextLayout::FormatRange range;
            range.start = start <= 0 ? std::max(-1, start) : view.displayPosition(start);
            const int finish = end >= block.length() ? int(view.display.text.size()) + 1 : view.displayPosition(end);
            range.length = finish - range.start;
            const auto group = hasFocus() ? QPalette::Active : QPalette::Inactive;
            range.format.setBackground(palette().brush(group, QPalette::Highlight));
            range.format.setForeground(palette().brush(group, QPalette::HighlightedText));
            QStyleOption option;
            option.initFrom(this);
            range.format.setProperty(QTextFormat::FullWidthSelection,
                                     style()->styleHint(QStyle::SH_RichText_FullWidthSelection, &option, this));
            selections.append(range);
        }
        QRectF clip = event->rect();
        clip.setLeft(std::max(clip.left(), bounds.left()));
        clip.setRight(std::min(clip.right(), bounds.right()));
        view.layout.draw(&painter, origin, selections, clip);
    }
    if (hasFocus() && !isReadOnly() && m_cursorVisible && m_preeditCursorVisible)
        painter.fillRect(cursorRect(), palette().brush(QPalette::Text));
    if (m_dropPosition)
        painter.fillRect(cursorRect(cursorForPosition(m_dropPosition->toPoint())), palette().brush(QPalette::Text));
}

void LC_TextEdit::keyPressEvent(QKeyEvent *event) {
    if (!hasChineseRtl()) { QTextEdit::keyPressEvent(event); return; }
    const auto mode = event->modifiers() & Qt::ShiftModifier
                        ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;
    const bool left = event->matches(QKeySequence::MoveToPreviousChar) || event->matches(QKeySequence::SelectPreviousChar);
    const bool right = event->matches(QKeySequence::MoveToNextChar) || event->matches(QKeySequence::SelectNextChar);
    if (left || right) {
        auto cursor = textCursor();
        const auto block = cursor.block();
        const auto &view = layoutForBlock(block);
        const int position = displayCursorPosition(cursor, view);
        if (cursor.hasSelection() && mode == QTextCursor::MoveAnchor) {
            auto start = cursor;
            auto finish = cursor;
            start.setPosition(cursor.selectionStart());
            finish.setPosition(cursor.selectionEnd());
            const auto first = cursorRect(start);
            auto last = cursorRect(finish);
            const auto &endView = layoutForBlock(finish.block());
            int endPosition = endView.displayPosition(finish.positionInBlock());
            while (endPosition > 0 && endView.display.sourcePositions[endPosition - 1] < 0) --endPosition;
            const auto endLine = endView.layout.lineForTextPosition(endPosition);
            if (endLine.isValid())
                last.moveTopLeft((blockOrigin(finish.block()) +
                                  QPointF(endLine.cursorToX(endPosition), endLine.y())).toPoint());
            const bool startIsLeft = first.y() == last.y() ? first.x() < last.x() : first.y() < last.y();
            if (left == startIsLeft)
                setDisplayCursor(start, layoutForBlock(start.block()).displayPosition(start.positionInBlock()), mode);
            else
                setDisplayCursor(finish, endPosition, mode);
            event->accept();
            return;
        }
        int next = position;
        do {
            const int previous = next;
            next = left ? view.layout.leftCursorPosition(next) : view.layout.rightCursorPosition(next);
            if (next < 0 || next == previous) { next = position; break; }
        } while (next != position &&
                 (!view.layout.isValidCursorPosition(next) ||
                  view.documentPosition(next) == cursor.positionInBlock()));
        if (next != position) {
            setDisplayCursor(cursor, next, mode);
            event->accept();
            return;
        }
        // At a visual line edge, cross to the next logical paragraph only.
        const bool forward = left == (block.textDirection() == Qt::RightToLeft);
        const auto adjacent = forward ? block.next() : block.previous();
        if (adjacent.isValid()) {
            cursor.setPosition(adjacent.position() + (forward ? 0 : adjacent.length() - 1), mode);
            setTextCursor(cursor);
        }
        event->accept();
        return;
    }
    const bool up = event->matches(QKeySequence::MoveToPreviousLine) || event->matches(QKeySequence::SelectPreviousLine);
    const bool down = event->matches(QKeySequence::MoveToNextLine) || event->matches(QKeySequence::SelectNextLine);
    const bool home = event->matches(QKeySequence::MoveToStartOfLine) || event->matches(QKeySequence::SelectStartOfLine);
    const bool end = event->matches(QKeySequence::MoveToEndOfLine) || event->matches(QKeySequence::SelectEndOfLine);
    if (up || down || home || end) {
        auto cursor = textCursor();
        auto block = cursor.block();
        const auto *view = &layoutForBlock(block);
        auto line = view->layout.lineForTextPosition(displayCursorPosition(cursor, *view));
        if (line.isValid()) {
            if (home || end) {
                const int position = home ? line.textStart()
                                        : line.textStart() + line.textLength();
                setDisplayCursor(cursor, position, mode);
            } else {
                const qreal x = m_verticalX >= 0 ? m_verticalX : cursorRect().x();
                const int step = up ? -1 : 1;
                int index = line.lineNumber() + step;
                if (index < 0 || index >= view->layout.lineCount()) {
                    const auto adjacent = step < 0 ? block.previous() : block.next();
                    if (!adjacent.isValid()) { event->accept(); return; }
                    block = adjacent;
                    cursor.setPosition(block.position(), mode);
                    view = &layoutForBlock(block);
                    index = step < 0 ? view->layout.lineCount() - 1 : 0;
                }
                line = view->layout.lineAt(index);
                setDisplayCursor(cursor, view->hitTest(line, x - blockOrigin(block).x()), mode);
                m_verticalX = x;
            }
            event->accept();
            return;
        }
    }
    QTextEdit::keyPressEvent(event);
}

bool LC_TextEdit::viewportEvent(QEvent *input) {
    switch (input->type()) {
    case QEvent::MouseButtonPress: case QEvent::MouseMove:
    case QEvent::MouseButtonRelease: case QEvent::MouseButtonDblClick:
        break;
    default:
        return QTextEdit::viewportEvent(input);
    }
    if (!hasChineseRtl()) return QTextEdit::viewportEvent(input);
    auto *event = static_cast<QMouseEvent *>(input);
    const auto cursor = cursorForPosition(event->position().toPoint());
    QPointF nativePoint = QTextEdit::cursorRect(cursor).center();
    {
        const auto block = cursor.block();
        const auto &view = layoutForBlock(block);
        const auto origin = blockOrigin(block);
        const auto local = event->position() - origin;
        const auto line = view.lineAtY(local.y());
        if (view.preeditLength && line.isValid()) {
            const int position = view.display.sourcePosition(view.hitTest(line, local.x()));
            if (position >= view.preeditPosition && position <= view.preeditPosition + view.preeditLength) {
                // IME clicks address the composing string, not the collapsed document position.
                const auto nativeLine = block.layout()->lineForTextPosition(position);
                if (nativeLine.isValid())
                    nativePoint = origin + QPointF(nativeLine.cursorToX(position),
                                                  nativeLine.y() + nativeLine.height() / 2);
            }
        }
    }
    QMouseEvent mapped(event->type(), nativePoint, event->scenePosition(),
                       event->globalPosition(), event->button(), event->buttons(),
                       event->modifiers(), Qt::MouseEventSynthesizedByApplication, event->pointingDevice());
    const bool handled = QTextEdit::viewportEvent(&mapped);
    const auto block = textCursor().block();
    if (cursor.position() == textCursor().position()) {
        const auto &view = layoutForBlock(block);
        const auto local = event->position() - blockOrigin(block);
        const auto line = view.lineAtY(local.y());
        if (line.isValid()) {
            m_cursorBlock = block.blockNumber();
            m_displayCursor = view.hitTest(line, local.x());
        }
    }
    m_mousePosition = event->position();
    if (event->type() == QEvent::MouseMove && (event->buttons() & Qt::LeftButton) &&
        !viewport()->rect().contains(m_mousePosition.toPoint())) {
        if (!m_selectionScrollTimer.isActive()) m_selectionScrollTimer.start(100, this);
    } else {
        m_selectionScrollTimer.stop();
    }
    event->setAccepted(mapped.isAccepted());
    return handled;
}

QVariant LC_TextEdit::inputMethodQuery(Qt::InputMethodQuery query) const {
    return inputMethodQuery(query, {});
}

QVariant LC_TextEdit::inputMethodQuery(Qt::InputMethodQuery query, QVariant argument) const {
    if (query == Qt::ImCursorRectangle && hasChineseRtl()) return cursorRect();
    if (query == Qt::ImAnchorRectangle && hasChineseRtl()) {
        auto cursor = textCursor();
        cursor.setPosition(cursor.anchor());
        return cursorRect(cursor);
    }
    if (hasChineseRtl() && (query == Qt::ImCursorPosition || query == Qt::ImAbsolutePosition) &&
        argument.canConvert<QPointF>() && !argument.toPointF().isNull()) {
        const int position = cursorForPosition(argument.toPointF().toPoint()).position();
        return query == Qt::ImAbsolutePosition ? position : position - textCursor().block().position();
    }
    return QTextEdit::inputMethodQuery(query, argument);
}

void LC_TextEdit::inputMethodEvent(QInputMethodEvent *event) {
    m_preeditCursor = int(event->preeditString().size());
    m_preeditCursorVisible = true;
    for (const auto &attribute : event->attributes()) {
        if (attribute.type == QInputMethodEvent::Cursor) {
            m_preeditCursor = attribute.start;
            m_preeditCursorVisible = attribute.length != 0;
        }
    }
    QTextEdit::inputMethodEvent(event);
    invalidateLayouts();
}

int LC_TextEdit::displayCursorPosition(const QTextCursor &cursor, const BlockLayout &view) const {
    if (cursor == textCursor() && m_cursorBlock == cursor.blockNumber() && m_displayCursor >= 0)
        return m_displayCursor;
    return view.displayPosition(cursor.positionInBlock());
}

void LC_TextEdit::setDisplayCursor(QTextCursor cursor, int position, QTextCursor::MoveMode mode) {
    const auto block = cursor.block();
    cursor.setPosition(block.position() + layoutForBlock(block).documentPosition(position), mode);
    setTextCursor(cursor);
    m_cursorBlock = block.blockNumber();
    m_displayCursor = position;
    m_verticalX = -1;
    ensureDisplayCursorVisible();
    viewport()->update();
}

void LC_TextEdit::ensureDisplayCursorVisible() {
    if (!hasChineseRtl()) return;
    const auto rect = cursorRect();
    auto *horizontal = horizontalScrollBar();
    const int sign = isRightToLeft() ? -1 : 1;
    if (rect.left() < 0) horizontal->setValue(horizontal->value() + sign * rect.left());
    else if (rect.right() >= viewport()->width())
        horizontal->setValue(horizontal->value() + sign * (rect.right() - viewport()->width() + 1));
    QGuiApplication::inputMethod()->update(Qt::ImCursorRectangle | Qt::ImAnchorRectangle);
}

void LC_TextEdit::dropEvent(QDropEvent *event) {
    if (!hasChineseRtl()) { QTextEdit::dropEvent(event); return; }
    QDragLeaveEvent leave;
    dragLeaveEvent(&leave);
    if (isReadOnly() || !canInsertFromMimeData(event->mimeData())) { event->ignore(); return; }
    auto insertion = cursorForPosition(event->position().toPoint());
    insertion.beginEditBlock();
    if (event->dropAction() == Qt::MoveAction &&
        (event->source() == this || event->source() == viewport())) {
        auto selection = textCursor();
        selection.removeSelectedText();
    }
    setTextCursor(insertion);
    insertFromMimeData(event->mimeData());
    insertion.endEditBlock();
    ensureDisplayCursorVisible();
    event->accept();
}

void LC_TextEdit::dragMoveEvent(QDragMoveEvent *event) {
    QTextEdit::dragMoveEvent(event);
    if (hasChineseRtl() && event->isAccepted()) {
        m_dropPosition = event->position();
        viewport()->update();
    }
}

void LC_TextEdit::dragLeaveEvent(QDragLeaveEvent *event) {
    QTextEdit::dragLeaveEvent(event);
    m_dropPosition.reset();
    m_selectionScrollTimer.stop();
    viewport()->update();
}

void LC_TextEdit::focusInEvent(QFocusEvent *event) {
    QTextEdit::focusInEvent(event);
    m_cursorVisible = true;
    viewport()->update();
}

void LC_TextEdit::focusOutEvent(QFocusEvent *event) {
    QTextEdit::focusOutEvent(event);
    m_selectionScrollTimer.stop();
    viewport()->update();
}

void LC_TextEdit::resizeEvent(QResizeEvent *event) {
    QTextEdit::resizeEvent(event);
    invalidateLayouts();
}
void LC_TextEdit::changeEvent(QEvent *event) {
    QTextEdit::changeEvent(event);
    invalidateLayouts();
}
void LC_TextEdit::timerEvent(QTimerEvent *event) {
    if (event->timerId() == m_selectionScrollTimer.timerId()) {
        auto *horizontal = horizontalScrollBar();
        auto *vertical = verticalScrollBar();
        const int dx = qRound(m_mousePosition.x()) - std::clamp(qRound(m_mousePosition.x()), 0, std::max(0, viewport()->width() - 1));
        const int dy = qRound(m_mousePosition.y()) - std::clamp(qRound(m_mousePosition.y()), 0, std::max(0, viewport()->height() - 1));
        horizontal->setValue(horizontal->value() + (isRightToLeft() ? -1 : 1) *
                             std::clamp(dx, -horizontal->singleStep(), horizontal->singleStep()));
        vertical->setValue(vertical->value() + std::clamp(dy, -vertical->singleStep(), vertical->singleStep()));
        QMouseEvent move(QEvent::MouseMove, m_mousePosition, viewport()->mapToGlobal(m_mousePosition.toPoint()),
                         Qt::NoButton, Qt::LeftButton, QApplication::keyboardModifiers());
        viewportEvent(&move);
    } else if (event->timerId() == m_cursorTimer.timerId()) {
        m_cursorVisible = !m_cursorVisible;
        if (hasFocus() && hasChineseRtl()) viewport()->update();
    } else {
        QTextEdit::timerEvent(event);
    }
}

void LC_SingleLineTextEdit::inputMethodEvent(QInputMethodEvent *event) {
    event->setCommitString(singleLine(event->commitString()), event->replacementStart(), event->replacementLength());
    LC_TextEdit::inputMethodEvent(event);
}
