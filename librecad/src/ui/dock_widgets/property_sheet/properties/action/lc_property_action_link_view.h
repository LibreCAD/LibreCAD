/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
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

#ifndef LC_PROPERTYACTIONLINKVIEW_H
#define LC_PROPERTYACTIONLINKVIEW_H

#include "lc_property_action.h"
#include "lc_property_view.h"

class LC_PropertyActionLinkView : public LC_PropertyView {
    Q_DISABLE_COPY(LC_PropertyActionLinkView)

public:
    static const QByteArray VIEW_NAME;
    static const QByteArray ATTR_TITLE;
    static const QByteArray ATTR_ENABLED_LEFT;
    static const QByteArray ATTR_ENABLED_RIGHT;
    static const QByteArray ATTR_TITLE_RIGHT;
    static const QByteArray ATTR_TOOLTIP_LEFT;
    static const QByteArray ATTR_TOOLTIP_RIGHT;

    explicit LC_PropertyActionLinkView(LC_PropertyAction* property);

    bool isSplittable() const override {
        return false;
    }

    bool isLocked() {
        return m_clickedLink != -1;
    }

    void lock(const int index) {
        m_clickedLink = index;
    }

    bool isClicked(int linkIndex) const {
        return m_clickedLink == linkIndex;
    }

protected:
    void buildPartBackground(const LC_PropertyPaintContext& ctx, QList<LC_PropertyViewPart>& parts);
    void builSingleLinkPart(const QRect& valuesRect, const QString& title, const QString& tooltip, int linkIndex,
                            QList<LC_PropertyViewPart>& parts, bool linkEnabled);
    void doApplyAttributes(const LC_PropertyViewDescriptor& info) override;
    void doBuildViewParts(LC_PropertyPaintContext& ctx, QList<LC_PropertyViewPart>& parts) override;
    LC_PropertyAction& typedProperty() const;

private:
    QCursor m_widgetCursor;
    bool m_cursorSet = false;
    QString m_titleLeft;
    QString m_titleRight;
    QString m_tooltipLeft;
    QString m_tooltipRight;
    bool m_enabledLeft{true};
    bool m_enabledRight{true};
    int m_clickedLink = -1;
};

#endif
