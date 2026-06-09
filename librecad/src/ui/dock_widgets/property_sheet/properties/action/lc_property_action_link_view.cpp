/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#include "lc_property_action_link_view.h"

#include <QKeyEvent>
#include <qnetworkreply.h>

#include "lc_properties_sheet.h"
#include "lc_property_view_part.h"
#include "rs_debug.h"

const QByteArray LC_PropertyActionLinkView::VIEW_NAME = QByteArrayLiteral("Link");
const QByteArray LC_PropertyActionLinkView::ATTR_TITLE = QByteArrayLiteral("title");
const QByteArray LC_PropertyActionLinkView::ATTR_ENABLED_LEFT = QByteArrayLiteral("enabledLeft");
const QByteArray LC_PropertyActionLinkView::ATTR_ENABLED_RIGHT = QByteArrayLiteral("enabledRight");
const QByteArray LC_PropertyActionLinkView::ATTR_TITLE_RIGHT = QByteArrayLiteral("titleRight");
const QByteArray LC_PropertyActionLinkView::ATTR_TOOLTIP_LEFT = QByteArrayLiteral("tooltipLeft");
const QByteArray LC_PropertyActionLinkView::ATTR_TOOLTIP_RIGHT = QByteArrayLiteral("tooltipRight");

LC_PropertyActionLinkView::LC_PropertyActionLinkView(LC_PropertyAction* property)
    : LC_PropertyView(property) {
}

void LC_PropertyActionLinkView::doApplyAttributes(const LC_PropertyViewDescriptor& info) {
    info.load(ATTR_TITLE, m_titleLeft);
    info.load(ATTR_TITLE_RIGHT, m_titleRight);
    info.load(ATTR_TOOLTIP_LEFT, m_tooltipLeft);
    info.load(ATTR_TOOLTIP_RIGHT, m_tooltipRight);
    info.load(ATTR_ENABLED_LEFT, m_enabledLeft);
    info.load(ATTR_ENABLED_RIGHT, m_enabledRight);
}

LC_PropertyAction& LC_PropertyActionLinkView::typedProperty() const {
    return *static_cast<LC_PropertyAction*>(this->getProperty());
}

void LC_PropertyActionLinkView::buildPartBackground(const LC_PropertyPaintContext& ctx, QList<LC_PropertyViewPart>& parts) {
    LC_PropertyViewPart part(ctx.rect);
    if (!part.rect.isValid()) {
        return;
    }
    part.funPaint = [](const LC_PropertyPaintContext& paintContext, const LC_PropertyViewPart& p) {
        auto& painter = *paintContext.painter;
        const auto& rect = p.rect;
        if (paintContext.isActive) {
            painter.fillRect(p.rect, paintContext.getPalette().color(QPalette::Highlight));
        }
        const QPen oldPen = painter.pen();
        const QPen linesPen(paintContext.getPalette().color(QPalette::Button));
        painter.setPen(linesPen);

        // draw part borders
        painter.drawLine(rect.bottomLeft(), rect.bottomRight());
        painter.setPen(oldPen);
    };

    parts.append(part);
}

void LC_PropertyActionLinkView::builSingleLinkPart(const QRect& valuesRect, const QString& title, const QString& tooltip, int linkIndex,
                                                   QList<LC_PropertyViewPart>& parts, bool linkEnabled) {
    LC_PropertyViewPart part(valuesRect);
    // part.m_rect.setWidth(ctx.painter->fontMetrics().boundingRect(m_title).width() + 5);
    if (tooltip.isEmpty()) {
        part.setPropertyDescriptionAsTooltip(typedProperty());
    }
    else {
        part.funGetTooltip = [tooltip](LC_PropertyEventContext&, const LC_PropertyViewPart&) -> QString {
            return tooltip;
        };
    }
    part.trackState();

    part.funPaint = [title, this,linkIndex, linkEnabled](const LC_PropertyPaintContext& paintContext, const LC_PropertyViewPart& p) {
        bool drawPressed = false;
        if (isLocked()) {
            if (isClicked(linkIndex)) {
                drawPressed = true;
            }
            else {
                return;
            }
        }
        const auto painter = paintContext.painter;
        painter->save();
        QColor linkColor;
        if (linkEnabled) {
            linkColor = paintContext.getPalette().color(paintContext.getCurrentColorGroup(),
                                                                     paintContext.isActive ? QPalette::HighlightedText : QPalette::Link);
            if (drawPressed) {
                auto font = painter->font();
                font.setUnderline(true);
                painter->setFont(font);
            }
            else if (p.isUnderCursor()) {
                auto font = painter->font();
                font.setUnderline(true);
                painter->setFont(font);
            }
            else if (p.isPushed()) {
                auto font = painter->font();
                font.setUnderline(true);
                painter->setFont(font);
            }
        }
        else {
            linkColor = paintContext.getPalette().color(QPalette::Disabled, QPalette::Text);
        }

        painter->setPen(linkColor);
        painter->drawText(p.rect, Qt::AlignLeading | Qt::AlignVCenter, title);
        painter->restore();
    };

    if (linkEnabled) {
        part.funHandleEvent = [this, linkIndex](LC_PropertyEventContext& eventContext, const LC_PropertyViewPart&,
                                                LC_PropertyEditContext*) -> bool {
            bool doClick = false;
            if (isLocked()) {
                return false;
            }
            switch (eventContext.eventType()) {
                case QEvent::KeyPress: {
                    const int key = eventContext.typedEvent<QKeyEvent>()->key();
                    doClick = (key == Qt::Key_Space) || (key == Qt::Key_Return);
                    break;
                }
                case LC_PropertyViewPartEvent::Activated: {
                    m_cursorSet = true;
                    const auto sheet = eventContext.sheet;
                    m_widgetCursor = sheet->cursor();
                    sheet->setCursor(Qt::PointingHandCursor);
                    break;
                }
                case LC_PropertyViewPartEvent::Deactivated: {
                    if (m_cursorSet) {
                        const auto sheet = eventContext.sheet;
                        if (!sheet->isInTreeRebuild()) {
                            sheet->setCursor(m_widgetCursor);
                        }
                    }
                    break;
                }
                case LC_PropertyViewPartEvent::ReleaseMouse: {
                    doClick = true;
                    break;
                }
                default:
                    break;
            }
            if (doClick) {
                // ctx.m_sheet->setSkipNextMouseReleaseEvent();
                lock(linkIndex);
                typedProperty().invokeClick(linkIndex);
                return false;
            }
            return false;
        };
    }
    parts.append(part);
}

void LC_PropertyActionLinkView::doBuildViewParts(LC_PropertyPaintContext& ctx, QList<LC_PropertyViewPart>& parts) {
    buildPartBackground(ctx, parts);
    const auto valuesRect = ctx.rect.marginsRemoved(ctx.margins);
    const bool hasNoRightPart = m_titleRight.isEmpty();

    if (hasNoRightPart) {
        builSingleLinkPart(valuesRect, m_titleLeft, m_tooltipLeft, 0, parts, m_enabledLeft);
    }
    else {
        const int splitPos = ctx.splitPos;
        QRect leftRect = valuesRect;
        leftRect.setRight(splitPos);

        builSingleLinkPart(leftRect, m_titleLeft, m_tooltipLeft, 0, parts, m_enabledLeft);

        QRect rightRect = valuesRect;
        rightRect.setLeft(splitPos);
        builSingleLinkPart(rightRect, m_titleRight, m_tooltipRight, 1, parts, m_enabledRight);
    }
}
